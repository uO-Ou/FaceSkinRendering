#pragma once
#include <GL/glew.h>
#include <OpenglWrappers/glMeshWrapper.h>
#include <OpenglWrappers/Effects/FrameBufferObject.h>
#include <OpenglWrappers/DemoMeshes/BlinnPhongMesh.h>
#include <OpenglWrappers/DemoMeshes/DepthPassMesh.h>

#include <map>
#include <string>

using namespace redips;

#define IRRMIX 0.2
#define SHADOWMAPX 1024
#define SHADOWMAPY 1024

class SSS {
	class FaceBlur {
		Shader shader;
		redips::int2 imgsize;
		redips::glTexture uvStretchMap;
	public:
		GLuint quardVao = 0, quardVbo = 0;
		redips::FrameBufferObject *mFbo1, *mFbo2;
		FaceBlur(redips::int2 imgsize, std::string shader_path, const std::string& uvStretchMapath) : imgsize(imgsize) {
			float data[] = { -1.0f, 1.0f, 0.0f, -1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f, -1.0f, 0.0f };
			glGenBuffers(1, &quardVbo);
			glBindBuffer(GL_ARRAY_BUFFER, quardVbo);
			glBufferData(GL_ARRAY_BUFFER, sizeof(data), &data, GL_STATIC_DRAW);

			glGenVertexArrays(1, &quardVao);
			glBindVertexArray(quardVao);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			useShader(redips::ShaderSource(shader_path.c_str()));
			shader.uniformInt1("input_texture", 0);
			shader.uniformInt1("stretch_map", 1);

			mFbo1 = new redips::FrameBufferObject(imgsize.x, imgsize.y);
			mFbo1 ->setupColorAndDepthAttachment();

			mFbo2 = new redips::FrameBufferObject(imgsize.x, imgsize.y);
			mFbo2 ->setupColorAndDepthAttachment();

			//stretch map
			if (uvStretchMapath.size() > 0) {
				auto img = new redips::FImage(uvStretchMapath.c_str());
				uvStretchMap.create2d(img, true);
				delete img;
			}

			CHECK_GL_ERROR("FaceBlur initialization failed");
		}

		~FaceBlur() {
			glDeleteBuffers(1, &quardVbo);
			glDeleteVertexArrays(1, &quardVao);

			delete mFbo1;
			delete mFbo2;
		}

		void blur(GLuint from, GLuint to, float gaussian_width) {

			shader.Use();
			shader.uniformFloat1("GaussWidth", gaussian_width);
			
			_blur_(from, mFbo1, 1);
			_blur_(mFbo1->colorMap, mFbo2, 0);
			
			//copy to destination
			mFbo2->bind4Reading();
			glBindTexture(GL_TEXTURE_2D, to);
			glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, imgsize.x, imgsize.y);
			glBindTexture(GL_TEXTURE_2D, 0);
			mFbo2->unbind();

			CHECK_GL_ERROR("FaceBlur.blur() failed");
		}

	private:

		void useShader(const redips::ShaderSource& source) {
			if (source.sourceType == ShaderSource::SourceType::_exists_program_) {
				shader = Shader(source.value.program);
			}
			else if (source.sourceType == ShaderSource::SourceType::_from_file_) {
				shader = Shader((std::string(source.value.path) + _vertex_shader_file_suffix_).c_str(), (std::string(source.value.path) + _fragment_shader_file_suffix_).c_str());
			}
		}

		void _blur_(GLuint from, redips::FrameBufferObject* mFbo, int UorV) {
			mFbo->bind();

			shader.uniformInt1("UorV", UorV);
			if (UorV) shader.uniformFloat1("ImgSize", imgsize.x);
			else shader.uniformFloat1("ImgSize", imgsize.y);

			glBindVertexArray(quardVao);
			glActiveTexture(GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, from);
			glActiveTexture(GL_TEXTURE1);
			glBindTexture(GL_TEXTURE_2D, uvStretchMap);
			glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
			glBindVertexArray(0);

			mFbo->unbind();
		}
	};

	class IrrMergeMesh : public redips::glMeshWrapper {
	public:
		IrrMergeMesh(const glMeshWrapper& another, redips::ShaderSource shaderSource = redips::ShaderSource()) : glMeshWrapper(another, shaderSource) {
			bindVaoAttribData(0, 1, 2, -1);
			useShader(shaderSource);

			m_shader->uniformFloat1("irrmix", IRRMIX);
		}

		void bindTexture(GLuint tex0, GLuint tex1, GLuint tex2, GLuint tex3, GLuint tex4, GLuint tex5) {
			irrmaps[0] = tex0;
			irrmaps[1] = tex1;
			irrmaps[2] = tex2;
			irrmaps[3] = tex3;
			irrmaps[4] = tex4;
			irrmaps[5] = tex5;

			m_shader->Use();
			for (int i = 0; i < 6; ++i) {
				glUniform1i(glGetUniformLocation(m_shader->Program, (std::string("irrmap") + std::to_string(i)).c_str()), i + 2);
			}
		}

		void render() {
			if (!m_shader) { std::cerr << "shader error" << std::endl; return; };
			m_shader->Use();
			for (int i = 0; i < meshCnt; i++) {
				if (meshFaceCnt[i] < 1) continue;
				
				unsigned int flags = 0;
				if (meshMtls[i]->texture_ka != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_) {
					glActiveTexture(GL_TEXTURE0);
					glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_ka]);
					glUniform1i(glGetUniformLocation(m_shader->Program, shaderAmbientTextureUniformStr), shaderAmbientTextureLocation);
					flags |= 1u;
				}
				{
					redips::float3 color = meshMtls[i]->ambient;
					glUniform3f(glGetUniformLocation(m_shader->Program, shaderAmbientColorUniformStr), color.x, color.y, color.z);
				}
				if (meshMtls[i]->texture_kd != NULL && meshFaceTypes[i] == redips::GROUP_FACE_TYPE::_withtex_) {
					glActiveTexture(GL_TEXTURE1);
					glBindTexture(GL_TEXTURE_2D, mtlTextureHandle[meshMtls[i]->texture_kd]);
					glUniform1i(glGetUniformLocation(m_shader->Program, shaderDiffuseTextureUniformStr), shaderDiffuseTextureLocation);
					flags |= 2u;
				}
				{
					redips::float3 color = meshMtls[i]->diffuse;
					glUniform3f(glGetUniformLocation(m_shader->Program, shaderDiffuseColorUniformStr), color.x, color.y, color.z);
				}

				glUniform1ui(glGetUniformLocation(m_shader->Program, shaderSurfaceTypeUniformStr), flags);

				for (int i = 0; i < 6; ++i) {
					glActiveTexture(GL_TEXTURE2 + i);
					glBindTexture(GL_TEXTURE_2D, irrmaps[i]);
				}

				glBindVertexArray(vaos[i]);
				glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
			}
		}

	private:

		GLuint irrmaps[6];

		const GLchar* shaderSurfaceTypeUniformStr = "material.flags";
		const GLchar* shaderAmbientColorUniformStr = "material.ambient";
		const GLchar* shaderDiffuseColorUniformStr = "material.diffuse";
		const GLchar* shaderAmbientTextureUniformStr = "material.ambientTexture";
		const GLchar* shaderDiffuseTextureUniformStr = "material.diffuseTexture";
		const GLuint shaderAmbientTextureLocation = 0;
		const GLuint shaderDiffuseTextureLocation = 1;
	};

public:
	IrrMergeMesh *mgmesh = nullptr;
	DepthPassMesh *dpmesh = nullptr;
	BlinnPhongMeshTBN *bpmesh = nullptr;

	SSS(int2 fbosize,
		const std::string& objpath,
		const std::string& uvStretchMapath,
		const std::string& faceUnwarpShaderPath, 
		const std::string& blurShaderPath, 
		const std::string& mergeShaderPath) : offscreen_fbo_size(fbosize){

		//setup shadow map
		fbo4shadowmap = new redips::FrameBufferObject(SHADOWMAPX, SHADOWMAPY);
		fbo4shadowmap ->setup4ShadowMap();

		//load bpmesh
		bpmesh = new redips::BlinnPhongMeshTBN(new redips::Triangles(objpath.c_str()), fbo4shadowmap->depthMap, faceUnwarpShaderPath.c_str());
		bpmesh ->uniformFloat1("irrmix", IRRMIX);

		//setup dpmesh
		dpmesh = new redips::DepthPassMesh(*bpmesh);

		//blur irrdiance maps
		blurer = new FaceBlur(fbosize, blurShaderPath, uvStretchMapath);

		//setup 1 + 5 = 6 irradiance maps ;
		fbo4faceUnWarp = new redips::FrameBufferObject(fbosize.x, fbosize.y);
		fbo4faceUnWarp ->setupColorAndDepthAttachment();
		for (int i = 0; i < 5; ++i) {
			irradiances[i].create2d(fbosize, GL_RGBA8, GL_RGBA, GL_FLOAT, NULL);
		}

		//setup final mgmesh
		mgmesh = new IrrMergeMesh(*bpmesh, mergeShaderPath.c_str());
		mgmesh ->bindTexture(fbo4faceUnWarp->colorMap, irradiances[0], irradiances[1], irradiances[2], irradiances[3], irradiances[4]);

		CHECK_GL_ERROR("SSS initialization failed");
	}
	~SSS() {
		delete fbo4shadowmap;
		delete fbo4faceUnWarp;
		delete blurer;
		delete bpmesh;
	}

	void render(bool shadowed) {
		
		if (shadowed) {
			fbo4shadowmap->bind4Writing();
			dpmesh->draw();
			fbo4shadowmap->unbind();
		}

		fbo4faceUnWarp->bind();  
		bpmesh->draw(shadowed);
		fbo4faceUnWarp->unbind();
		
		//blur
		const static double blur_gaussian_width[]{ 0.0064, 0.0484, 0.187, 0.567, 1.99, 7.41 };
		blurer->blur(fbo4faceUnWarp->colorMap, irradiances[0], (blur_gaussian_width[1]) - (blur_gaussian_width[0]));
		for (int i = 2; i < 6; ++i) {
			blurer->blur(irradiances[i - 2], irradiances[i - 1], (blur_gaussian_width[i]) - (blur_gaussian_width[i - 1]));
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
	
		//merge
		mgmesh->render();

	}
public:
	redips::int2 offscreen_fbo_size;

	//offscreen buffer
	redips::FrameBufferObject *fbo4shadowmap  = nullptr;
	redips::FrameBufferObject *fbo4faceUnWarp = nullptr;

	//flur irradiance map
	FaceBlur *blurer = nullptr;
	redips::glTexture irradiances[5];

	void set_irrmix_ratio(float ratio) {
		this->mgmesh->uniformFloat1("irrmix", ratio);
		this->bpmesh->uniformFloat1("irrmix", ratio);
	}
};