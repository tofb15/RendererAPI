#include "Game.h"
#include "../D3D12Engine/Utills/Utills.h"
#include "GameObject.h"

Game::Game() {}

Game::~Game() {
	for (auto e : m_cameras) {
		delete e;
	}
	for (auto e : m_objects) {
		delete e;
	}

	for (auto e : m_windows) {
		delete e;
	}

	if (m_renderer) {
		delete m_renderer;
	}
	if (m_renderAPI) {
		delete m_renderAPI;
	}
	if (m_rm) {
		delete m_rm;
	}
}
int Game::Initialize() {

	if (!InitializeRendererAndWindow()) {
		return -1;
	}

	m_globalWindowInput = &m_windows[0]->GetGlobalWindowInputHandler();

	InitializeCameras();

	m_rm = ResourceManager::GetInstance(m_renderAPI);
	m_rm->SetAssetPath("../assets/");

	//Preload one texture into memory. The first texture loaded will be used as default or "missing"-texture.
	m_dummyTexture = m_rm->GetTexture("emptyNormal.png");
	m_lights.emplace_back();

	m_rm->RefreshFileSystemResourceLists();
	for (auto scene : m_rm->m_foundScenes.files) {
		if (scene.path.stem() == "example_scene") {
			size_t start = m_rm->m_foundScenes.path.string().length();
			size_t count = scene.path.string().length() - start;
			std::string s = scene.path.string().substr(start, count);

			LoadScene(s);
			break;
		}
	}

	return 0;
}
bool Game::InitializeRendererAndWindow() {
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

void Game::InitializeCameras() {
	Int2 dim = m_windows[0]->GetDimensions();
	float aspRatio = dim.x / dim.y;

	Camera* cam = m_renderAPI->MakeCamera();
	cam->SetPosition(Float3(-50, 100, -50));
	cam->SetTarget(Float3(100, 20, 100));
	cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
	m_cameras.push_back(cam);
}

void Game::Run() {
	//Delta Time
	static Time now, then;

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
	while (!m_windows[0]->WindowClosed()) {
		then = now;
		now = Clock::now();
		double dt = (double)((now - then).count()) * 0.000000001;

		m_time += dt * 0.05f;

		frameCount++;

		m_windows[0]->HandleWindowEvents();
		if ((Clock::now() - t1).count() > 1e9 * TIME_PER_SHORT_TERM) {
			t1 = Clock::now();

			// Set short-term average FPS
			int fps = static_cast<int>(frameCount / TIME_PER_SHORT_TERM);
			fpsStr = "FPS: " + std::to_string(fps);

			avgUpdateCount++;
			totalFramesLastInterval += frameCount;

			// Set long-term average FPS
			if (avgUpdateCount >= SHORT_TERM_UPDATES_PER_LONG_TERM) {
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

		UpdateObjects(dt);
		RenderWindows();

	}
}
void Game::UpdateObjects(double dt) {
	if (m_animateLight) {
		m_time_lightAnim += dt * 0.05;
	}

	float lightRad = 50;
	float lightSpeedMulti = 15;
	for (auto& e : m_lights) {
		e.m_position_animated = (e.m_position_center + Float3(cos(m_time_lightAnim * lightSpeedMulti) * lightRad, 0, sin(m_time_lightAnim * lightSpeedMulti) * lightRad));
	}
}
void Game::UpdateInput() {
	// This input handler is shared between windows
	WindowInput& input_Global = Window::GetGlobalWindowInputHandler();

	//The global input needs to be reset each frame, before any HandleWindowEvents() is called.
	input_Global.Reset();

	//Handle window events to detect window movement, window destruction, input etc. 
	for (size_t i = 0; i < m_windows.size(); i++) {
		m_windows[i]->HandleWindowEvents();
	}
}

void Game::ProcessGlobalInput() {
	WindowInput& input_Global = Window::GetGlobalWindowInputHandler();
}

void Game::ProcessLocalInput(double dt) {
	std::vector<WindowInput*> inputs;


	for (size_t i = 0; i < m_windows.size(); i++) {
		inputs.push_back(&m_windows[i]->GetLocalWindowInputHandler());
	}

	if (inputs[0]->IsKeyDown(WindowInput::KEY_CODE_R)) {
		ReloadShaders();
	}

	bool rightMouse = inputs[0]->IsKeyDown(WindowInput::MOUSE_KEY_CODE_RIGHT);
	if (rightMouse) {
		for (size_t i = 0; i < m_windows.size(); i++) {
			bool shift = inputs[i]->IsKeyDown(WindowInput::KEY_CODE_SHIFT);
			float ms = m_ms * (shift ? 1 : 0.02);
			if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_W)) {
				m_cameras[i]->Move(m_cameras[0]->GetTargetDirection().normalized() * (ms * dt));
			}
			if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_S)) {
				m_cameras[i]->Move(m_cameras[0]->GetTargetDirection().normalized() * -(ms * dt));
			}
			if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_A)) {
				m_cameras[i]->Move(m_cameras[0]->GetRight().normalized() * -(ms * dt));
			}
			if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_D)) {
				m_cameras[i]->Move(m_cameras[0]->GetRight().normalized() * (ms * dt));
			}
		}

		// Rotation is based on delta time
		Int2 mouseMovement = inputs[0]->GetMouseMovement();
		m_cameras[0]->Rotate({ 0, 1, 0 }, (double)(mouseMovement.x) * dt * 2);
		m_cameras[0]->Rotate(m_cameras[0]->GetRight(), (double)(mouseMovement.y) * dt * 2);
	}
}

void Game::RenderWindows() {
	Window* window;

	size_t nWindows = m_windows.size();
	for (int i = 0; i < nWindows; i++) {
		m_renderer->ClearSubmissions();
		m_renderer->SetLightSources(m_lights);
		window = m_windows[i];

		SubmitObjectsForRendering();

		//TODO: Do we want to support multiple Frame calls in a row? What if we want to render split-screen? Could differend threads prepare different frames?
		m_rm->PrepareRendering();
		m_renderer->Frame(window, m_cameras[0]);
		m_renderer->Present(window, this);
	}
}

std::filesystem::path Game::RecursiveDirectoryList(const FileSystem::Directory& path, const std::string& selectedItem, const bool isMenuBar, unsigned int currDepth) {
	std::filesystem::path clicked = "";
	if (path.files.empty() && path.directories.empty()) {
		ImGui::Text("-Empty Directory-");
		return "";
	}

	std::string depthStr;
	for (size_t i = 0; i < currDepth; i++) {
		depthStr += "-";
	}

	for (auto& e : path.directories) {
		if (isMenuBar) {
			if (ImGui::BeginMenu((e.path.filename().string()).c_str())) {
				clicked = RecursiveDirectoryList(e, selectedItem, isMenuBar, currDepth + 1);
				ImGui::EndMenu();
			}
		} else {
			if (ImGui::CollapsingHeader((depthStr + e.path.filename().string()).c_str())) {
				clicked = RecursiveDirectoryList(e, selectedItem, isMenuBar, currDepth + 1);
			}
		}
	}

	for (auto& e : path.files) {
		std::string name = e.path.filename().string().substr(0, e.path.filename().string().find_last_of("."));
		if (!isMenuBar) {
			name = depthStr + name;
		}
		if (ImGui::Selectable(name.c_str(), selectedItem == name)) {
			clicked = e.path;
		}
	}

	return clicked;
}

void Game::RenderBlueprintWindow() {
	//if (ImGui::Button("Revert")) {}
	//ImGui::SameLine();
	//if (ImGui::Button("Save")) {
	//	m_rm->SaveBlueprintToFile(selectedBP, m_rm->GetBlueprintName(selectedBP));
	//}
}

void Game::RenderSettingWindow() {
	//static bool open = true;
	ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Scene Settings", NULL, ImGuiWindowFlags_MenuBar)) {
		if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
			if (ImGui::BeginTabItem("Edit Blueprints")) {
				RenderBlueprintWindow();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Edit Scene")) {
				if (ImGui::BeginTabBar("##Tabs2", ImGuiTabBarFlags_None)) {
					if (ImGui::BeginTabItem("Objects")) {
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Scene Options")) {
				if (ImGui::Checkbox("Animated Light", &m_animateLight)) {}
				ImGui::SameLine();
				ImGui::DragFloat("Light Anim time", &m_time_lightAnim, 0.001, 0, 1);

				if (ImGui::Checkbox("Allow Anyhit Shaders", &m_allowAnyhitShaders)) {
					m_renderer->SetSetting("anyhit", m_allowAnyhitShaders);
					m_reloadShaders = true;
				}

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

				if (ImGui::Checkbox("TRACE_NON_OPAQUE_SEPARATELY", &m_def_TRACE_NON_OPAQUE_SEPARATELY)) {
					m_reloadShaders = true;
				}

				if (ImGui::Checkbox("DEBUG_RECURSION_DEPTH", &m_def_DEBUG_RECURSION_DEPTH)) {
					m_reloadShaders = true;
				}
				ImGui::SameLine();
				if (ImGui::Checkbox("MISS_ONLY", &m_def_DEBUG_RECURSION_DEPTH_MISS_ONLY)) {
					if (m_def_DEBUG_RECURSION_DEPTH_MISS_ONLY) {
						m_def_DEBUG_RECURSION_DEPTH_HIT_ONLY = false;
					}
					m_reloadShaders = true;
				}
				ImGui::SameLine();
				if (ImGui::Checkbox("HIT_ONLY", &m_def_DEBUG_RECURSION_DEPTH_HIT_ONLY)) {
					if (m_def_DEBUG_RECURSION_DEPTH_HIT_ONLY) {
						m_def_DEBUG_RECURSION_DEPTH_MISS_ONLY = false;
					}
					m_reloadShaders = true;
				}

				if (ImGui::Checkbox("DEBUG_DEPTH", &m_def_DEBUG_DEPTH)) {
					m_reloadShaders = true;
				}
				ImGui::SameLine();
				if (ImGui::DragInt("DEBUG_DEPTH_EXP", &m_def_DEBUG_DEPTH_EXP, 2, 1, 1000)) {
					m_reloadShaders = true;
				}

				if (ImGui::Checkbox("RAY_GEN_ALPHA_TEST", &m_def_RAY_GEN_ALPHA_TEST)) {
					m_reloadShaders = true;
				}

				if (ImGui::Checkbox("CLOSEST_HIT_ALPHA_TEST_1", &m_def_CLOSEST_HIT_ALPHA_TEST_1)) {
					m_reloadShaders = true;
				}

				if (ImGui::Checkbox("CLOSEST_HIT_ALPHA_TEST_2", &m_def_CLOSEST_HIT_ALPHA_TEST_2)) {
					m_reloadShaders = true;
				}
				ImGui::EndTabItem();
			}

			ImGui::EndTabBar();
		}
	}
	ImGui::End();
}

void Game::SubmitObjectsForRendering() {
	for (auto& e : m_objects) {
		m_renderer->Submit({ e->blueprint, e->transform }, m_cameras[0]);
	}
}

bool Game::PreLoadScene(const std::filesystem::path& path, Asset_Types assets_to_load_flag) {
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
				if (m_rm->PreLoadBlueprint(line, assets_to_load_flag)) {
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

bool Game::LoadScene(const std::string& name, bool clearOld) {
	if (clearOld) {
		ClearScene();
	}

	m_currentSceneName = name;
	std::filesystem::path path = m_rm->GetSceneFolderFullPath() + name;
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
	int tempInt;
	while (std::getline(inFile, line)) {
		if (line[0] == '#') {
			continue;
		} else if (line == "Cameras" && !m_loadSettingkeepKamera) {

			inFile >> tempSizeT;

			Int2 dim = m_windows[0]->GetDimensions();
			float aspRatio = dim.x / dim.y;
			for (size_t i = 0; i < tempSizeT; i++) {
				Camera* cam = m_renderAPI->MakeCamera();

				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				cam->SetPosition(tempFloat);
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				cam->SetTarget(tempFloat);

				cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
				m_cameras.push_back(cam);
			}
		} else if (line == "Lights") {
			inFile >> m_time_lightAnim;
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
				if ((bp = m_rm->GetBlueprint(line)) == nullptr) {
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
		Int2 dim = m_windows[0]->GetDimensions();
		float aspRatio = dim.x / dim.y;

		Camera* cam = m_renderAPI->MakeCamera();
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

void Game::ClearScene(bool clearName) {
	if (clearName) {
		m_currentSceneName = "";
	}

	for (auto e : m_objects) {
		delete e;
	}
	m_objects.clear();

	if (!m_loadSettingkeepKamera) {
		for (auto e : m_cameras) {
			delete e;
		}
		m_cameras.clear();
	}

	m_lights.clear();
}

void Game::ReloadShaders() {
	m_reloadShaders = false;

	//std::vector<ShaderDefine> defines;
	//if (m_def_NO_NORMAL_MAP) {
	//	defines.push_back({ L"NO_NORMAL_MAP" });
	//}
	//
	//if (m_def_NO_SHADOWS) {
	//	defines.push_back({ L"NO_SHADOWS" });
	//}
	//
	//if (m_def_NO_SHADING) {
	//	defines.push_back({ L"NO_SHADING" });
	//}
	//
	//if (m_def_CLOSEST_HIT_ALPHA_TEST_1) {
	//	defines.push_back({ L"CLOSEST_HIT_ALPHA_TEST_1" });
	//}
	//
	//if (m_def_CLOSEST_HIT_ALPHA_TEST_2) {
	//	defines.push_back({ L"CLOSEST_HIT_ALPHA_TEST_2" });
	//}
	//
	//if (m_def_TRACE_NON_OPAQUE_SEPARATELY) {
	//	defines.push_back({ L"TRACE_NON_OPAQUE_SEPARATELY" });
	//}
	//
	//if (m_def_RAY_GEN_ALPHA_TEST) {
	//	defines.push_back({ L"RAY_GEN_ALPHA_TEST" });
	//}
	//
	//if (m_def_DEBUG_RECURSION_DEPTH) {
	//	defines.push_back({ L"DEBUG_RECURSION_DEPTH" });
	//
	//	if (m_def_DEBUG_RECURSION_DEPTH_MISS_ONLY) {
	//		defines.push_back({ L"DEBUG_RECURSION_DEPTH_MISS_ONLY" });
	//	}
	//
	//	if (m_def_DEBUG_RECURSION_DEPTH_HIT_ONLY) {
	//		defines.push_back({ L"DEBUG_RECURSION_DEPTH_HIT_ONLY" });
	//	}
	//}
	//
	//if (m_def_DEBUG_DEPTH) {
	//	defines.push_back({ L"DEBUG_DEPTH" });
	//	defines.push_back({ L"DEBUG_DEPTH_EXP", std::to_wstring(m_def_DEBUG_DEPTH_EXP) });
	//}
	//
	//m_renderer->Refresh(&defines);
}
