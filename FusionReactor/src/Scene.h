#pragma once
#include "Light/LightSource.h"
#include "Camera.hpp"
#include "GameObject.h"
#include "ResourceManager.h"

#include <vector>
#include <string>

class RenderAPI;

class Scene {
public:
	Scene();
	~Scene();

	void InitializeCameras();

	void ClearScene(bool clearName = true);
	bool LoadSceneAssets(const std::filesystem::path& path, ResourceManager* rm, Asset_Types assets_to_load_flag = Asset_Type_Any);
	/*
		Loads a scene from file.
		If the scene uses any assets not yet loaded into memory these will be loaded aswell.

		@param name, the name of the scene to  be loaded.
		@return true if the scene file could be loaded correctly, else false.

		==Remarks==
		Some assets might be loaded asynchronously as an effect of calling this function, like textures.
		These assets might still fail to load even though LoadScene returns true.
	*/
	bool LoadScene(const std::string& name, ResourceManager* rm, RenderAPI* api, Int2 dim, bool clearOld = true);
	/*
	Save the current scene to file. The filename of the scene will be decided by m_currentSceneName.

	@param saveAsNew, if true m_currentSceneName will be set a non existing generated filename
		before saving in order to avoid overwriting a previusly saved scene.
		If Set to false m_currentSceneName will be used as the filename and potentially overwrite
		any previusly saved scene.

	@return true of succeeded
	*/
	bool SaveScene(bool saveAsNew, ResourceManager* rm);
	/*
	Calls ClearScene() and sets up basic scene elements.
	*/
	bool NewScene();
public:
	std::string m_currentSceneName = "";
	std::vector<Camera*>		m_cameras; // List of all cameras
	std::vector<Object*>		m_objects; // List of All game objects
	std::vector<LightSource> m_lights;

	bool LoadSceneVersion1(std::ifstream& file, ResourceManager* rm, RenderAPI* api, Int2 dim);
	bool LoadSceneVersion2(std::ifstream& file, ResourceManager* rm, RenderAPI* api, Int2 dim);
};
