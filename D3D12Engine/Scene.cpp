#include "stdafx.h"
#include "Scene.h"
#include <sstream>
#include "RenderAPI.hpp"

#define SCENE_VERSION 2

Scene::Scene() {
}

Scene::~Scene() {
	for (auto e : m_cameras) {
		delete e;
	}
	for (auto e : m_objects) {
		delete e;
	}
}

void Scene::InitializeCameras() {
	//Int2 dim = m_windows[0]->GetDimensions();
	//float aspRatio = dim.x / dim.y;
	//
	//Camera* cam = m_renderAPI->MakeCamera();
	//cam->SetPosition(Float3(-50, 100, -50));
	//cam->SetTarget(Float3(100, 20, 100));
	//cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
	//m_cameras.push_back(cam);
}

void Scene::ClearScene(bool clearName) {
	if (clearName) {
		m_currentSceneName = "";
	}

	for (auto e : m_objects) {
		delete e;
	}
	m_objects.clear();
	m_lights.clear();
}


bool Scene::LoadSceneAssets(const std::filesystem::path& path, ResourceManager* rm, Asset_Types assets_to_load_flag) {
	std::ifstream inFile(path);

	if (!std::filesystem::exists(path) || !inFile.is_open()) {
		m_currentSceneName = "";
		return false;
	}

	//Load Camera
	std::stringstream ss;
	std::string line;
	Float3 tempFloat;
	size_t tempSizeT;

	bool allGood = true;

	while (std::getline(inFile, line)) {
		if (line[0] == '#') {
			continue;
		} else if (line == "Objects") {
			//Load Objects
			inFile >> tempSizeT;
			inFile.ignore();
			for (size_t i = 0; i < tempSizeT; i++) {
				std::getline(inFile, line);
				if (rm->PreLoadBlueprint(line, assets_to_load_flag)) {
					allGood = false;
				}
				for (size_t i = 0; i < 3; i++) {
					std::getline(inFile, line);
				}
			}
		}
	}

	return allGood;
}

bool Scene::LoadScene(const std::string& name, ResourceManager* rm, RenderAPI* api, Int2 dim, bool clearOld) {
	if (clearOld) {
		ClearScene();
	}

	m_currentSceneName = name;
	std::filesystem::path path = rm->GetSceneFolderFullPath() + name;
	std::ifstream inFile(path);
	if (!std::filesystem::exists(path) || !inFile.is_open()) {
		m_currentSceneName = "";
		return false;
	}

	//Load Camera
	std::stringstream ss;
	std::string line;

	if (std::getline(inFile, line)) {
		ss = std::stringstream(line);
		std::string versionText;
		int versionNumber;

		ss >> versionText;
		if (versionText == "version") {
			ss >> versionNumber;
			switch (versionNumber) {
			case 1:
				if (LoadSceneVersion1(inFile, rm, api, dim)) {
					inFile.close();
					return true;
				}
				break;
			case 2:
				if (LoadSceneVersion2(inFile, rm, api, dim)) {
					inFile.close();
					return true;
				}
				break;
			default:
				std::cout << "Error: Scene file version is not supported! Could not load scene." << std::endl;;
				break;
			}
		}
	}

	inFile.close();
	return false;
}


bool Scene::SaveScene(bool saveAsNew, ResourceManager* rm) {
	std::string sceneFolderPath = rm->GetSceneFolderFullPath();
	if (!std::filesystem::exists(sceneFolderPath)) {
		std::filesystem::create_directories(sceneFolderPath);
	}

	int i = 0;
	bool nameOK = false;
	std::filesystem::path scenePath;
	std::string sceneName;

	if (saveAsNew || m_currentSceneName == "") {
		while (!nameOK) {
			sceneName = "Scene" + std::to_string(i);
			scenePath = sceneFolderPath + sceneName + ".scene";
			if (!std::filesystem::exists(scenePath)) {
				nameOK = true;
			}
			i++;
		}

		m_currentSceneName = sceneName;
	} else {
		scenePath = sceneFolderPath + m_currentSceneName;
	}

	std::ofstream outFile(scenePath);

	outFile << "version " << SCENE_VERSION << "\n";

	//Save Camera
	outFile << "Cameras\n"; //Version 2
	outFile << m_cameras.size() << "\n";
	for (auto& e : m_cameras) {
		outFile << e->GetFOV() << "\n";
		outFile << e->GetPosition().x << " "
			<< e->GetPosition().y << " "
			<< e->GetPosition().z << "\n";
		outFile << e->GetTarget().x << " "
			<< e->GetTarget().y << " "
			<< e->GetTarget().z << "\n";
	}

	//Save Lights
	outFile << "Lights\n"; //Version 2
	outFile << m_lights.size() << "\n";
	for (auto& e : m_lights) {
		outFile << e.m_position_center.x << " "
			<< e.m_position_center.y << " "
			<< e.m_position_center.z << "\n";
		outFile << e.m_color.x << " "
			<< e.m_color.y << " "
			<< e.m_color.z << "\n";
		outFile << e.m_enabled << " "
			<< e.m_reachRadius << "\n";
	}

	//Save Objects
	outFile << "Objects\n"; //Version 1
	outFile << m_objects.size() << "\n";
	for (auto& e : m_objects) {
		outFile << rm->GetBlueprintName(e->blueprint) << "\n";

		outFile << e->transform.pos.x << " "
			<< e->transform.pos.y << " "
			<< e->transform.pos.z << "\n";

		outFile << e->transform.scale.x << " "
			<< e->transform.scale.y << " "
			<< e->transform.scale.z << "\n";

		outFile << e->transform.rotation.x << " "
			<< e->transform.rotation.y << " "
			<< e->transform.rotation.z << "\n";
	}

	rm->RefreshFileSystemResourceLists();

	return true;
}

bool Scene::NewScene() {
	ClearScene();
	//ClearSelectedResources();

	//==============
	//Int2 dim = m_windows[0]->GetDimensions();
	//float aspRatio = dim.x / dim.y;

	//Camera* cam = m_renderAPI->MakeCamera();
	//cam->SetPosition(Float3(0, 10, -10));
	//cam->SetTarget(Float3(0, 0, 0));
	//cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
	//m_cameras.push_back(cam);

	//==============
	LightSource ls;
	ls.m_position_center = (Float3(10, 60, 10));
	m_lights.push_back(ls);

	return true;
}

bool Scene::LoadSceneVersion1(std::ifstream& inFile, ResourceManager* rm, RenderAPI* api, Int2 dim) {
	std::stringstream ss;
	std::string line;
	Float3 tempFloat;
	size_t tempSizeT;
	int tempInt;

	while (std::getline(inFile, line)) {
		if (line[0] == '#') {
			continue;
		} else if (line == "Cameras" /*&& !m_loadSettingkeepKamera*/) {

			inFile >> tempSizeT;

			//TODO: Load aspRatio from file.
			//Int2 dim = m_windows[0]->GetDimensions();
			float aspRatio = dim.x / dim.y;
			for (size_t i = 0; i < tempSizeT; i++) {
				Camera* cam = api->MakeCamera();

				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				cam->SetPosition(tempFloat);
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				cam->SetTarget(tempFloat);

				cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
				m_cameras.push_back(cam);
			}
		} else if (line == "Lights") {
			inFile >> tempSizeT;
			for (size_t i = 0; i < tempSizeT; i++) {
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				LightSource ls;
				ls.m_position_center = (tempFloat);
				m_lights.push_back(ls);
			}
		} else if (line == "Objects") {
			//Load Objects
			inFile >> tempSizeT;
			Blueprint* bp;
			for (size_t i = 0; i < tempSizeT; i++) {
				inFile.ignore();
				std::getline(inFile, line);
				if ((bp = rm->GetBlueprint(line)) == nullptr) {
					ClearScene();
					m_currentSceneName = "";
					return false;
				}

				Object* obj = new Object;
				obj->blueprint = bp;

				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				obj->transform.pos = tempFloat;
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				obj->transform.scale = tempFloat;
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				obj->transform.rotation = tempFloat;

				m_objects.push_back(obj);
			}
		}
	}

	if (m_cameras.empty()) {
		//==============
		//Int2 dim = m_windows[0]->GetDimensions();
		float aspRatio = dim.x / dim.y;

		Camera* cam = api->MakeCamera();
		cam->SetPosition(Float3(0, 10, -10));
		cam->SetTarget(Float3(0, 0, 0));
		cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
		m_cameras.push_back(cam);
	}

	if (m_lights.empty()) {
		//==============
		LightSource ls;
		ls.m_position_center = (Float3(10, 60, 10));
		m_lights.push_back(ls);
	}

	return true;
}

bool Scene::LoadSceneVersion2(std::ifstream& inFile, ResourceManager* rm, RenderAPI* api, Int2 dim) {
	std::stringstream ss;
	std::string line;
	Float3 tempFloat;
	size_t tempSizeT;
	int tempInt;

	while (std::getline(inFile, line)) {
		if (line[0] == '#') {
			continue;
		} else if (line == "Cameras" /*&& !m_loadSettingkeepKamera*/) {

			inFile >> tempSizeT;

			//TODO: Load aspRatio from file.
			//Int2 dim = m_windows[0]->GetDimensions();
			float aspRatio = dim.x / dim.y;
			float fov;
			for (size_t i = 0; i < tempSizeT; i++) {
				Camera* cam = api->MakeCamera();
				inFile >> fov;
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				cam->SetPosition(tempFloat);
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				cam->SetTarget(tempFloat);
				//3.14159265f * 0.5f
				cam->SetPerspectiveProjection(fov, aspRatio, 0.1f, 2000.0f);
				m_cameras.push_back(cam);
			}
		} else if (line == "Lights") {
			inFile >> tempSizeT;
			for (size_t i = 0; i < tempSizeT; i++) {
				LightSource ls;
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				ls.m_position_center = (tempFloat);
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				ls.m_color = (tempFloat);
				inFile >> ls.m_enabled >> ls.m_reachRadius;
				m_lights.push_back(ls);
			}
		} else if (line == "Objects") {
			//Load Objects
			inFile >> tempSizeT;
			Blueprint* bp;
			for (size_t i = 0; i < tempSizeT; i++) {
				inFile.ignore();
				std::getline(inFile, line);
				if ((bp = rm->GetBlueprint(line)) == nullptr) {
					ClearScene();
					m_currentSceneName = "";
					return false;
				}

				Object* obj = new Object;
				obj->blueprint = bp;

				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				obj->transform.pos = tempFloat;
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				obj->transform.scale = tempFloat;
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				obj->transform.rotation = tempFloat;

				m_objects.push_back(obj);
			}
		}
	}

	if (m_cameras.empty()) {
		//==============
		//Int2 dim = m_windows[0]->GetDimensions();
		float aspRatio = dim.x / dim.y;

		Camera* cam = api->MakeCamera();
		cam->SetPosition(Float3(0, 10, -10));
		cam->SetTarget(Float3(0, 0, 0));
		cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
		m_cameras.push_back(cam);
	}

	if (m_lights.empty()) {
		//==============
		LightSource ls;
		ls.m_position_center = (Float3(10, 60, 10));
		m_lights.push_back(ls);
	}
	return false;
}