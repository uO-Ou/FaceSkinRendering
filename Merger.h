#pragma once
#include <map>
#include <GL/glew.h>
#include <OpenglWrappers/glMeshWrapper.h>
using namespace redips;

class Merger{
	GLuint quardVao = 0, quardVbo = 0;

	Shader shader;
	void useShader(redips::ShaderSource& source){
		if (source.sourceType == ShaderSource::SourceType::_exists_program_){
			shader = Shader(source.value.program);
		}
		else if (source.sourceType == ShaderSource::SourceType::_from_file_){
			shader = Shader((std::string(source.value.path) + _vertex_shader_file_suffix_).c_str(), (std::string(source.value.path) + _fragment_shader_file_suffix_).c_str());
		}
	}

	int2 winsize;
	std::map<int, GLuint> binded_textures;
public:
	redips::glTexture textures[5];

public:
	Merger(redips::int2 winsize, std::string shader_path):winsize(winsize){
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
		for (int i = 0; i < 5; ++i){
			textures[i].create2d(winsize, GL_RGBA32F, GL_RGBA, GL_FLOAT, nullptr);
		}
	}
	~Merger(){
		glDeleteBuffers(1, &quardVbo);
		glDeleteVertexArrays(1, &quardVao);
	}

	void render(){
		glDisable(GL_DEPTH_TEST);
		shader.Use();
		glBindVertexArray(quardVao);
		for (const auto& tex : binded_textures){
			glActiveTexture(tex.first + GL_TEXTURE0);
			glBindTexture(GL_TEXTURE_2D, tex.second);
		}
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
		glEnable(GL_DEPTH_TEST);
	}

	void bindTexture(int location, GLuint texid, const char* uniform_str){
		shader.uniformInt1(uniform_str, location);
		binded_textures[location] = texid;
	}
};