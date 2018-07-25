#pragma once
#include <map>
#include <string>
#include <sstream>
#include <Common/vec.h>
#include <Common/utils.h>

class Props{
	class Value{
	public:
		Value(){ _str = std::string(""); };
		explicit Value(std::string str):_str(str){}
		operator int(){
			std::stringstream buf(_str);
			int ret; 
			buf >> ret;
			return ret;
		}
		operator float(){
		    std::stringstream buf(_str);
			float ret;
			buf >> ret;
			return ret;
		}
		operator std::string(){ return _str; }
		operator bool(){ return _str == "true" || _str == "yes" || _str == "on" || _str == "ok"; }
	private:
		std::string _str;
	};
public:
	int lightsRetriveMode(){ 
		auto mode = entries["lightsRetriveMode"].operator std::string();
		if (mode == "off") return 0;
		if (mode == "on") return 1;
		if (mode == "load") return 2;
		return 0;
	};
	bool withVisibilities(){
		return entries["withVisibilities"].operator std::string() == "true";
	}
	std::string name(){ return entries["name"]; };
	std::string modelPath(){ return entries["model_path"]; }
	std::string otherModel_path(){ return entries["otherModel_path"]; }
	std::string goalPicturePath(){ return entries["goal_picture_path"]; };

	bool hasProjectionViewMatrix(){ return entries.count("ProjectionViewMatrix"); }
	bool hasViewMatrix(){ return entries.count("ViewMatrix"); }
	bool hasCamControl(){ return entries.count("camera_control_file"); }
	std::string CamControlFile(){ return entries["camera_control_file"]; }

	redips::Mat44f ProjectionViewMatrix(){
		std::vector<std::string> strs;
		STRING_UTIL.split(entries["ProjectionViewMatrix"], "[,]", strs);
		_RUNTIME_ASSERT_(strs.size() == 16, "assert [ProjectionViewMatrix has 16 elements] failed");
		float mat[16]; for (int i = 0; i < 16; ++i) mat[i] = Value(strs[i]);
		return mat;
	}
	redips::Mat44f ViewMatrix(){
		std::vector<std::string> strs;
		STRING_UTIL.split(entries["ViewMatrix"], "[,]", strs);
		_RUNTIME_ASSERT_(strs.size() == 16, "assert [ViewMatrix has 16 elements] failed");
		float mat[16]; for (int i = 0; i < 16; ++i) mat[i] = Value(strs[i]);
		return mat;
	}
	std::string cameraConfigFile(){
		if (!entries.count("camera_config_file")) return "";
		return entries["camera_config_file"];
	};
	int genLightsMethod(){
		if (entries.count("GenLightsMethod")){
			return Value(entries["GenLightsMethod"]);
		}
		return 0;
	}
	bool acceptControl(){
		if (!entries.count("accept_control")) return false;
		return entries["accept_control"];
	}
	bool savePicture(){ 
		if (!acceptControl()) return false;
		if (!entries.count("save_picture")) return false;
		return entries["save_picture"]; 
	}
	bool Three2One(){
		if (!entries.count("Three2One")) return false;
		return entries["Three2One"];
	}

	/*
	redips::float4 msclq(){
		std::vector<std::string> strs;
		STRING_UTIL.split(entries["msclq"], "[,]", strs);
		return redips::float4(Value(strs[0]).operator float(), Value(strs[1]).operator float(), Value(strs[2]).operator float(), Value(strs[3]).operator float());
	};
	
	redips::float3 camera_offset(){
		std::vector<std::string> strs;
		STRING_UTIL.split(entries["camera_offset"], "[,]", strs);
		_RUNTIME_ASSERT_(strs.size() == 3, "config file > camera_offset");
		return redips::float3(Value(strs[0]),Value(strs[1]),Value(strs[2]));
	}
	*/
	redips::float3 rot2(){
		if (entries.count("rot2")){
			std::vector<std::string> strs;
			STRING_UTIL.split(entries["rot2"], "[,]", strs);
			_RUNTIME_ASSERT_(strs.size() == 3, "config file > rot");
			return redips::float3(Value(strs[0]), Value(strs[1]), Value(strs[2]));
		}
		return redips::float3(0,0,0);
	}
	bool cameraHasTarget(){
		return entries.count("camera_target");
	}
	bool generateBackfaceLights(){
		if (entries.count("GEN_BACKFACE_LIGHTS")) 
			return (entries["GEN_BACKFACE_LIGHTS"].operator std::string()=="true");
		return false;
	}
	int MAX_DIST(){
		if (entries.count("MAX_DIST")) return Value(entries["MAX_DIST"]);
		return 2000;
	}
	int NORM_OFFSET(){
		if (entries.count("NORM_OFFSET")) return Value(entries["NORM_OFFSET"]);
		return 1000;
	}
	int CONSTRAINT_PIXELS_TYPE(){
		if (entries.count("CONSTRAINT_PIXELS_TYPE")) return Value(entries["CONSTRAINT_PIXELS_TYPE"]);
		return 1;
	}
	int renderMode(){
		if (entries.count("renderMode")) return Value(entries["renderMode"]);
		return 0;
	}
	redips::float3 camera_intrinsic(){
		std::vector<std::string> strs;
		if (!entries.count("camera_intrinsic"))  return redips::float3(60, 0.1, 100000);
		STRING_UTIL.split(entries["camera_intrinsic"], "[,]", strs);
		float height = Value(strs[1]);
		float focaLen = Value(strs[0]);
		return redips::float3(ANGLE(atan(height*0.5f / focaLen)) * 2, Value(strs[2]), Value(strs[3]));
	}
	redips::int3 BackGroundColor(){
		if (!entries.count("BackGroundColor")) return redips::int3();
		std::vector<std::string> strs;
		STRING_UTIL.split(entries["BackGroundColor"], "[,]", strs);
		return redips::int3(Value(strs[0]), Value(strs[1]), Value(strs[2]));
	}
	bool hasEuler(){
		return entries.count("camera_euler");
	}
	redips::float3 camera_euler(){
		std::vector<std::string> strs;
		STRING_UTIL.split(entries["camera_euler"], "[,]", strs);
		return redips::float3(Value(strs[0]), Value(strs[1]), Value(strs[2]));
	}
	redips::float3 camera_target(){
		std::vector<std::string> strs;
		STRING_UTIL.split(entries["camera_target"], "[,]", strs);
		return redips::float3(Value(strs[0]), Value(strs[1]), Value(strs[2]));
	}
	redips::float3 model_position(){
		std::vector<std::string> strs;
		STRING_UTIL.split(entries["model_position"], "[,]", strs);
		return redips::float3(Value(strs[0]), Value(strs[1]), Value(strs[2]));
	}
	redips::float3 model_scale(){
		std::vector<std::string> strs;
		STRING_UTIL.split(entries["model_scale"], "[,]", strs);
		return redips::float3(Value(strs[0]), Value(strs[1]), Value(strs[2]));
	}
	redips::float3 model_rotation(){
		std::vector<std::string> strs;
		STRING_UTIL.split(entries["model_rotation"], "[,]", strs);
		return redips::float3(Value(strs[0]), Value(strs[1]), Value(strs[2]));
	}
	~Props(){ entries.clear(); }
	
	
	static Props* getInstance(const char* path = ""){
		if (strlen(path)){
			delete instance;
			instance = new Props(path);
		}
		return instance;
	}

private:
	static Props* instance;
	Props(const char* filepath){
		std::ifstream fin(filepath);
		_RUNTIME_ASSERT_(fin.is_open(), "config file opened");
		std::string line;
		std::vector<std::string> strs;
		while (std::getline(fin, line)){
			if (line.size() == 0) continue;
			if (STRING_UTIL.split(line, "= \t", strs) == 2)
				entries[strs[0]] = Value(strs[1]);
		}
		fin.close();
		std::cout << "[prop-manager] : load properties file finish" << std::endl;
	}
	std::map<std::string, Value> entries;
};