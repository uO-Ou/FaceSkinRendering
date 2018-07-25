#pragma once
#include <cstdio>
#include <limits>
#include <vector>
#include <limits>
#include <sutil.h>
#include <nvTime.h>
#include <string.h>
#include <nvModel.h>
#include <GL/glew.h>
#include <Common/vec.h>
#include <Cameras/phc.h>
#include <framebufferObject.h>
#include <optixu/optixpp_namespace.h>
#include <Common/fImage.h>
#include <OpenglWrappers/glHelper.h>
#include <OpenglWrappers/Effects/GBuffer.h>

#define CHECK_GL_ERROR(s) {if(glGetError()!=GL_NO_ERROR){printf("glError %s\n",(s));exit(-1);};}

//#define OUTPUT_UCHAR4

class OptixRenderer{
	redips::glImageRender* render = nullptr;
public:
	redips::glTexture output_texture;
	OptixRenderer(int width, int height, 
		          const char* model_path,const redips::Mat44f& transform,
		          GLuint lightsVbo, int lightsCnt, 
				  redips::GBuffer& gbuffer) : scr_width(width), scr_height(height), transmat(transform), lightCnt(lightsCnt), lightsAB(lightsVbo){
		render = new redips::glImageRender(redips::int2(width,height),32);
		CHECK_GL_ERROR("bad optix-renderer enviroment.");
		uintCntPerPixel = (lightCnt - 1) / 32 + 1;

		normalTex = gbuffer.getTexture(redips::GBuffer::_GL_GBUFFER_TEXTURE_TYPE_::_normal_);
		positionTex = gbuffer.getTexture(redips::GBuffer::_GL_GBUFFER_TEXTURE_TYPE_::_position_);
		diffuseTex = gbuffer.getTexture(redips::GBuffer::_GL_GBUFFER_TEXTURE_TYPE_::_albedo_spec_);
		specularTex = gbuffer.getTexture(redips::GBuffer::_GL_GBUFFER_TEXTURE_TYPE_::_albedo_spec_);

		output_texture.create2d(redips::int2(width, height), GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);

		if (!initialize(model_path)){
			puts("[optix] : initialize failed");
		}
		else {
			puts("[optix] : initialize success");
			initialized = true;
		}
		imgbuf_uchar4 = new unsigned char[width*height*4];
		imgbuf_float4 = new float[width*height * 4];
	};

	~OptixRenderer(){
		if (modelVB) glDeleteBuffers(1, &modelVB);
		if (modelIB) glDeleteBuffers(1, &modelIB);
		if (smAB)    glDeleteBuffers(1, &smAB);

		delete imgbuf_uchar4;
		delete imgbuf_float4;
	};

	/*
		option: 0, uchar4 1, float4
	*/
	void fly(redips::float3 campos){
		if (!initialized) return;

#ifdef OUTPUT_UCHAR4
		int option = 0;
#else
		int option = 1;
#endif

		rtContext["option"]->setInt(option);
		rtContext["camera_pos"]->set3fv(&campos[0]);

		redips::float3 bgcolor(0, 0, 0);
		rtContext["bgcolor"]->set3fv(&bgcolor.x);
		
		rtContext->launch(0, scr_width, scr_height);

#ifdef OUTPUT_UCHAR4
		unsigned char* ptr = (unsigned char*)(rtOutputBuffer_uchar4->map());
		memcpy(imgbuf_uchar4, ptr, sizeof(unsigned char) * 4 * scr_width*scr_height);
		rtOutputBuffer_uchar4->unmap();
		render->render(imgbuf_uchar4);
		//redips::FImage::saveImage(imgbuf_uchar4,scr_width,scr_height,4,"fuck.bmp");
#else
		auto ptr = (float*) (rtOutputBuffer_float4->map());
		memcpy(imgbuf_float4, ptr, sizeof(float)* 4 * scr_width*scr_height);
		rtOutputBuffer_float4->unmap();
		output_texture.update(ptr);
#endif

	}

	void setCamera(redips::PhC& phc){
		m_phc = phc;

		rtContext["nearp"]->setFloat(m_phc.nearp);
		rtContext["camera_x"]->set3fv(&m_phc.cameraX.x);
		rtContext["camera_y"]->set3fv(&m_phc.cameraY.x);
		rtContext["camera_z"]->set3fv(&m_phc.cameraZ.x);
		rtContext["canvaSize"]->set2fv(&m_phc.canvaSize.x);
	}

	void setRoughness(float roughness){
		rtContext["face_roughness"]->setFloat(roughness);
	}

	void setReflectanceTexture(GLuint tid){
		bindTexSampler(tid, reflectanceSampler,"reflectance_texture");
	}

	void setWinsize(int width,int height){
		rtContext["WinWidth"]->setInt(width);
		rtContext["WinHeight"]->setInt(height);
	}

private:
	redips::PhC m_phc;		//for ray-casting

	int lightCnt = 0;
	unsigned int zero = 0u;
	int uintCntPerPixel = 0;
	bool initialized = false;
	float scene_epsilon = 2.0f;
	GLuint scr_width, scr_height;
	float* imgbuf_float4 = nullptr;
	unsigned char* imgbuf_uchar4 = nullptr;

	//模型相关
	nv::Model* model;
	redips::Mat44f transmat;
	GLuint modelVB = 0, modelIB = 0;

	//gbuffer输入
	GLuint positionTex = 0, normalTex = 0;
	GLuint diffuseTex = 0, specularTex = 0;

	//gl buffer
	GLuint lightsAB = 0, smAB = 0;

	//optix
	optix::Context        rtContext;
	optix::Buffer         rtLightsBuffer;
	
	optix::TextureSampler reflectanceSampler;

	optix::TextureSampler postexSampler, normtexSampler, diffuseSampler, specularSampler;
	int rtSMBufferWidth;
	optix::Buffer         rtOutputBuffer_float4;
	optix::Buffer		  rtOutputBuffer_uchar4,rtSMBuffer;

	std::string ptxpath(const char* filename){ return std::string("./ptx/") + std::string(filename) + ".ptx"; }

	bool load(const char* model_path){
		model = new nv::Model();
		puts("[optix] : loading nvmodel ... ");
		if (!model->loadModelFromFile(model_path)) {
			std::cerr << "[optix] : ! Unable to load model '" << model_path << "'" << std::endl;
			return false;
		}
		model->removeDegeneratePrims();
		model->clearTexCoords();
		model->clearTangents();
		model->clearColors();
		model->computeNormals();
		model->compileModel();

		glGenBuffers(1, &modelVB);
		glBindBuffer(GL_ARRAY_BUFFER, modelVB);
		glBufferData(GL_ARRAY_BUFFER, model->getCompiledVertexCount()*model->getCompiledVertexSize()*sizeof(float), model->getCompiledVertices(), GL_STATIC_READ);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glGenBuffers(1, &modelIB);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, modelIB);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, model->getCompiledIndexCount()*sizeof(int), model->getCompiledIndices(), GL_STATIC_READ);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
		return true;
	}

	void bindTexSampler(GLuint texid, optix::TextureSampler& sampler, const char* target){
		sampler = rtContext->createTextureSamplerFromGLImage(texid, RT_TARGET_GL_TEXTURE_2D);
		sampler->setWrapMode(0, RT_WRAP_CLAMP_TO_EDGE);
		sampler->setWrapMode(1, RT_WRAP_CLAMP_TO_EDGE);
		sampler->setIndexingMode(RT_TEXTURE_INDEX_ARRAY_INDEX);
		sampler->setReadMode(RT_TEXTURE_READ_ELEMENT_TYPE);
		sampler->setMaxAnisotropy(1.0f);
		sampler->setFilteringModes(RT_FILTER_NEAREST, RT_FILTER_NEAREST, RT_FILTER_NONE);
		rtContext[target]->setTextureSampler(sampler);
	}

	bool initialize(const char* model_path){
		//load model
		if (!load(model_path)){
			puts("[optix] : load model failed"); return false;
		}
		else puts("[optix] : load model finish");

		//init optix
		try {
			rtContext = optix::Context::create();
			rtContext ->setRayTypeCount(1);
			rtContext ->setEntryPointCount(1);

			// Limit number of devices to 1 as this is faster for this particular sample.
			std::vector<int> enabled_devices = rtContext->getEnabledDevices();
			rtContext->setDevices(enabled_devices.begin(), enabled_devices.begin() + 1);
			
			rtContext["shadow_ray_type"]->setUint(0u);
			rtContext["light_cnt"]->setUint(lightCnt);
			rtContext["scr_width"]->setUint(scr_width);
			rtContext["scene_epsilon"]->setFloat(scene_epsilon);

			//texture sampler   //input
			bindTexSampler(normalTex, normtexSampler, "normal_texture");
			bindTexSampler(diffuseTex, diffuseSampler, "diffuse_texture");
			bindTexSampler(positionTex, postexSampler, "position_texture");
			bindTexSampler(specularTex, specularSampler, "specular_texture");

			//lights buffer     //input
			rtLightsBuffer = rtContext->createBufferFromGLBO(RT_BUFFER_INPUT, lightsAB);
			rtLightsBuffer ->setSize(lightCnt * 9);
			rtLightsBuffer ->setFormat(RT_FORMAT_FLOAT);
			rtContext["lights_buffer"]->setBuffer(rtLightsBuffer);

			//sm result         //output
			rtSMBufferWidth = scr_width*scr_height*((lightCnt - 1) / 32 + 1);

			glGenBuffers(1, &smAB);
			glBindBuffer(GL_ARRAY_BUFFER, smAB);
			glBufferData(GL_ARRAY_BUFFER, sizeof(unsigned int)*rtSMBufferWidth, nullptr, GL_DYNAMIC_COPY);
			glBindBuffer(GL_ARRAY_BUFFER, 0);

			rtSMBuffer = rtContext->createBufferFromGLBO(RT_BUFFER_INPUT_OUTPUT, smAB);
			rtSMBuffer ->setSize(rtSMBufferWidth);
			rtSMBuffer ->setFormat(RT_FORMAT_UNSIGNED_INT);
			rtContext["sm_buffer"]->setBuffer(rtSMBuffer);

			//render result     //output

			rtOutputBuffer_uchar4 = rtContext->createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_UNSIGNED_BYTE4, scr_width, scr_height);
			rtContext["output_buffer_uchar4"]->setBuffer(rtOutputBuffer_uchar4);
			rtOutputBuffer_float4 = rtContext->createBuffer(RT_BUFFER_OUTPUT, RT_FORMAT_FLOAT4, scr_width, scr_height);
			rtContext["output_buffer_float4"]->setBuffer(rtOutputBuffer_float4);

			//program
			rtContext->setRayGenerationProgram(0, rtContext->createProgramFromPTXFile(ptxpath("ray"), "shading"));                  //发射光线的函数
			rtContext->setExceptionProgram(0, rtContext->createProgramFromPTXFile(ptxpath("ray"), "exception"));                    //异常函数

			optix::Material opaque = rtContext->createMaterial();                                                                                                       //创建一个材质
			opaque->setAnyHitProgram(0, rtContext->createProgramFromPTXFile(ptxpath("ray"), "any_hit_shadow"));                     //hit 函数

			//rt model
			optix::Geometry rtModel = rtContext->createGeometry();
			rtModel->setPrimitiveCount(model->getCompiledIndexCount() / 3);
			rtModel->setIntersectionProgram(rtContext->createProgramFromPTXFile(ptxpath("mesh"), "mesh_intersect"));                //判断相交的函数
			rtModel->setBoundingBoxProgram(rtContext->createProgramFromPTXFile(ptxpath("mesh"), "mesh_bounds"));                    //用于创建包围盒

			int num_vertices = model->getCompiledVertexCount();
			optix::Buffer vertex_buffer = rtContext->createBufferFromGLBO(RT_BUFFER_INPUT, modelVB);
			vertex_buffer->setFormat(RT_FORMAT_USER);
			vertex_buffer->setElementSize(3 * 2 * sizeof(float));
			vertex_buffer->setSize(num_vertices);
			rtModel["vertex_buffer"]->setBuffer(vertex_buffer);

			optix::Buffer index_buffer = rtContext->createBufferFromGLBO(RT_BUFFER_INPUT, modelIB);
			index_buffer->setFormat(RT_FORMAT_INT3);
			index_buffer->setSize(model->getCompiledIndexCount() / 3);
			rtModel["index_buffer"]->setBuffer(index_buffer);

			//setup geometry instance
			optix::GeometryInstance instance = rtContext->createGeometryInstance();
			instance->setMaterialCount(1);
			instance->setMaterial(0, opaque);
			instance->setGeometry(rtModel);

			optix::GeometryGroup geometrygroup = rtContext->createGeometryGroup();
			geometrygroup->setChildCount(1);
			geometrygroup->setChild(0, instance);
			geometrygroup->setAcceleration(rtContext->createAcceleration("Bvh", "Bvh"));

			optix::Transform transformNode = rtContext->createTransform();
			transformNode->setMatrix(0, transmat.ptr(), 0);
			transformNode->setChild(geometrygroup);

			optix::Group topGroup = rtContext->createGroup();
			topGroup->setChildCount(1);
			topGroup->setChild(0,transformNode);
			topGroup->setAcceleration(rtContext->createAcceleration("Bvh", "Bvh"));
			rtContext["shadow_casters"]->set(topGroup);

			rtContext->setStackSize(2048);
			rtContext->validate();
		}
		catch (optix::Exception& e) {
			sutilReportError(e.getErrorString().c_str());
			return false;
		}
		return true;
	}
};

