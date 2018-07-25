#include <Common/glfwApp.h>
#include <OpenglWrappers/glHelper.h>
#include <OpenglWrappers/Effects/GBuffer.h>
#include <OpenglWrappers/DemoMeshes/QuadMesh.h>

#include "blur.h"
#include "Merger.h"
#include "PLYMesh.h"
#include "ImageHelper.h"
#include "OptixRenderer.h"
#include "LightsGenerator.h"

#define FACE_MODEL_PATH "objs/00052_20061024_00526_neutral_face05.ply.obj"
#define FACE_COLOR_PATH "objs/00052_20061024_00526_neutral_face05.ply.color"
#define FACE_IMAGE_PREFIX "./capture/p09/p09"

#define OPTIX_RENDER

const redips::int2 WinSize = redips::int2{ 640,640 };

auto application = redips::glfw::getInstance(WinSize.x, WinSize.y);

redips::PhC phc(60, WinSize.x*1.0f / WinSize.y, 1.0f, 10000);

redips::GBuffer gbuffer(WinSize.x, WinSize.y);

const float roughness = 0.5;
OptixRenderer *optixRenderer = nullptr;

double blur_gaussian_width[] {0.0064, 0.0484, 0.187, 0.567, 1.99, 7.41};
FaceBlur faceblur(gbuffer.getTexture(redips::GBuffer::_GL_GBUFFER_TEXTURE_TYPE_::_normal_), WinSize, "./blur");

//merge diff0-diff6+specular textures;
Merger merger(WinSize,"./merger");

//for debug
redips::QuadMesh texture_render("./render_texture");

//create a screen-capture to capture rendered image
auto screenCapture = redips::glScreenCapture::getInstance(WinSize);

//load a obj and then wrap into a glMesh¡£
#ifdef OPTIX_RENDER
	PLYMesh face_mesh(WinSize, new redips::Triangles(FACE_MODEL_PATH), FACE_COLOR_PATH, "./face_reflectance_map.bmp", "./PLYMesh_GBuffer");
#else
    PLYMesh face_mesh(WinSize, new redips::Triangles(FACE_MODEL_PATH), FACE_COLOR_PATH, "./face_reflectance_map.bmp", "./PLYMesh");
#endif

clock_t start = clock();
class SceneManager{
	redips::PhC* m_phc;
	std::string litsBase;
	redips::float3 focus;
	const int Radius = 300;
	LightsGenerator *litFactory;
	redips::float3 CurLightCenter;
	std::vector<redips::int2 > ATs;
	int CurLightId = 0, CurViewId = 0;
	redips::float3 UnitVec(float theta, float phi){ return redips::float3(cos(theta)*sin(phi), sin(theta), cos(theta)*cos(phi)); }
	void UpdateView(){
		auto AT = ATs[CurViewId];
		AT.x = AT.y = 0;
		auto position = UnitVec(RAD(AT.x), RAD(AT.y))*Radius + focus;
		m_phc->lookAt(position, focus, redips::float3(0, 1, 0));
	}
public:
	SceneManager(std::string litsBase, redips::PhC* phc) :m_phc(phc), litsBase(litsBase){
		for (int a = -24; a <= 24; a+=8) for (int t = -40; t <= 40; t+=8){
			ATs.push_back(redips::int2(a, t));
		}
		litFactory = new LightsGenerator(0.2, 16);
		litFactory ->load_all_lights(1024);
		litFactory->switch_lights(CurLightId, CurLightCenter);
		UpdateView();
	}
	GLuint lightVbo() const{ return litFactory->lights_vbo; }
	int lightCnt() const { return litFactory->light_number;  }
	void setFocus(const redips::float3& center){ focus = center; }
	
	bool update(){
		CurLightId += 20 + rand() % 20;
		if (CurLightId >= 1024){
			CurLightId = 0;
			++CurViewId;
			if (CurViewId >= ATs.size()) {
				clock_t end = clock();
				std::cout << double(end - start) / CLOCKS_PER_SEC / 60.0 << "minutes" << std::endl;
				exit(0);
			}
		}
		litFactory->switch_lights(CurLightId, CurLightCenter);
		UpdateView();
		return CurLightId == 1;
	}
	void save(const char* base){
		auto AT = ATs[CurViewId];
		char name[1024];
		sprintf(name,"%s_%03d_%03d_%04d.bmp", base, AT.x, AT.y, CurLightId);
		screenCapture->capture(name, 1);
		ImageHelper::swapchannel(name);
	}
}sceneManager(std::string("./lights/"), &phc);

void movement(){
	if (application->keydown(GLFW_KEY_M)){
		screenCapture->capture("face3.bmp", 1);
		ImageHelper::swapchannel("face3.bmp");
	}
	if (application->keydown(GLFW_KEY_SPACE)){
		sceneManager.update();
	}
	//sceneManager.save(FACE_IMAGE_PREFIX);
	//sceneManager.update();
}

void display(){
#ifdef OPTIX_RENDER
	gbuffer.bind4Writing();
#endif

	face_mesh.uniformMat44f("model", face_mesh.model_ptr()->Transform().transpose().ptr());
	face_mesh.uniformMat44f("projection_view", phc.glProjectionView().ptr());
	face_mesh.draw();

#ifdef OPTIX_RENDER
	gbuffer.unbind();
	optixRenderer->fly(phc.pos());

	faceblur.blur(optixRenderer->output_texture, merger.textures[0], sqrt(blur_gaussian_width[1]) - sqrt(blur_gaussian_width[0]));
	for (int i = 2; i < 6; ++i){
		faceblur.blur(merger.textures[i - 2], merger.textures[i - 1], sqrt(blur_gaussian_width[i]) - sqrt(blur_gaussian_width[i - 1]));
	}
	//texture_render.render();

	merger.render();
#endif
	
	movement();
}

void initialize(){
	using namespace redips;
	const_cast<Triangles*>(face_mesh.model_ptr())->setTransform(
		Mat44f::scale(float3(0.001))
	);
	auto heart = face_mesh.model_ptr()->aabb_T().heart();

	//setup camera
	phc.lookAt(heart + float3(0, 0, 300), heart, redips::float3(0, 1, 0));
	phc.setResolution(WinSize.x, WinSize.y);

	optixRenderer = new OptixRenderer(WinSize.x, WinSize.y, FACE_MODEL_PATH, face_mesh.model_ptr()->Transform(), sceneManager.lightVbo(), sceneManager.lightCnt(), gbuffer);
	optixRenderer ->setCamera(phc);
	optixRenderer ->setRoughness(roughness);
	optixRenderer ->setWinsize(WinSize.x, WinSize.y);
	optixRenderer ->setReflectanceTexture(face_mesh.reftex());

	merger.bindTexture(0, optixRenderer->output_texture, "diff0");
	merger.bindTexture(1, merger.textures[0], "diff1");
	merger.bindTexture(2, merger.textures[1], "diff2");
	merger.bindTexture(3, merger.textures[2], "diff3");
	merger.bindTexture(4, merger.textures[3], "diff4");
	merger.bindTexture(5, merger.textures[4], "diff5");

	texture_render.bindTexture(0, merger.textures[4], "picture");
	//texture_render.bindTexture(0, optixRenderer->output_texture, "picture");
	//texture_render.bindTexture(0,faceblur.texture_buffer, "picture");
	sceneManager.setFocus(heart);
	sceneManager.update();
}

int main(){
	initialize();
	application->bindCamera(&phc);
	application->registerDisplayCallback(display);
	application->acceptMouseControl(false);
	application->loop();
	return 0;
}