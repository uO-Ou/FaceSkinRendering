#pragma once
#include <random>
#include <string>
#include <GL/glew.h>
#include <Geometry/geometry.h>
/************************************************************************/
/*                           LightsGenerator                            */
//X[-1,1] Y[-1,1] Z[0,1]
/************************************************************************/

class LightsGenerator{
	std::default_random_engine generator;
	std::uniform_real_distribution<double> uniform_floats;

	std::string basepath;

	double gap;
	double patch_size;
	double valid_side_length;
	int lights_number_per_side;

	redips::float3 light_color;
	std::vector<redips::float3> centers;
	std::vector<std::vector<redips::Light >> illuminators_store;

public:
	int light_number = 0;
	GLuint lights_vbo = 0;
	std::vector<redips::Light> illuminators;

public:
	LightsGenerator(double patch_size, int lights_number_per_side, std::string basepath="./lights") : lights_number_per_side(lights_number_per_side), patch_size(patch_size),basepath(basepath){
		uniform_floats = std::uniform_real_distribution<double>(0, 1);

		valid_side_length = 1 - patch_size * 0.5;

		gap = patch_size / (lights_number_per_side - 1);
		
		light_color = redips::float3(1.0 / (lights_number_per_side*lights_number_per_side));

		light_number = lights_number_per_side * lights_number_per_side;

		glGenBuffers(1, &lights_vbo);
		glBindBuffer(GL_ARRAY_BUFFER, lights_vbo);
		glBufferData(GL_ARRAY_BUFFER, sizeof(float)* 9 * light_number, nullptr, GL_DYNAMIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}
	
	GLuint update_lights(){
		illuminators.clear();
		
		GenPatch(illuminators);

		glBindBuffer(GL_ARRAY_BUFFER, lights_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)* 9 * illuminators.size(), &illuminators[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		
		return lights_vbo;
	}

	void load_all_lights(int n){
		centers.resize(n);
		illuminators_store.resize(n);
		for (int i = 0; i < n; ++i){
			std::ifstream fin(basepath + std::string("/") + std::to_string(i) + ".txt");
			int litcnt;
			fin >> litcnt >> centers[i].x >> centers[i].y >> centers[i].z;
			
			_RUNTIME_ASSERT_(litcnt == lights_number_per_side*lights_number_per_side, "litcnt == lights_number_per_side*lights_number_per_side");

			auto& curlits = illuminators_store[i];
			curlits.resize(litcnt);
			for (int j = 0; j < litcnt; ++j){
				redips::Light lit;
				fin >> lit.position.x >> lit.position.y >> lit.position.z;
				fin >> lit.intensity.x >> lit.intensity.y >> lit.intensity.z;
				fin >> lit.attenuation_coef.x >> lit.attenuation_coef.y >> lit.attenuation_coef.z;
				curlits[j] = lit;
			}

			fin.close();
		}
	}

	void generate_n_patch(int n){
		std::vector<redips::Light> lights;
		std::vector<redips::double3> centers;
		for (int i = 0; i < n; ++i){
			lights.clear();
			auto center = GenPatch(lights);
			centers.push_back(center);
			std::ofstream fout(basepath+std::string("/")+std::to_string(i)+".txt");
			fout << lights.size() << std::endl;
			fout << center.x << " " << center.y << " " << center.z << std::endl;
			for (auto& lit : lights){
				fout << lit.position.x << " " << lit.position.y << " " << lit.position.z << " " ;
				fout << lit.intensity.x << " " << lit.intensity.y << " " << lit.intensity.z << " ";
				fout << lit.attenuation_coef.x << " " << lit.attenuation_coef.y << " " << lit.attenuation_coef.z << "\n";
			}
			fout.close();
		}
		std::ofstream fout(basepath + "/summery.txt");
		for (auto & center : centers){
			fout << center.x << " " << center.y << " " << center.z << std::endl;
		}
		fout.close();
	}

	void switch_lights(int id, redips::float3& center){
		_RUNTIME_ASSERT_(id < illuminators_store.size(), "id < illuminators_store.size()");

		auto& curlits = illuminators_store[id];

		glBindBuffer(GL_ARRAY_BUFFER, lights_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float) * 9 * curlits.size(), &curlits[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		center = centers[id];
	}
private:
	redips::double3 GenPatch(std::vector<redips::Light>& lights){
		using namespace redips;

		const int distance = 10000;

		double x, y, z;
		if (uniform_floats(generator) < 0.33333){ //gen_front_face[z=1]
			z = 1;
			
			x = (uniform_floats(generator) * 2 - 1)*valid_side_length;
			y = (uniform_floats(generator) * 2 - 1)*valid_side_length;

			double basex = x - patch_size*0.5;
			double basey = y - patch_size*0.5;
			for (int i = 0; i < lights_number_per_side; ++i) for (int j = 0; j < lights_number_per_side; ++j){
				auto light_pos = redips::float3(basex + gap*i, basey + gap*j, z);
				lights.push_back(redips::Light(light_pos.unit()*distance, light_color, float3(1, 0, 0)));
			}
		}
		else{
			if (uniform_floats(generator) < 0.4){ //gen_side_face[x=1/-1]
				x = uniform_floats(generator) < 0.5 ? -1 : 1;
				
				y = (uniform_floats(generator) * 2 - 1)*valid_side_length;
				z = uniform_floats(generator) * valid_side_length;

				double basey = y - patch_size*0.5;
				double basez = z - patch_size*0.5;
				for (int i = 0; i < lights_number_per_side; ++i) for (int j = 0; j < lights_number_per_side; ++j){
					auto light_pos = redips::float3(x, basey + gap*i, basez + gap*j);
					lights.push_back(redips::Light(light_pos.unit()*distance, light_color, float3(1, 0, 0)));
				}
			}
			else{                                //gen_updown_face[y=1/-1]
				y = uniform_floats(generator) < 0.5 ? -1 : 1;

				x = (uniform_floats(generator) * 2 - 1)*valid_side_length;
				z = uniform_floats(generator) * valid_side_length;

				double basex = x - patch_size*0.5;
				double basez = z - patch_size*0.5;
				for (int i = 0; i < lights_number_per_side; ++i) for (int j = 0; j < lights_number_per_side; ++j){
					auto light_pos = redips::float3(basex + gap*i, y, basez + gap*j);
					lights.push_back(redips::Light(light_pos.unit()*distance, light_color, float3(1, 0, 0)));
				}
			}
		}

		return double3(x, y, z);
	}

	GLuint load_lights(const std::string& path, redips::float3& center){
		std::ifstream fin(path);
		int litcnt;
		fin >> litcnt >> center.x >> center.y >> center.z;

		_RUNTIME_ASSERT_(litcnt == lights_number_per_side*lights_number_per_side, "litcnt == lights_number_per_side*lights_number_per_side");

		illuminators.resize(litcnt);
		for (int i = 0; i < litcnt; ++i){
			redips::Light lit;
			fin >> lit.position.x >> lit.position.y >> lit.position.z;
			fin >> lit.intensity.x >> lit.intensity.y >> lit.intensity.z;
			fin >> lit.attenuation_coef.x >> lit.attenuation_coef.y >> lit.attenuation_coef.z;
			illuminators[i] = lit;
		}
		fin.close();

		glBindBuffer(GL_ARRAY_BUFFER, lights_vbo);
		glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(float)* 9 * illuminators.size(), &illuminators[0]);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		return lights_vbo;
	}
};