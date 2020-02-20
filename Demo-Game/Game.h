#pragma once

#include "../D3D12Engine/RenderAPI.hpp"
#include "../D3D12Engine/Window.hpp"
#include "../D3D12Engine/Camera.hpp"
#include "../D3D12Engine/Mesh.hpp"
#include "../D3D12Engine/Material.hpp"
#include "../D3D12Engine/Technique.hpp"
#include "../D3D12Engine/RenderState.hpp"
#include "../D3D12Engine/Texture.hpp"
#include "../D3D12Engine/ShaderManager.hpp"
#include "../D3D12Engine/Terrain.hpp"
#include "../D3D12Engine/Light/LightSource.h"
#include "../D3D12Engine/ResourceManager.h"
#include "../D3D12Engine/External/IMGUI/imgui.h"
#include "../D3D12Engine/External/IMGUI/imgui_internal.h"

#include <iostream>
#include <chrono>
#include <filesystem>
#include <sstream>

#define AT_OFFICE
//#define PERFORMACE_TEST
//#define PRELOAD_RESOURCES
/*
	GameObject/Entity that can interact with the world and be rendered.
*/
struct Object {
	Blueprint* blueprint;
	Transform transform;

	/*
		Used to clone a blueprint and create a GameObject

		@param bp, the blueprint that should be used to create this object.

		@return, A Object cloned from the input blueprint.
	*/
	static Object* CreateObjectFromBlueprint(Blueprint* bp) { return nullptr; };
};

typedef std::chrono::steady_clock Clock;
typedef std::chrono::time_point<std::chrono::steady_clock> Time;

class Game : public GUI
{
public:
	Game();
	virtual ~Game();

	int Initialize();
	bool InitializeRendererAndWindow();
	bool InitializeMaterialsAndRenderStates();
	bool InitializeShadersAndTechniques();
	bool InitializeBlueprints();
	void InitializeObjects();
	void InitializeHeightMap();
	void InitializeCameras();
	void Run();
	void UpdateObjects(double dt);
	void UpdateInput();
	void ProcessGlobalInput();
	void ProcessLocalInput(double dt);
	void RenderWindows();
	bool SaveScene(bool saveAsNew);
	bool LoadScene(std::string name);
	void NewScene();
	void ClearScene();
	void RefreshSceneList();
	void ReloadShaders();


	//GUI
	void RenderObjectEditor();
	void RenderLightsAndCameraEditor();
	void RenderSettingWindow();
	void RenderGUI() override;

private:
	RenderAPI* m_renderAPI;
	Renderer* m_renderer;

	ShaderManager* m_sm;
	std::vector<Window*>		m_windows;

	//std::vector<Material*>	m_materials;    //Not used
	//std::vector<Technique*>	m_techniques;   //Used for raster
	//std::vector<RenderState*>	m_renderStates; //Used for raster
	std::vector<Camera*>		m_cameras;
	std::vector<Object*>		m_objects;

	std::string m_sceneFolderPath;
	std::string m_currentSceneName = "";
	int m_selectedObject = 0;
	std::vector<std::string>	m_foundScenes;
	std::vector<std::string>	m_foundBluePrints;

	double m_time = 0.0;
	double m_ms = 300.0;
	bool m_demoMovement[2];

	std::vector<LightSource> m_lights;
	ResourceManager* m_rm;

	//Scene Settings
	bool m_animateLight = false;
	bool m_allowAnyhitShaders = true;
	float m_time_lightAnim = 0.0;

	//Shader Defines
	bool m_def_NO_NORMAL_MAP = false;
	bool m_def_NO_SHADOWS = false;
	bool m_def_NO_SHADING = false;
	bool m_def_CLOSEST_HIT_ALPHA_TEST = false;
	bool m_def_TRACE_NON_OPAQUE_SEPARATELY = false;
	bool m_def_RAY_GEN_ALPHA_TEST = false;
	bool m_def_DEBUG_RECURSION_DEPTH = false;
	bool m_def_DEBUG_DEPTH = false;
	int m_def_DEBUG_DEPTH_EXP = 100;
	bool m_reloadShaders = false;
};