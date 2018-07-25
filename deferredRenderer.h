#pragma once
#include <OpenglWrappers/Effects/GBuffer.h>

class DeferredRenderer : public redips::GBuffer{
public:
	DeferredRenderer(int width,int height,const char* shaderpath):GBuffer(width,height){
		useShader(redips::ShaderSource(shaderpath));
	}
	~DeferredRenderer(){};
	void drawQuard(){
		deferredShader.Use();
		bindTexture(0,_GL_GBUFFER_TEXTURE_TYPE_::_normal_,"normalTexture");
		bindTexture(1, _GL_GBUFFER_TEXTURE_TYPE_::_position_,"positionTexture");
		bindTexture(2, _GL_GBUFFER_TEXTURE_TYPE_::_albedo_spec_,"materialTexture");
		redips::GBuffer::drawQuard();
	}
};