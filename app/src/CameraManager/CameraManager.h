#pragma once

#include "./CameraGL.h"
#include "../realsnese//RealsenseDevice.h"
#include "../realsnese/JsonRealsenseDevice.h"
#include <functional>

#define CamIterator std::vector<CameraGL>::iterator&

class CameraManager {

	rs2::context ctx;
	std::vector<CameraGL> realsenses;
	std::set<std::string> serials;

	void addNetworkDevice(std::string url);
	void addJsonDevice(std::string serial);
	void addDevice(std::string serial);
	void removeDevice(CamIterator device);

public :

	CameraManager();
	void destory();
	void deleteIdleCam();

	void addCameraUI();
	void setExtrinsicsUI();

	size_t size();
	void getAllDevice(std::function<void(CamIterator)> callback);
	void getAllDevice(std::function<void(CamIterator, std::vector<CameraGL>&)> callback);
	void getProjectTextureDevice(std::function<void(CamIterator)> callback);
	void getFowardDepthWarppingDevice(std::function<void(CamIterator)> callback);

	void updateProjectTextureWeight(glm::mat4 vmodelMat);
};