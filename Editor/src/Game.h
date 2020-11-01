#pragma once

#include "FusionReactor_DX12/src/D3D12API.hpp"
#include "FusionReactor/src/Window.hpp"
#include "FusionReactor/src/Camera.hpp"
#include "FusionReactor/src/Mesh.hpp"
#include "FusionReactor/src/Material.hpp"
#include "FusionReactor/src/Technique.hpp"
#include "FusionReactor/src/RenderState.hpp"
#include "FusionReactor/src/Texture.hpp"
#include "FusionReactor/src/ShaderManager.hpp"
#include "FusionReactor/src/Terrain.hpp"
#include "FusionReactor/src/Light/LightSource.h"
#include "FusionReactor/src/ResourceManager.h"
#include "FusionReactor/src/External/IMGUI/imgui.h"
#include "FusionReactor/src/External/IMGUI/imgui_internal.h"
#include "FusionReactor/src/Utills/FileSystem.h"
#include "FusionReactor/src/Scene.h"

#include <iostream>
#include <chrono>
#include <sstream>

#define AT_OFFICE
//#define PERFORMACE_TEST
//#define PRELOAD_RESOURCES


typedef std::chrono::steady_clock Clock;
typedef std::chrono::time_point<std::chrono::steady_clock> Time;
class FusionReactor::Object;

using namespace::FusionReactor;
class Game : public GUI {
public:
	Game();
	virtual ~Game();

	virtual int Initialize();
	bool InitializeRendererAndWindow();

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
		Triggers the renderer to recompile shaders with the current shader defines and settings
	*/
	void ReloadShaders();
	//void ReloadShaders(const std::vector<ShaderDefine>& defines);

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
	void RenderBlueprintWindow();
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
	virtual void RenderGUI() override {};
protected:
	RenderAPI* m_renderAPI;
	Renderer* m_renderer;

	std::vector<Window*>		m_windows;
	WindowInput* m_globalWindowInput;

	Scene m_scene;

	double m_time = 0.0;
	double m_ms = 300.0;

	ResourceManager* m_rm;
	Texture* m_dummyTexture;

	//Scene Settings
	bool m_animateLight = false;
	bool m_allowAnyhitShaders = true;
	float m_time_lightAnim = 0.0;
	float maxRandomPos = 100;
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

	virtual void SubmitObjectsForRendering();
};