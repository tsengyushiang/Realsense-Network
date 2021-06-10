#pragma once

#include <iomanip>
#include <iostream>
#include <fstream> 

#include "json/json.hpp"
using json = nlohmann::json;
namespace Jsonformat {
	struct CamPose {
		std::string id;
		std::vector<float> extrinsic;
	};

	template<typename T>
	void to_json(json& j, const T& p);
	template<typename T>
	void from_json(const json& j, T& p);
}

class JsonUtils {

public:
	static void saveRealsenseJson(
		std::string filename, 
		int width,int height,
		float fx,float fy,float ppx,float py,
		float depthscale, const unsigned short* depthmap,const unsigned char* colormap,
		std::vector<float> extrinsic4x4
	);static void saveRealsenseJson(
		std::string filename, 
		int width,int height,
		float fx,float fy,float ppx,float py,
		float depthscale, float* depthmap,unsigned char* colormap,
		std::vector<float> extrinsic4x4
	);

	static std::string cameraPoseFilename;
	static void saveCameraPoses(
		std::vector<Jsonformat::CamPose>& poses
	);
	static void loadCameraPoses(
		std::vector<Jsonformat::CamPose>& poses
	);
};