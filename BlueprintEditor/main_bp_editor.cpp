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
#include "Profiler.h"

#include <iostream>
#include <string>
#include <crtdbg.h>
#include <chrono>
#include <filesystem>

#define AT_OFFICE

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
	Game()
	{

	}
	virtual ~Game()
	{
		for (auto e : m_materials) {
			delete e;
		}
		for (auto e : m_renderStates) {
			delete e;
		}
		for (auto e : m_cameras) {
			delete e;
		}
		for (auto e : m_objects) {
			delete e;
		}
		for (auto e : m_windows) {
			delete e;
		}
		for (auto e : m_techniques) {
			delete e;
		}

		delete m_renderer;
		delete m_renderAPI;
		delete m_rm;
		delete m_sm;
	}

	int Initialize()
	{
		if (!InitializeRendererAndWindow())
		{
			return -1;
		}

		m_rm = ResourceManager::GetInstance(m_renderAPI);
		m_dummyTexture = m_rm->GetTexture("emplyNormal.png");

#ifdef AT_OFFICE
		m_rm->SetAssetPath("../../Exported_Assets/");
#else
		m_rm->SetAssetPath("../assets/");
#endif // AT_OFFICE

		if (!InitializeMaterialsAndRenderStates()) {
			return -2;
		}

		if (!InitializeShadersAndTechniques())
		{
			return -3;
		}
		InitializeCameras();
		m_lights.emplace_back();
		m_lights.back().m_position_center = (Float3(0, 10, 0));

		ListBlueprints();
		m_profilerAVGFPS.SetTitle("FPS");

		return 0;
	}
	bool InitializeRendererAndWindow()
	{
		//Initialize renderer and window. Maybe we should give more options here to set things like forward/deferred rendering, fullscreen etc.
		m_renderAPI = RenderAPI::MakeAPI(RenderAPI::RenderBackendAPI::D3D12);	//Specify Forward or Deferred Rendering?

		if (m_renderAPI == nullptr) {
			std::cout << "Selected renderAPI was not implemented and could therefor not be created." << std::endl;
			return false;
		}

		if (!m_renderAPI->Initialize()) {
			return false;
		}

		m_renderer = m_renderAPI->MakeRenderer(RenderAPI::RendererType::Raytracing);
		if (!m_renderer) {
			std::cout << "Selected renderer was not implemented within the current renderAPI and could therefor not be created." << std::endl;
			return false;
		}

		//Init Window. if the window is created this way, how should the rendertarget dimensions be specified? 
		Window* window = m_renderAPI->MakeWindow();
		window->SetTitle("Window 1");
		if (!window->Create(1920, 1080)) {
			return false;
		}

		window->Show();
		m_windows.push_back(window);

		return true;
	}

	bool InitializeMaterialsAndRenderStates()
	{
		//Create a material
		Material* mat = m_renderAPI->MakeMaterial();
		if (!mat->LoadFromFile("generator.mtl")) { return false; }
		m_materials.push_back(mat);

		//Create RenderState
		RenderState* renderState = m_renderAPI->MakeRenderState();
		renderState->SetWireframe(false);
		renderState->SetOpaque(true);
		renderState->SetFaceCulling(RenderState::FaceCulling::BACK);
		m_renderStates.push_back(renderState);

		renderState = m_renderAPI->MakeRenderState();
		renderState->SetWireframe(false);
		renderState->SetOpaque(false);
		renderState->SetFaceCulling(RenderState::FaceCulling::NONE);
		renderState->SetUsingDepthBuffer(true);
		m_renderStates.push_back(renderState);

		return true;
	}
	bool InitializeShadersAndTechniques()
	{
		//TODO: Add support to select Raytracing shaders here
		m_sm = m_renderAPI->MakeShaderManager();
		ShaderDescription sd = {};

		sd.defines = "#define NORMAL\n#define TEXTCOORD\n";
		sd.name = "VertexShader";
		sd.type = ShaderType::VS;
		Shader vs = m_sm->CompileShader(sd);

		sd.name = "FragmentShader";
		sd.type = ShaderType::FS;
		Shader fs = m_sm->CompileShader(sd);

		ShaderProgram sp = {};

		sp.VS = vs;
		sp.FS = fs;

		//Create Technique from renderstate
		Technique* tech = m_renderAPI->MakeTechnique(m_renderStates[0], &sp, m_sm);
		m_techniques.push_back(tech);

		tech = m_renderAPI->MakeTechnique(m_renderStates[1], &sp, m_sm);
		m_techniques.push_back(tech);

		return true;
	}

	void InitializeCameras()
	{
		Int2 dim = m_windows[0]->GetDimensions();
		float aspRatio = dim.x / dim.y;

		Camera* cam = m_renderAPI->MakeCamera();
		cam->SetPosition(Float3(0, 0, 0));
		cam->SetTarget(Float3(100, 20, 100));
		cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
		m_cameras.push_back(cam);
	}
	void Run()
	{
		//Delta Time
		static Time now, then;


		m_demoMovement[0] = false;
		m_demoMovement[1] = false;

		m_time = 0;

		Time t1;
		int frameCount = 0;
		int avgUpdateCount = 0;
		int totalFramesLastInterval = 0;
		const float TIME_PER_SHORT_TERM = 0.25f;
		const int SHORT_TERM_UPDATES_PER_LONG_TERM = 8;
		const float TIME_PER_LONG_TERM = TIME_PER_SHORT_TERM * SHORT_TERM_UPDATES_PER_LONG_TERM;

		std::string fpsStr;
		std::string fpsAvgStr;

		t1 = Clock::now();

		//Game Loop
		now = Clock::now();
		while (!m_windows[0]->WindowClosed())
		{
			then = now;
			now = Clock::now();
			double dt = (double)((now - then).count()) * 0.000000001;

			m_time += dt * 0.05f;

			frameCount++;

			m_windows[0]->HandleWindowEvents();
			if ((Clock::now() - t1).count() > 1e9 * TIME_PER_SHORT_TERM)
			{
				t1 = Clock::now();

				// Set short-term average FPS
				int fps = static_cast<int>(frameCount / TIME_PER_SHORT_TERM);
				fpsStr = "FPS: " + std::to_string(fps);

				avgUpdateCount++;
				totalFramesLastInterval += frameCount;
				m_profilerAVGFPS.AddData(fps);

				// Set long-term average FPS
				if (avgUpdateCount >= SHORT_TERM_UPDATES_PER_LONG_TERM)
				{
					int avgFps = static_cast<int>(totalFramesLastInterval / TIME_PER_LONG_TERM);
					fpsAvgStr = ",    Avg FPS: " + std::to_string(avgFps);

					avgUpdateCount = 0;
					totalFramesLastInterval = 0;
				}

				std::string fpsFinalStr = fpsStr + fpsAvgStr;
				m_windows[0]->SetTitle(fpsFinalStr.c_str());
				frameCount = 0;
			}

			ProcessGlobalInput();
			ProcessLocalInput(dt);

			if (m_demoMovement[0])
				m_cameras[0]->SetPosition(Float3(sinf(m_time) * 4.0f, 3.0f, cosf(m_time) * 4.0f));
			if (m_demoMovement[1])
				m_cameras[1]->SetPosition(Float3(sinf(m_time) * 4.0f, 3.0f, cosf(m_time) * 4.0f));

			UpdateObjects(dt);
			RenderWindows();

		}
	}
	void UpdateObjects(double dt)
	{
		if (m_animateLight) {
			m_time_lightAnim += dt * 0.05;
		}

		float lightRad = 50;
		float lightSpeedMulti = 15;
		for (auto& e : m_lights) {
			e.m_position_animated = (e.m_position_center + Float3(cos(m_time_lightAnim * lightSpeedMulti) * lightRad, 0, sin(m_time_lightAnim * lightSpeedMulti) * lightRad));
		}
	}
	void UpdateInput()
	{
		// This input handler is shared between windows
		WindowInput& input_Global = Window::GetGlobalWindowInputHandler();

		//The global input needs to be reset each frame, before any HandleWindowEvents() is called.
		input_Global.Reset();

		//Handle window events to detect window movement, window destruction, input etc. 
		for (size_t i = 0; i < m_windows.size(); i++)
		{
			m_windows[i]->HandleWindowEvents();
		}
	}
	void ProcessGlobalInput()
	{
		WindowInput& input_Global = Window::GetGlobalWindowInputHandler();
		if (input_Global.IsKeyDown(WindowInput::KEY_CODE_R)) {
			ReloadShaders();
		}
	}
	void ProcessLocalInput(double dt)
	{
		std::vector<WindowInput*> inputs;

		for (size_t i = 0; i < m_windows.size(); i++)
		{
			inputs.push_back(&m_windows[i]->GetLocalWindowInputHandler());
		}

		bool rightMouse = inputs[0]->IsKeyDown(WindowInput::MOUSE_KEY_CODE_RIGHT);
		if (rightMouse) {
			for (size_t i = 0; i < m_windows.size(); i++)
			{
				bool shift = inputs[i]->IsKeyDown(WindowInput::KEY_CODE_SHIFT);

				float ms = m_ms * (shift ? 1 : 0.02);
				if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_W))
				{
					m_cameras[i]->Move(m_cameras[0]->GetTargetDirection().normalized() * (ms * dt));
				}
				if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_S))
				{
					m_cameras[i]->Move(m_cameras[0]->GetTargetDirection().normalized() * -(ms * dt));
				}
				if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_A))
				{
					m_cameras[i]->Move(m_cameras[0]->GetRight().normalized() * -(ms * dt));
				}
				if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_D))
				{
					m_cameras[i]->Move(m_cameras[0]->GetRight().normalized() * (ms * dt));
				}

			}

			// Rotation is based on delta time
			Int2 mouseMovement = inputs[0]->GetMouseMovement();
			m_cameras[0]->Rotate({ 0, 1, 0 }, (double)(mouseMovement.x) * dt * 2);
			m_cameras[0]->Rotate(m_cameras[0]->GetRight(), (double)(mouseMovement.y) * dt * 2);
		}
	}
	void RenderWindows()
	{
		if (m_reloadShaders) {
			ReloadShaders();
		}
		Camera* cam0 = m_cameras[0];
		Camera* cam;
		Window* window;
		Object* obj;

		size_t nWindows = m_windows.size();
		size_t nObjects = m_objects.size();
		for (int i = 0; i < nWindows; i++)
		{
			m_renderer->ClearSubmissions();
			m_renderer->SetLightSources(m_lights);
			window = m_windows[i];
			cam = m_cameras[i];

			for (int j = 0; j < nObjects; j++)
			{
				obj = m_objects[j];
				m_renderer->Submit({ obj->blueprint, obj->transform }, cam0);
			}

			//m_renderer->Submit({ &m_terrainBlueprint }, m_cameras[0], 1);

			//Draw all meshes in the submit list. Do we want to support multiple frames? What if we want to render split-screen? Could differend threads prepare different frames?
			m_renderer->Frame(window, cam);
			m_renderer->Present(window, this);
		}
	}

	////////////////////////
	void RenderMenu() {
		// ===========Menu==============
		if (ImGui::BeginMainMenuBar())
		{
			if (ImGui::BeginMenu("Project"))
			{
				if (ImGui::MenuItem("New Project", "")) {

				}
				if (ImGui::MenuItem("Open Project", "")) {}
				if (ImGui::MenuItem("Save", "", false)) {}
				if (ImGui::MenuItem("Save As", "", false)) {}

				//ShowExampleMenuFile();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Scene"))
			{
				if (ImGui::MenuItem("New Scene", "")) {

				}

				//ShowExampleMenuFile();
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Edit"))
			{
				if (ImGui::MenuItem("Undo", "CTRL+Z")) {}
				if (ImGui::MenuItem("Redo", "CTRL+Y", false, false)) {}  // Disabled item
				ImGui::Separator();
				if (ImGui::MenuItem("Cut", "CTRL+X")) {}
				if (ImGui::MenuItem("Copy", "CTRL+C")) {}
				if (ImGui::MenuItem("Paste", "CTRL+V")) {}
				ImGui::EndMenu();
			}
			ImGui::EndMainMenuBar();
		}
	}

	void RenderGeometryWindow(Blueprint* bp) {
		//if(ImGui::Checkbox("Force Opaque", &bp->allGeometryIsOpaque)) {
		//	bp->hasChanged = true;
		//}

		ImGui::BeginChild("Geometry left pane", ImVec2(150, 0), true);
		static int selectedGeometry = 0;

		if (bp->mesh) {
			for (int i = 0; i < bp->mesh->GetNumberOfSubMeshes(); i++)
			{		
				if (ImGui::Selectable(bp->mesh->GetSubMesheName(i).c_str(), i == selectedGeometry)) {
						selectedGeometry = i;
				}

				if (ImGui::IsItemHovered()) {
					ImGui::BeginTooltip();
					ImGui::Text(bp->mesh->GetSubMesheName(i).c_str());
					ImGui::EndTooltip();
				}
			}
			ImGui::Separator();
		}

		ImGui::EndChild();
		ImGui::SameLine();

		// right
		if (bp->mesh && selectedGeometry < bp->mesh->GetNumberOfSubMeshes()) {
			ImGui::BeginGroup();
			ImGui::BeginChild("item view 2", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us

			bool b = bp->alphaTested[selectedGeometry];
			if (ImGui::Checkbox("Alpha Tested", &b)) {
				bp->hasChanged = true;
				bp->alphaTested[selectedGeometry] = b;

				bp->allGeometryIsOpaque = true;
				for (bool b : bp->alphaTested) {
					if (b) {
						bp->allGeometryIsOpaque = false;
						break;
					}
				}
			}

			//if (selectedBP) {
			//	meshName = m_rm->GetMeshName(selectedBP->mesh);
			//}
			std::string textureName;
			int textureIndex;

			textureIndex = selectedGeometry * 2;	
			textureName = m_rm->GetTextureName(bp->textures[textureIndex]);
			if (ImGui::BeginCombo("Albedo Texture", textureName.c_str())) {
				for (auto const& texture : m_textureList) {
					if (ImGui::Selectable(texture.c_str(), texture == textureName)) {
						bp->textures[textureIndex] = m_rm->GetTexture(texture);
					}
				}
				ImGui::EndCombo();
			}


			textureIndex++;
			textureName = m_rm->GetTextureName(bp->textures[textureIndex]);
			if (ImGui::BeginCombo("NormalMap Texture", textureName.c_str())) {
				for (auto const& texture : m_textureList) {
					if (ImGui::Selectable(texture.c_str(), texture == textureName)) {
						bp->textures[textureIndex] = m_rm->GetTexture(texture);
					}
				}
				ImGui::EndCombo();
			}
			ImGui::EndChild();
			ImGui::EndGroup();
		}
	}

	void RenderBlueprintWindow() {
		static bool open = true;
		static std::string selected = "";
		static Blueprint* selectedBP = nullptr;

		ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Blueprint List", &open, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("Add New Blueprint")) {
						Blueprint* bp = nullptr;
						if (bp = m_rm->CreateBlueprint("unsavedBP")) {
							selected = "unsavedBP";
							selectedBP = bp;
							m_unSavedBlueprints.push_back("unsavedBP");

							for (auto& e : m_objects)
							{
								delete e;
							}
							m_objects.clear();
						}
					}
					if (ImGui::MenuItem("Refresh")) {
						ListBlueprints();
					}
					if (ImGui::MenuItem("Save Selected")) {

					}
					if (ImGui::MenuItem("SaveAll")) {

					}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}

			if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
			{
				if (ImGui::BeginTabItem("Blueprints"))
				{
					// left
					//static Blueprint* selected = nullptr;

					ImGui::BeginChild("left pane", ImVec2(150, 0), true);
					int i = 0;

					if (!m_unSavedBlueprints.empty()) {
						for (auto& e : m_unSavedBlueprints)
						{
							if (ImGui::Selectable(e.c_str(), selected == e)) {
								if (selectedBP = m_rm->GetBlueprint(e)) {
									selected = e;
									if (m_objects.empty()) {
										m_objects.push_back(new Object);
									}
									m_objects.front()->blueprint = selectedBP;
								}
							}

							if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
								Blueprint* bp = m_rm->GetBlueprint(e);
								bool found = false;
								for (int j = 0; j < m_objects.size(); j++) {
									if (m_objects[j]->blueprint == bp) {
										found = true;
										m_objects.erase(m_objects.begin() + j);
										break;
									}
								}

								if (!found) {
									m_objects.push_back(new Object);
									m_objects.back()->blueprint = bp;
								}
							}
							i++;
						}

						ImGui::Separator();
					}
					
					for (auto& e : m_SavedBlueprints)
					{		
						if (ImGui::Selectable(e.c_str(), selected == e)) {
							if (selectedBP = m_rm->GetBlueprint(e)) {
								selected = e;
								if (m_objects.empty()) {
									m_objects.push_back(new Object);
								}
								m_objects.front()->blueprint = selectedBP;
							}
						}

						if (ImGui::IsItemHovered() && ImGui::IsMouseClicked(1)) {
							Blueprint* bp = m_rm->GetBlueprint(e);
							bool found = false;
							for (int j = 0; j < m_objects.size(); j++) {
								if (m_objects[j]->blueprint == bp) {
									found = true;
									m_objects.erase(m_objects.begin() + j);
									break;
								}
							}

							if (!found) {
								m_objects.push_back(new Object);
								m_objects.back()->blueprint = bp;
							}
						}
						i++;
					}

					ImGui::EndChild();
					ImGui::SameLine();

					// right
					if (selected != "") {
						ImGui::BeginGroup();
						ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
						ImGui::Text(selected.c_str());

						std::string meshName = "";
						if (selectedBP) {
							meshName = m_rm->GetMeshName(selectedBP->mesh);
						}
						if (ImGui::BeginCombo("Mesh Select", meshName.c_str())) {
							for (auto const& mesh : m_meshList) {
								if (ImGui::Selectable(mesh.c_str(), mesh == meshName)) {
									selectedBP->hasChanged = true;
									selectedBP->mesh = m_rm->GetMesh(mesh);
									int nNeededTextures = selectedBP->mesh->GetNumberOfSubMeshes() * 2;
									for (size_t i = selectedBP->textures.size(); i < nNeededTextures; i++)
									{
										selectedBP->textures.push_back(m_dummyTexture);
									}

									if (selectedBP->textures.size() > nNeededTextures) {
										selectedBP->textures.erase(selectedBP->textures.begin() + nNeededTextures, selectedBP->textures.end());
									}

									selectedBP->allGeometryIsOpaque = true;
									selectedBP->alphaTested.clear();
									for (size_t i = 0; i < selectedBP->mesh->GetNumberOfSubMeshes(); i++)
									{
										selectedBP->alphaTested.push_back(false);
									}
								}
							}
							ImGui::EndCombo();
						}

						ImGui::Separator();
						if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
						{
							if (ImGui::BeginTabItem("Geometries"))
							{
								RenderGeometryWindow(selectedBP);
								ImGui::EndTabItem();
							}
							ImGui::EndTabBar();
						}
						ImGui::EndChild();
						if (ImGui::Button("Revert")) {}
						ImGui::SameLine();
						if (ImGui::Button("Save")) {				
							m_rm->SaveBlueprintToFile(selectedBP, m_rm->GetBlueprintName(selectedBP));						
						}
						ImGui::EndGroup();
					}
					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Scene"))
				{
					ImGui::Text("Scene Options");
					if (ImGui::Checkbox("Animated Light", &m_animateLight)) {}
					ImGui::SameLine();
					ImGui::DragFloat("Light Anim time", &m_time_lightAnim, 0.001, 0, 1);

					ImGui::Separator();
					ImGui::Text("Shader Options");
					if (ImGui::Checkbox("NO_NORMAL_MAP", &m_def_NO_NORMAL_MAP)) {
						m_reloadShaders = true;
					}

					if (ImGui::Checkbox("NO_SHADOWS", &m_def_NO_SHADOWS)) {
						m_reloadShaders = true;
					}

					if (ImGui::Checkbox("NO_SHADING", &m_def_NO_SHADING)) {
						m_reloadShaders = true;
					}

					ImGui::EndTabItem();
				}

				if (ImGui::BeginTabItem("Performance"))
				{
					m_profilerAVGFPS.Render();
					ImGui::EndTabItem();
				}

				ImGui::EndTabBar();
			}
		}
		ImGui::End();
	}

	void RenderGUI() override {
		//UI SETUP HERE
		static bool b = true;
		if (b)
			ImGui::ShowDemoWindow(&b);

		RenderMenu();
		RenderBlueprintWindow();
	}
	void ListBlueprints() {
		std::error_code err;

		m_SavedBlueprints.clear();
		for (auto& file : std::filesystem::directory_iterator("../../Exported_Assets/" + std::string(BLUEPRINT_FOLDER_NAME), err)) {
			if (file.path().extension().string() == ".bp") {
				std::string s = file.path().filename().string();
				s = s.substr(0, s.find_last_of("."));
				if (!m_rm->IsBlueprintLoaded(file.path().filename().string())) {
					m_SavedBlueprints.push_back(s);
				}
			}
		}

		//Textures
		m_textureList.clear();
		for (auto& file : std::filesystem::directory_iterator("../../Exported_Assets/" + std::string(TEXTURE_FODLER_NAME), err)) {
			if (file.path().extension().string() == ".png") {
				std::string s = file.path().filename().string();
				//s = s.substr(0, s.find_last_of("."));
				m_textureList.push_back(s);
			}
		}

		//Meshes
		m_meshList.clear();
		for (auto& file : std::filesystem::directory_iterator("../../Exported_Assets/" + std::string(MESH_FOLDER_NAME), err)) {
			if (file.path().extension().string() == ".obj") {
				std::string s = file.path().filename().string();
				//s = s.substr(0, s.find_last_of("."));
				m_meshList.push_back(s);
			}
		}
	}
	void ReloadShaders() {
		m_reloadShaders = false;
		std::vector<ShaderDefine> defines;
		if (m_def_NO_NORMAL_MAP) {
			defines.push_back({L"NO_NORMAL_MAP"});
		}

		if (m_def_NO_SHADOWS) {
			defines.push_back({ L"NO_SHADOWS" });
		}
		
		if (m_def_NO_SHADING) {
			defines.push_back({ L"NO_SHADING" });
		}

		m_renderer->Refresh(&defines);
	}

	void CreateNewBlueprint(std::string bpName) {
		
	}

private:
	RenderAPI* m_renderAPI;
	Renderer* m_renderer;

	ShaderManager* m_sm;
	std::vector<Window*>		m_windows;

	//Globals. Since these vectors are used by all games using this API, should these maybe we its own class called something like "SceneManager"?
	std::vector<Material*>		m_materials;
	std::vector<Technique*>		m_techniques;
	std::vector<RenderState*>	m_renderStates;
	std::vector<Camera*>		m_cameras;
	std::vector<Object*>		m_objects;
	std::vector<Object*>		m_locked;

	//Terrain* m_terrain;
	Blueprint m_terrainBlueprint;

	double m_time = 0.0;
	float m_time_lightAnim = 0.0;
	double m_ms = 300.0;
	bool m_demoMovement[2];

	std::vector<LightSource> m_lights;
	ResourceManager* m_rm;

	std::vector<std::string> m_SavedBlueprints;
	std::vector<std::string> m_unSavedBlueprints;

	std::vector<std::string> m_textureList;
	std::vector<std::string> m_meshList;

	Texture* m_dummyTexture;

	bool m_animateLight = false;
	//Shader Defines
	bool m_def_NO_NORMAL_MAP = false;
	bool m_def_NO_SHADOWS = false;
	bool m_def_NO_SHADING = false;
	bool m_reloadShaders = false;

	//GUI
	Profiler m_profilerAVGFPS;
};

/*This main is only an exemple of how this API could/should be used to render a scene.*/
int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Game game;
	if (game.Initialize() < 0) {
		return 0;
	}
	game.Run();

	return 0;
}