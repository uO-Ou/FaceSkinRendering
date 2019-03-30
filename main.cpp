#include <common/glfwApp.h>
#include <OpenglWrappers/glHelper.h>
#include <OpenglWrappers/DemoMeshes/QuadMesh.h>
#include <OpenglWrappers/DemoMeshes/UnlitMesh.h> 
#include <OpenglWrappers/DemoMeshes/BlinnPhongMesh.h>
#include <OpenglWrappers/Effects/FrameBufferObject.h>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "SSS.h"
#include "ImgHelper.h"

//#define UV_STRETCH
#define MODELPATH "../LPSHead/head.obj"

//window size
auto winsize = redips::int2(960, 960);

//glfw setup
auto application = redips::glfw::getInstance(winsize.x, winsize.y);

//standard pinhole camera
redips::PhC phc(60, 1.0f, 0.1f, 100.0f);

//sss initialize
SSS *sss = new SSS(redips::int2(960, 960), MODELPATH, "../LPSHead/uv-stretch.bmp", "./faceUnwarp", "./blur", "./merger");

//meshptr
auto mesh0 = sss->dpmesh; //depth pass
auto mesh1 = sss->bpmesh; //face unwarp, calculate irrmap0
auto mesh2 = sss->mgmesh; //merge blurered irrmaps
auto mesh3 = new redips::BlinnPhongMeshTBN((redips::glMeshWrapper&)(*mesh1), sss->fbo4shadowmap->depthMap);

#ifdef UV_STRETCH
redips::UnlitMesh gunlit_mesh(*mesh1, "./gen_uv_stretch_texture");
#endif // UV_STRETCH

//for debug
redips::QuadMesh texture_render("./render_texture");

//screen capture
auto capture = redips::glScreenCapture::getInstance(winsize);

enum class InputMode : unsigned char { IMGUI, MOUSE };
InputMode input_mode = InputMode::IMGUI;

bool usesss = true;
bool shadowed = true;
char input_text_4_imgname[1024];
void movement() {
	if (application->keydown(GLFW_KEY_C)) {
		phc.save("camera.config");
	}
	if (application->keydown(GLFW_KEY_O)) {
		phc.load("camera.config");
	}
	if (application->keydown(GLFW_KEY_M)) {
		capture->capture("capture.bmp", 1.0);
		ImgHelper::swap_channel("capture.bmp");
	}
	if (application->keydown(GLFW_KEY_P)) {
		//application->swapCursorMode();
		if (input_mode == InputMode::IMGUI) {
			application->acceptMouseControl(true);
			application->setCursorMode(GLFW_CURSOR_DISABLED);
			input_mode = InputMode::MOUSE;
		}
		else {
			application->acceptMouseControl(false);
			application->setCursorMode(GLFW_CURSOR_NORMAL);
			input_mode = InputMode::IMGUI;
		}
	}

	if (input_mode == InputMode::IMGUI) {
		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		static float irrmix = 0.2f;
		ImGui::SliderFloat("irrmix", &irrmix, 0, 1);
		sss->set_irrmix_ratio(irrmix);

		ImGui::Checkbox("enable sss", &usesss);
		ImGui::Checkbox("shadowed", &shadowed);
		ImGui::InputText("save image to", input_text_4_imgname, 1024);

		std::string imgpath = "save image to " + std::string(input_text_4_imgname);
		if (ImGui::Button(imgpath.c_str())) {                            // Buttons return true when clicked (most widgets return true when edited/activated)
			capture->capture(input_text_4_imgname, 1.0);
			ImgHelper::swap_channel(input_text_4_imgname);
		}

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
	}

	//application->acceptMouseControl(accmouse);
}

void display() {
#ifdef UV_STRETCH
	gunlit_mesh.uniformMat44f("model", redips::Mat44f::eye().ptr());
	gunlit_mesh.uniformMat44f("projection_view", phc.glProjectionView().ptr());
	gunlit_mesh.draw();
#else

	auto scenebox = mesh1->model_ptr()->aabb_T();
	//auto litdir = (scenebox.heart() - phc.pos()).unit();
	auto litdir = redips::float3{ -0.644989f, -0.181168, -0.742407 };

	if (shadowed) {
		auto litpv = redips::DirectionalLight(litdir).calProjectionView4ShadowMap(redips::float3(0, 1, 0), scenebox).transpose();
		mesh0->uniformMat44f("model", redips::Mat44f::eye().ptr());
		mesh0->uniformMat44f("projection_view", litpv.ptr());

		mesh1->uniformMat44f("light_space_ort_projection_view", litpv.ptr());
		mesh3->uniformMat44f("light_space_ort_projection_view", litpv.ptr());
	}

	if (usesss) {
		//mesh1->uniformInt1("pointLightNumber", 1);
		//mesh1->uniformFloat3("pointLights[0].position", phc.pos());
		//mesh1->uniformFloat3("pointLights[0].intensity", redips::float3(1, 1, 1));
		//mesh1->uniformFloat3("pointLights[0].attenuation", redips::float3(1, 0.0000002, 0.00002));

		mesh1->uniformInt1("directionaLightNumber", 1);
		mesh1->uniformFloat3("directionaLights[0].direction", litdir);
		mesh1->uniformFloat3("directionaLights[0].intensity", redips::float3(1, 1, 1));

		mesh1->uniformFloat3("cameraPosition", phc.pos());

		mesh1->uniformMat44f("model", redips::Mat44f::rotatey(RAD(0)).transpose().ptr());
		mesh1->uniformMat44f("projection_view", phc.glProjectionView().ptr());

		mesh2->uniformMat44f("model", redips::Mat44f::rotatey(RAD(0)).transpose().ptr());
		mesh2->uniformMat44f("projection_view", phc.glProjectionView().ptr());

		sss->render(shadowed);
		//texture_render.render();
	}
	else {
		//mesh3->uniformInt1("pointLightNumber", 1);
		//mesh3->uniformFloat3("pointLights[0].position", phc.pos());
		//mesh3->uniformFloat3("pointLights[0].intensity", redips::float3(1, 1, 1));
		//mesh3->uniformFloat3("pointLights[0].attenuation", redips::float3(1, 0.0000002, 0.00002));

		if (shadowed) {
			sss->fbo4shadowmap->bind4Writing();
			mesh0->draw();
			sss->fbo4shadowmap->unbind();
		}

		mesh3->uniformInt1("directionaLightNumber", 1);
		mesh3->uniformFloat3("directionaLights[0].direction", litdir);
		mesh3->uniformFloat3("directionaLights[0].intensity", redips::float3(1, 1, 1));

		mesh3->uniformFloat3("cameraPosition", phc.pos());

		mesh3->uniformMat44f("model", redips::Mat44f::rotatey(RAD(0)).transpose().ptr());
		mesh3->uniformMat44f("projection_view", phc.glProjectionView().ptr());
		mesh3->draw(shadowed);
	}
#endif // UNLIT

	movement();
}

void initialize() {
	//mesh center
	redips::float3 heart = mesh1->model_ptr()->aabb_T().heart();

	//setup camera
	phc.lookAt(heart + redips::float3(0, 0, 40), heart, redips::float3(0, 1, 0));

	phc.load("camera.config");

	texture_render.bindTexture(0, sss->irradiances[0], "picture");
	//texture_render.bindTexture(0, sss->fbo4faceUnWarp->colorMap, "picture");
	//texture_render.bindTexture(0, blurer.mFbo2->colorMap, "picture");

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer bindings
	ImGui_ImplGlfw_InitForOpenGL(application->getGlfWindow(), true);
	ImGui_ImplOpenGL3_Init(/*glsl_version*/);
}

int main() {
	initialize();
	application->registerDisplayCallback(display);
	application->bindCamera(&phc);
	application->acceptMouseControl(false);
	application->setCursorMode(GLFW_CURSOR_NORMAL);
	application->loop();
	system("pause");
	return 0;
}