
#include "src/imgui/ImguiOpeGL3App.h"
#include "src/realsnese//RealsenseDevice.h"
#include "src/opencv/opecv-utils.h"
#include "src/pcl/examples-pcl.h"
#include <ctime>

class CorrespondPointCollector {

public :
	GLuint srcVao, srcVbo;
	GLuint trgVao, trgVbo;

	RealsenseDevice* sourcecam;
	RealsenseDevice* targetcam;
	int vaildCount = 0;
	int size;
	float pushThresholdmin = 0.1f;

	std::vector<glm::vec3> srcPoint;
	std::vector<glm::vec3> dstPoint;
	float* source;
	float* target;
	float* result;
	CorrespondPointCollector(RealsenseDevice* srcCam, RealsenseDevice* trgCam,int count = 10,float threshold=0.2f) {
		sourcecam = srcCam;
		targetcam = trgCam;
		size = count;
		pushThresholdmin = threshold;
		source = (float*)calloc(size * 3 * 2, sizeof(float));
		target = (float*)calloc(size * 3 * 2, sizeof(float));
		result = (float*)calloc(size * 3 * 2, sizeof(float));
		glGenVertexArrays(1, &srcVao);
		glGenBuffers(1, &srcVbo);
		glGenVertexArrays(1, &trgVao);
		glGenBuffers(1, &trgVbo);

		sourcecam->opencvImshow = true;
		targetcam->opencvImshow = true;
	}
	~CorrespondPointCollector() {
		free(source);
		free(target);
		free(result);
		glDeleteVertexArrays(1, &srcVao);
		glDeleteBuffers(1, &srcVbo);
		glDeleteVertexArrays(1, &trgVao);
		glDeleteBuffers(1, &trgVbo);

		sourcecam->opencvImshow = false;
		targetcam->opencvImshow = false;
	}

	void render(glm::mat4 mvp,GLuint shader_program) {
		ImguiOpeGL3App::setPointsVAO(srcVao, srcVao, source, size);
		ImguiOpeGL3App::setPointsVAO(trgVao,trgVbo, target, size);

		ImguiOpeGL3App::render(mvp, 10, shader_program, srcVao, size, GL_POINTS);
		ImguiOpeGL3App::render(mvp, 10, shader_program,trgVao, size, GL_POINTS);
	}

	bool pushCorrepondPoint(glm::vec3 src, glm::vec3 trg) {

		int index = vaildCount;
		if (index != 0) {
			// check threshold
			glm::vec3 p;
			int previousIndex = index - 1;

			for (auto p : srcPoint) {
				auto d = glm::length(p - src);
				if (d < pushThresholdmin) {
					return false;
				}
			}

			for (auto p : dstPoint) {
				auto d = glm::length(p - trg);
				if (d < pushThresholdmin) {
					return false;
				}
			}
		}

		srcPoint.push_back(src);
		dstPoint.push_back(trg);

		source[index*6+0] = src.x;
		source[index*6+1] = src.y;
		source[index*6+2] = src.z;
		source[index*6+3]=1.0;
		source[index*6+4]=0.0;
		source[index*6+5]=0.0;
		
		target[index*6+0] = trg.x;
		target[index*6+1] = trg.y;
		target[index*6+2] = trg.z;
		target[index*6+3]=0.0;
		target[index*6+4]=1.0;
		target[index*6+5]=0.0;
		
		vaildCount++;
		
		return true;
	}

	glm::mat4 calibrate() {
		glm::mat4 transform = pcl_pointset_rigid_calibrate(size, srcPoint, dstPoint);
		sourcecam->modelMat = transform * sourcecam->modelMat;
		sourcecam->calibrated = true;
	}
};

typedef struct calibrateResult {
	std::vector<glm::vec3> points;
	glm::mat4 calibMat;
	// pointcloud datas {x1,y1,z1,r1,g1,b1,...}	
	bool success;
}CalibrateResult;

class RealsenseGL {

public :
	bool use2createMesh = true;
	bool ready2Delete = false;
	RealsenseDevice* camera;
	// pointcloud datas {x1,y1,z1,r1,g1,b1,...}	
	GLuint vao, vbo, image;

	//helper 
	GLuint camIconVao, camIconVbo;

	RealsenseGL() {
		glGenVertexArrays(1, &vao);
		glGenBuffers(1, &vbo);
		glGenTextures(1, &image);
		glGenVertexArrays(1, &camIconVao);
		glGenBuffers(1, &camIconVbo);
	}
	~RealsenseGL() {
		glDeleteVertexArrays(1, &vao);
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &camIconVao);
		glDeleteBuffers(1, &camIconVbo);
		glDeleteTextures(1, &image);
	}
};

class PointcloudApp :public ImguiOpeGL3App {
	GLuint shader_program;
	GLuint axisVao, axisVbo;
	
	// mesh reconstruct 
	GLuint meshVao, meshVbo, meshibo;
	GLfloat* vertices = nullptr;
	int verticesCount = 0;
	unsigned int *indices = nullptr;
	int indicesCount = 0;
	bool wireframe = false;
	bool renderMesh = false;
	long time = 0;
	float searchRadius = 0.1;
	int maximumNearestNeighbors = 30;
	float maximumSurfaceAngle = M_PI / 4;

	int pointcloudDensity = 1;

	int width = 640;
	int height = 480;

	rs2::context ctx;
	std::vector<RealsenseGL> realsenses;
	std::set<std::string> serials;

	float t,pointsize=0.1f;

	CorrespondPointCollector* calibrator=nullptr;
	int collectPointCout = 15;
	float collectthreshold = 0.1f;

public:
	PointcloudApp():ImguiOpeGL3App(){
		serials = RealsenseDevice::getAllDeviceSerialNumber(ctx);
	}
	~PointcloudApp() {
		for (auto device = realsenses.begin(); device != realsenses.end(); device++) {
			removeDevice(device);
		}
		glDeleteVertexArrays(1, &axisVao);
		glDeleteBuffers(1, &axisVbo);

		glDeleteVertexArrays(1, &meshVao);
		glDeleteBuffers(1, &meshVbo);
		glDeleteBuffers(1, &meshibo);
	}

	void addGui() override {
		{
			ImGui::Begin("Reconstruct: ");
			ImGui::Text("Reconstruct Time : %d",time);
			ImGui::Text("Vetices : %d",verticesCount);
			ImGui::Text("Faces : %d",indicesCount/3);

			ImGui::SliderInt("PointcloudDecimate", &pointcloudDensity, 1,10);
			ImGui::SliderFloat("Radius", &searchRadius, 0,1);
			ImGui::SliderInt("maxNeighbor", &maximumNearestNeighbors, 5,100);
			ImGui::SliderFloat("maxSurfaceAngle", &maximumSurfaceAngle, 0,M_PI*2);

			for (auto device = realsenses.begin(); device != realsenses.end(); device++) {
				ImGui::Checkbox((device->camera->serial+ std::string("##recon")).c_str(), &(device->use2createMesh));
			}
			if (ImGui::Button("Reconstruct")) {
				clock_t start = clock();
				verticesCount = 0;
				for (auto device = realsenses.begin(); device != realsenses.end(); device++) {
					if (device->use2createMesh) {
						verticesCount += device->camera->vaildVeticesCount;
					}
				}

				if (vertices != nullptr) {
					free(vertices);
				}
				vertices = (float*)calloc(verticesCount * 6, sizeof(float));

				int currentEmpty = 0;
				for (auto device = realsenses.begin(); device != realsenses.end(); device++) {
					if (device->use2createMesh) {
						memcpy(vertices + currentEmpty,
							device->camera->vertexData, 
							device->camera->vaildVeticesCount * 6 * sizeof(float));
						currentEmpty += device->camera->vaildVeticesCount * 6;
					}
				}

				if (indices != nullptr) {
					free(indices);
				}
				indices = fast_triangulation_of_unordered_pcd(
					vertices,
					verticesCount,
					indicesCount,
					searchRadius,
					maximumNearestNeighbors,
					maximumSurfaceAngle
				);

				clock_t end_t = clock();
				time = end_t - start;
				renderMesh = true;
			}
			ImGui::Checkbox("wireframe", &wireframe);
			ImGui::Checkbox("renderMesh", &renderMesh);

			ImGui::End();
		}
		{
			ImGui::Begin("Aruco calibrate: ");
			ImGui::SliderFloat("Threshold", &collectthreshold, 0.05f, 0.3f);
			ImGui::SliderInt("ExpectCollect Point Count", &collectPointCout, 3, 50);
			if (calibrator != nullptr) {
				if (ImGui::Button("cancel calibrate")) {
					delete calibrator;
					calibrator = nullptr;
				}
			}
			ImGui::End();
		}

		{
			ImGui::Begin("Realsense pointcloud viewer: ");                          // Create a window called "Hello, world!" and append into it.
			
			ImGui::SliderFloat("Point size", &pointsize, 0.5f, 50.0f);

			// input url for network device
			static char url[20] = "192.168.0.106";
			ImGui::Text("Network device Ip: ");
			ImGui::SameLine();
			if (ImGui::Button("add")) {
				addNetworkDevice(url);
			}
			ImGui::SameLine();
			ImGui::InputText("##urlInput", url, 20);

			// list all usb3.0 realsense device
			if (ImGui::Button("Refresh"))
				serials = RealsenseDevice::getAllDeviceSerialNumber(ctx);
			ImGui::SameLine();
			ImGui::Text("Realsense device :");

			// waiting active device
			for (std::string serial : serials) {
				bool alreadyStart = false;
				for (auto device = realsenses.begin(); device != realsenses.end(); device++) {
					if (device->camera->serial == serial) {
						alreadyStart = true;
						break;
					}
				}
				if (!alreadyStart)
				{
					if (ImGui::Button(serial.c_str())) {
						addDevice(serial);
					}
				}
			}

			// Running device
			ImGui::Text("Running Realsense device :");
			for (auto device = realsenses.begin(); device != realsenses.end(); device++) {				
				
				if (ImGui::Button((std::string("stop##") + device->camera->serial).c_str())) {
					device->ready2Delete = true;
				}
				ImGui::SameLine();
				ImGui::Text(device->camera->serial.c_str());
				ImGui::SameLine();
				ImGui::Checkbox((std::string("visible##") + device->camera->serial).c_str(), &(device->camera->visible));
				ImGui::SameLine();
				ImGui::Checkbox((std::string("calibrated##") + device->camera->serial).c_str(), &(device->camera->calibrated));

				ImGui::SliderFloat((std::string("clip-z##") + device->camera->serial).c_str(), &device->camera->farPlane, 0.5f, 15.0f);
				ImGui::SameLine();
				ImGui::Checkbox((std::string("OpencvWindow##") + device->camera->serial).c_str(), &(device->camera->opencvImshow));
			}
			ImGui::End();
		}		
	}

	void removeDevice(std::vector<RealsenseGL>::iterator& device) {
		delete device->camera;		
		serials = RealsenseDevice::getAllDeviceSerialNumber(ctx);
		device = realsenses.erase(device);
	}

	void addNetworkDevice(std::string url) try {
		RealsenseGL device;

		device.camera = new RealsenseDevice();		
		std::string serial= device.camera->runNetworkDevice(url, ctx);

		realsenses.push_back(device);
	}
	catch (const rs2::error& e)
	{
		std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
	}

	void addDevice(std::string serial) {
		RealsenseGL device;

		device.camera = new RealsenseDevice();
		device.camera->runDevice(serial.c_str(), ctx);

		glGenVertexArrays(1, &device.vao);
		glGenBuffers(1, &device.vbo);

		realsenses.push_back(device);
	}

	void initGL() override {
		shader_program = ImguiOpeGL3App::genPointcloudShader(this->window);
		glGenVertexArrays(1, &axisVao);
		glGenBuffers(1, &axisVbo);

		glGenVertexArrays(1, &meshVao);
		glGenBuffers(1, &meshVbo);
		glGenBuffers(1, &meshibo);
	}

	void collectCalibratePoints() {

		std::vector<glm::vec2> cornerSrc = OpenCVUtils::opencv_detect_aruco_from_RealsenseRaw(
			calibrator->sourcecam->width,
			calibrator->sourcecam->height,
			calibrator->sourcecam->p_color_frame
		);

		std::vector<glm::vec2> cornerTrg = OpenCVUtils::opencv_detect_aruco_from_RealsenseRaw(
			calibrator->targetcam->width,
			calibrator->targetcam->height,
			calibrator->targetcam->p_color_frame
		);

		if (cornerSrc.size() > 0 && cornerTrg.size() > 0) {
			if (calibrator->vaildCount == calibrator->size) {
				calibrator->calibrate();
				delete calibrator;
				calibrator = nullptr;
			}
			else {
				glm::vec3 centerSrc = glm::vec3(0, 0, 0);
				int count = 0;
				for (auto p : cornerSrc) {
					glm::vec3 point = calibrator->sourcecam->colorPixel2point(p);
					if(point.z==0)	return;
					centerSrc += point;
					count++;
				}
				centerSrc /= count;

				glm::vec3 centerTrg = glm::vec3(0, 0, 0);
				count = 0;
				for (auto p : cornerTrg) {
					glm::vec3 point = calibrator->targetcam->colorPixel2point(p);
					if (point.z == 0)	return;
					centerTrg += point;
					count++;
				}
				centerTrg /= count;

				glm::vec4 src = calibrator->sourcecam->modelMat * glm::vec4(centerSrc.x, centerSrc.y, centerSrc.z, 1.0);
				glm::vec4 dst = calibrator->targetcam->modelMat * glm::vec4(centerTrg.x, centerTrg.y, centerTrg.z, 1.0);

				bool result = calibrator->pushCorrepondPoint(
					glm::vec3(src.x, src.y, src.z),
					glm::vec3(dst.x, dst.y, dst.z)
				);
			}
		}

		//calibrator->sourcecam->calibrated = true;
	}

	void alignDevice2calibratedDevice(RealsenseDevice* uncalibratedCam) {

		RealsenseDevice* baseCamera = nullptr;
		glm::mat4 baseCam2Markerorigion;
		for (auto device = realsenses.begin(); device != realsenses.end(); device++) {
			if (device->camera->calibrated) {
				CalibrateResult c = putAruco2Origion(device->camera);
				if (c.success) {
					baseCamera = device->camera;
					baseCam2Markerorigion = c.calibMat;
					break;
				}
			}
		}
		if (baseCamera) {
			CalibrateResult c = putAruco2Origion(uncalibratedCam);
			if (c.success) {
				uncalibratedCam->modelMat = baseCamera->modelMat*glm::inverse(baseCam2Markerorigion)* c.calibMat;
				
				calibrator = new CorrespondPointCollector(uncalibratedCam,baseCamera,collectPointCout, collectthreshold);
			}
		}
	}

	CalibrateResult putAruco2Origion(RealsenseDevice* camera) {

		CalibrateResult result;
		result.success = false;

		// detect aruco and put tag in origion
		std::vector<glm::vec2> corner = OpenCVUtils::opencv_detect_aruco_from_RealsenseRaw(
			camera->width,
			camera->height,
			camera->p_color_frame
		);

		if (corner.size() > 0) {
			for (auto p : corner) {
				glm::vec3 point = camera->colorPixel2point(p);
				if (point.z == 0) {
					return result;
				}
				result.points.push_back(point);
			}
			glm::vec3 x = glm::normalize(result.points[0] - result.points[1]);
			glm::vec3 z = glm::normalize(result.points[2] - result.points[1]);
			glm::vec3 y = glm::vec3(
				x.y * z.z - x.z * z.y,
				x.z * z.x - x.x * z.z,
				x.x * z.y - x.y * z.x
			);
			glm::mat4 tranform = glm::mat4(
				x.x, x.y, x.z, 0.0,
				y.x, y.y, y.z, 0.0,
				z.x, z.y, z.z, 0.0,
				result.points[1].x, result.points[1].y, result.points[1].z, 1.0
			);
			result.success = true;
			result.calibMat = glm::inverse(tranform);

			// draw xyz-axis
			//GLfloat axisData[] = {
			//	//  X     Y     Z           R     G     B
			//		points[0].x, points[0].y, points[0].z,       0.0f, 0.0f, 0.0f,
			//		points[1].x, points[1].y, points[1].z,       1.0f, 0.0f, 0.0f,
			//		points[2].x, points[2].y, points[2].z,       0.0f, 1.0f, 0.0f,
			//		points[3].x, points[3].y, points[3].z,       0.0f, 0.0f, 1.0f,
			//};
			//ImguiOpeGL3App::setPointsVAO(axisVao, axisVbo, axisData, 4);
			//glm::mat4 mvp = Projection * View;
			//ImguiOpeGL3App::render(mvp, 10.0, shader_program, axisVao, 4, GL_POINTS);
		}
		return result;
	}

	void mainloop() override {

		glm::mat4 mvp = Projection * View * Model;

		// render center axis
		ImguiOpeGL3App::genOrigionAxis(axisVao, axisVbo);
		glm::mat4 mvpAxis = Projection * View * glm::translate(glm::mat4(1.0),lookAtPoint) * Model;
		ImguiOpeGL3App::render(mvpAxis, pointsize, shader_program, axisVao, 6, GL_LINES);

		// render reconstructed mesh
		if (renderMesh && indicesCount != 0) {
			ImguiOpeGL3App::setTrianglesVAOIBO(meshVao, meshVao, meshibo, vertices, verticesCount, indices, indicesCount);
			ImguiOpeGL3App::renderElements(mvp, pointsize, shader_program, meshVao, indicesCount,
				wireframe? GL_LINE: GL_FILL);
			return;
		}

		// render realsense Info
		for (auto device = realsenses.begin(); device != realsenses.end(); device++) {
			glm::mat4 deviceMVP = mvp * device->camera->modelMat;

			if (device->ready2Delete) {
				removeDevice(device);
				device--;
				continue;
			}
			else if (!device->camera->visible) {
				continue;
			}		

			device->camera->fetchframes(pointcloudDensity);
				
			glm::vec3 camhelper = glm::vec3(1, 1, 1);
			bool isCalibratingCamer = false;
			if (calibrator != nullptr) {
				if (device->camera->serial == calibrator->sourcecam->serial) {
					camhelper = glm::vec3(1, 0, 0);
					isCalibratingCamer = true;
					calibrator->render(mvp, shader_program);
				}
				if (device->camera->serial == calibrator->targetcam->serial) {
					camhelper = glm::vec3(0, 1, 0);
					isCalibratingCamer = true;
					calibrator->render(mvp, shader_program);
				}
			}
			else {
				if (device->camera->calibrated) {
					camhelper = glm::vec3(0, 0, 1);
				}
			}
			
			if (isCalibratingCamer || calibrator==nullptr) {
				// render pointcloud
				ImguiOpeGL3App::setPointsVAO(device->vao, device->vbo, device->camera->vertexData, device->camera->vertexCount);
				ImguiOpeGL3App::setTexture(device->image, device->camera->p_color_frame, device->camera->width, device->camera->height);
				ImguiOpeGL3App::render(mvp, pointsize, shader_program, device->vao, device->camera->vaildVeticesCount, GL_POINTS);
			}

			// render camera frustum
			ImguiOpeGL3App::genCameraHelper(
				device->camIconVao, device->camIconVbo,
				device->camera->width, device->camera->height,
				device->camera->intri.ppx, device->camera->intri.ppy, device->camera->intri.fx, device->camera->intri.fy,
				camhelper, 0.2
			);
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
			ImguiOpeGL3App::render(deviceMVP, pointsize, shader_program, device->camIconVao, 3*4, GL_TRIANGLES);

			if (device == realsenses.begin()) {
				device->camera->calibrated = true;
				device->camera->modelMat = glm::mat4(1.0);
			}
			else if(!device->camera->calibrated){

				if (calibrator == nullptr) {
					alignDevice2calibratedDevice(device->camera);
				}
				else {
					collectCalibratePoints();
				}
			}
		}
	}
};

int main() {
	PointcloudApp pviewer;
	pviewer.initImguiOpenGL3();
}