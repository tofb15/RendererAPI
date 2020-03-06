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
#include "../D3D12Engine/Utills/FileSystem.h"

#include <iostream>
#include <chrono>
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
	/*
		Submits the scene to the renderer and triggers it to render.
	*/
	void RenderWindows();
	/*
		Save the current scene to file. The filename of the scene will be decided by m_currentSceneName.
		
		@param saveAsNew, if true m_currentSceneName will be set a non existing generated filename
			before saving in order to avoid overwriting a previusly saved scene.
			If Set to false m_currentSceneName will be used as the filename and potentially overwrite
			any previusly saved scene.

		@return true of succeeded
	*/
	bool SaveScene(bool saveAsNew);
	/*
		Loads a scene from file.
		If the scene uses any assets not yet loaded into memory these will be loaded aswell.

		@param name, the name of the scene to  be loaded.
		@return true if the scene file could be loaded correctly, else false.

		==Remarks==
		Other assets loaded indirectly and asynchronously with this function, like textures,
		may fail to load even though it returns true.
	*/
	bool PreLoadScene(const std::filesystem::path& path, Asset_Types assets_to_load_flag = Asset_Type_Any);
	bool LoadScene(const std::filesystem::path& path, bool clearOld = true);
	bool LoadScene(const std::string& name, bool clearOld = true);
	/*
		Calls ClearScene() and sets up basic scene elements.
	*/
	void NewScene();
	/*
		Clears the scene
	*/
	void ClearScene(bool clearName = true);
	/*
		Refresh all assets stored in the filesystem.
	*/
	void RefreshSceneList();
	/*
		Triggers the renderer to recompile shaders with the current shader defines and settings
	*/
	void ReloadShaders();
	void ReloadShaders(const std::vector<ShaderDefine>& defines);

	//===========ImGui Rendering============
	/**
		Populates a dynamic ImGui container with files listed in a FileSystem::Directory.

		@param path, the directory from which to populate the ImGui Container.
		@param selectedItem, any file with this name will be visualized as selected.
		@param isMenuBar, set this to true if the ImGui container is a menubar, else set this to false.
		@param currDepth, used internally, set this to 0.

		@return the path to the file that was clicked by the user(if any).
			If no file was clicked this will return an empty path: ""
	*/
	std::filesystem::path RecursiveDirectoryList(const FileSystem::Directory& path, const std::string& selectedItem, const bool isMenuBar = false, unsigned int currDepth = 0);

	/*
		ImGui subwindow
	*/
	void RenderObjectEditor();
	/*
		ImGui subwindow
	*/
	void RenderBlueprintWindow();
	/*
		ImGui subwindow
	*/
	void RenderGeometryWindow(Blueprint* bp);
	/*
		ImGui subwindow
	*/
	void RenderLightsAndCameraEditor();
	/*
		ImGui subwindow
	*/
	void RenderSettingWindow();
	/**
		The root-function used to render all the ImGui windows.
		All ImGui rendering shoud be implemented in this functions or functions called from here.
		This will be called at the right time by the renderer if a pointer to this class is passed to Renderer->Present().

		==Remarks==
		This function will be called i the middle of a render call.
		Changing Blueprints or other assets used by the renderer here must be done with care to avoid crashes.
	*/
	void RenderGUI() override;

	void MirrorScene(int lvl = 1);
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
	std::vector<Object*>		m_objects_mirrored;
	std::vector<std::string>    m_unSavedBlueprints;

	bool m_mirrorScene = false;
	int m_mirrorLevel = 1;

	std::string m_sceneFolderPath;
	std::string m_currentSceneName = "";

	
	std::vector<int> m_selectedObjects;
	//int m_selectedObject = 0;

	FileSystem::Directory m_foundScenes;
	FileSystem::Directory m_foundBluePrints;
	FileSystem::Directory m_foundMeshes;
	FileSystem::Directory m_foundTextures;

	double m_time = 0.0;
	double m_ms = 300.0;
	bool m_demoMovement[2];

	std::vector<LightSource> m_lights;
	ResourceManager* m_rm;
	Texture* m_dummyTexture;

	//Scene Settings
	bool m_animateLight = false;
	bool m_allowAnyhitShaders = true;
	float m_time_lightAnim = 0.0;

	//Shader Defines
	bool m_def_NO_NORMAL_MAP = false;
	bool m_def_NO_SHADOWS = false;
	bool m_def_NO_SHADING = false;
	bool m_def_CLOSEST_HIT_ALPHA_TEST_1 = false;
	bool m_def_CLOSEST_HIT_ALPHA_TEST_2 = false;
	bool m_def_TRACE_NON_OPAQUE_SEPARATELY = false;
	bool m_def_RAY_GEN_ALPHA_TEST = false;
	bool m_def_DEBUG_RECURSION_DEPTH = false;
	bool m_def_DEBUG_RECURSION_DEPTH_MISS_ONLY = false;
	bool m_def_DEBUG_RECURSION_DEPTH_HIT_ONLY = false;
	bool m_def_DEBUG_DEPTH = false;
	int m_def_DEBUG_DEPTH_EXP = 100;
	bool m_reloadShaders = false;

	//Scene Load Settings	
	bool m_loadSettingkeepKamera = false;

#ifdef DO_TESTING
	FileSystem::Directory m_TestScenes;
	uint64_t m_nFrames = 0;
	unsigned int m_currentTestSceneIndex = 0;
	unsigned int m_nWarmUpFrames = 500;

	struct ShaderSettings
	{
		std::string name;
		std::vector<ShaderDefine> shaderDefines;
	};

	std::vector<ShaderSettings> m_shaderSettings;
	unsigned int m_currentShaderDefineTestCase;

	void GenerateGnuPlotScript(const std::filesystem::path& scriptPath, const std::filesystem::path& dataPath);
	void GenerateGnuPlotScript_full(const std::filesystem::path& scriptPath, const std::filesystem::path& dataPath);
	void GenerateGnuPlotScript_perScene(const std::filesystem::path& scriptPath, const std::filesystem::path& dataPath);
	void GenerateGnuPlotScript_perShader(const std::filesystem::path& scriptPath, const std::filesystem::path& dataPath);
#endif // DO_TESTING
};