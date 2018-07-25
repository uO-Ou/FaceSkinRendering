#pragma once

#include <OpenglWrappers/glMeshWrapper.h>

class PLYMesh : public redips::glMeshWrapper {
	GLuint color_vbo = 0;
	redips::glTexture reflectance_texture;
public:
	PLYMesh(redips::int2 winsize,
		    const redips::Triangles* model, 
		    const char* vcolor_file, 
			const char* refimgpath ,
			redips::ShaderSource shaderSource = redips::ShaderSource())
		: glMeshWrapper(model, shaderSource) {
		bindVaoAttribData(0, 1, 2);

		redips::FImage reflimg(refimgpath);
		reflectance_texture.create2d(&reflimg, false);

		int face_cnt = this->model_ptr()->faceCnt;
		auto vcolors = new redips::float3[face_cnt * 3];
		std::ifstream fin(vcolor_file);
		for (int i = 0; i < face_cnt * 3; ++i) {
			fin >> vcolors[i].x >> vcolors[i].y >> vcolors[i].z;
		}
		fin.close();

		glGenBuffers(1, &color_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
		glBufferData(GL_ARRAY_BUFFER, face_cnt * 9 * sizeof(float), vcolors, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		delete vcolors;

		//assert there are only one mesh
		for (int i = 0; i < meshCnt; i++) {
			glBindVertexArray(vaos[i]);
			glBindBuffer(GL_ARRAY_BUFFER, color_vbo);
			glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, 0, 0);
			glEnableVertexAttribArray(3);
		}

	};
	PLYMesh(const glMeshWrapper& another, redips::ShaderSource shaderSource = redips::ShaderSource()) = delete;

	void draw() {
		if (!m_shader) { std::cerr << "shader error" << std::endl; return; };
		m_shader->Use();
		for (int i = 0; i < meshCnt; i++) {
			glBindVertexArray(vaos[i]);
			glActiveTexture(GL_TEXTURE0 + 0);
			glBindTexture(GL_TEXTURE_2D, reflectance_texture);
			glDrawArrays(GL_TRIANGLES, 0, 3 * meshFaceCnt[i]);
		}
	}

	GLuint reftex() const { return reflectance_texture; }
};
