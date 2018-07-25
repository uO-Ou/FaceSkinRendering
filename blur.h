#include <GL/glew.h>
#include <OpenglWrappers/glMeshWrapper.h>
#include <OpenglWrappers/Effects/GBuffer.h>

#include <map>
#include <string>

using namespace redips;

class FaceBlur{
	Shader shader;
	GLuint stentid = 0;
	redips::int2 imgsize;
	redips::GBuffer* gbuffer;
	redips::glTexture SwapTexture;
	GLuint quardVao = 0, quardVbo = 0;

public:
	FaceBlur(GLuint stencilTid, redips::int2 imgsize, std::string shader_path) :imgsize(imgsize), stentid(stencilTid){
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
		shader.uniformInt1("stenc_texture", 1);

		gbuffer = new redips::GBuffer(imgsize.x, imgsize.y);

		SwapTexture.create2d(imgsize, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
	}

	~FaceBlur(){
		glDeleteBuffers(1, &quardVbo);
		glDeleteVertexArrays(1, &quardVao);

		delete gbuffer;
	}

	void blur(GLuint from, GLuint to, float gaussian_width){
		shader.Use();
		shader.uniformFloat1("GaussWidth", gaussian_width);
		
		blur_(from,SwapTexture,1);
		blur_(SwapTexture, to, 0);
	}

private:

	void useShader(redips::ShaderSource& source){
		if (source.sourceType == ShaderSource::SourceType::_exists_program_){
			shader = Shader(source.value.program);
		}
		else if (source.sourceType == ShaderSource::SourceType::_from_file_){
			shader = Shader((std::string(source.value.path) + _vertex_shader_file_suffix_).c_str(), (std::string(source.value.path) + _fragment_shader_file_suffix_).c_str());
		}
	}

	void GbufferToTexture(GLuint to){
		gbuffer->bind4Reading();
		glBindTexture(GL_TEXTURE_2D, to);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, imgsize.x, imgsize.y);
		glBindTexture(GL_TEXTURE_2D, 0);
		gbuffer->unbind();
	}

	void blur_(GLuint from, GLuint to, int UorV){
		gbuffer->bind4Writing();

		shader.uniformInt1("UorV", UorV);
		if (UorV) shader.uniformFloat1("ImgSize", imgsize.x);
		else shader.uniformFloat1("ImgSize", imgsize.y);

		glBindVertexArray(quardVao);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, from);
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, stentid);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);

		gbuffer->unbind();
		GbufferToTexture(to);
	}
};