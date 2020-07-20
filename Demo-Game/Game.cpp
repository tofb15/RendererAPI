#include "Game.h"
#include "../D3D12Engine/Utills/Utills.h"

Game::Game() {}

Game::~Game() {
	for (auto e : m_cameras) {
		delete e;
	}
	for (auto e : m_objects) {
		delete e;
	}

	for (auto e : m_objects_mirrored) {
		delete e;
	}

	for (auto e : m_windows) {
		delete e;
	}

	delete m_renderer;
	delete m_renderAPI;
	delete m_rm;
	delete m_sm;
}
int Game::Initialize() {

	if (!InitializeRendererAndWindow()) {
		return -1;
	}

	InitializeCameras();

	m_rm = ResourceManager::GetInstance(m_renderAPI);
	m_rm->SetAssetPath("../assets/");
	m_sceneFolderPath = "../assets/Scenes/";

	//Preload one texture into memory. The first texture loaded will be used as default or "missing"-texture.
	m_dummyTexture = m_rm->GetTexture("emplyNormal.png");
	m_lights.emplace_back();

	RefreshSceneList();
	for (auto scene : m_foundScenes.files) {
		if (scene.path.stem() == "example_scene") {
			LoadScene(scene.path);
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
	if (m_reloadShaders) {
		ReloadShaders();
	}
	Camera* cam0 = m_cameras[0];
	Window* window;

	size_t nWindows = m_windows.size();
	for (int i = 0; i < nWindows; i++) {
		m_renderer->ClearSubmissions();
		m_renderer->SetLightSources(m_lights);
		window = m_windows[i];

		for (auto& e : m_objects) {
			m_renderer->Submit({ e->blueprint, e->transform }, cam0);
		}

		if (m_mirrorScene) {
			for (auto& e : m_objects_mirrored) {
				m_renderer->Submit({ e->blueprint, e->transform }, cam0);
			}
		}

		//Draw all meshes in the submit list.
		//TODO: Do we want to support multiple Frame calls in a row? What if we want to render split-screen? Could differend threads prepare different frames?

		m_renderer->Frame(window, cam0);
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
void Game::RenderObjectEditor() {
	if (ImGui::Checkbox("Mirror Scene", &m_mirrorScene)) {
		if (m_mirrorScene) {
			MirrorScene(m_mirrorLevel);
		}
	}

	ImGui::SameLine();

	if (ImGui::DragInt("Mirror Level", &m_mirrorLevel, 1, 1, 5)) {
		if (m_mirrorScene) {
			MirrorScene(m_mirrorLevel);
		}
	}

	if (ImGui::Button("Apply Mirror Permanent")) {
		MirrorScenePermanent();
	}

	ImGui::Columns(3);
	ImGui::Text("Available Blueprints");
	ImGui::NextColumn();
	ImGui::Text("Scene Objects");
	ImGui::NextColumn();
	ImGui::Text(("Selected Objects: " + std::to_string(m_selectedObjects.size())).c_str());
	ImGui::NextColumn();
	ImGui::Separator();

	ImGui::BeginChild("Blueprints pane");

	std::filesystem::path clickedItem = RecursiveDirectoryList(m_foundBluePrints, "");
	if (clickedItem != "") {
		size_t len1 = m_foundBluePrints.path.string().length();
		size_t len2 = clickedItem.string().length() - len1;

		std::string s = clickedItem.string().substr(len1, len2);
		s = s.substr(0, s.find_last_of("."));

		Object* obj = new Object;
		obj->blueprint = m_rm->GetBlueprint(s);
		m_objects.push_back(obj);
	}

	ImGui::EndChild();
	ImGui::NextColumn();

	ImGui::BeginChild("Objects pane");
	int i = 0;
	for (auto& e : m_objects) {
		if (ImGui::Selectable(("obj#" + std::to_string(i) + " : " + m_rm->GetBlueprintName(e->blueprint)).c_str(), Contains<std::vector, int>(m_selectedObjects, i))) {
			//m_selectedObjects.push_back(i);
		}

		if (ImGui::IsItemClicked(0) && ImGui::IsItemHovered()) {
			if (!m_selectedObjects.empty() && Window::GetGlobalWindowInputHandler().IsKeyDown(WindowInput::KEY_CODE_SHIFT)) {
				int min = m_selectedObjects.front();
				int max = i;
				if (min > max) {
					std::swap(min, max);
				}

				m_selectedObjects.clear();
				for (int a = min; a <= max; a++) {
					m_selectedObjects.push_back(a);
				}
			} else if (!m_selectedObjects.empty() && Window::GetGlobalWindowInputHandler().IsKeyDown(WindowInput::KEY_CODE_CTRL)) {
				if (!Contains<std::vector, int>(m_selectedObjects, i)) {
					m_selectedObjects.push_back(i);
				}
			} else {
				m_selectedObjects.clear();
				m_selectedObjects.push_back(i);
			}
		}
		i++;
	}
	ImGui::EndChild();
	ImGui::NextColumn();

	ImGui::BeginChild("Object pane");

	if (!m_selectedObjects.empty()) {
		size_t size = (int)m_objects.size();
		size_t nSelected = (int)m_selectedObjects.size();

		if (ImGui::Button("Copy")) {
			for (auto i : m_selectedObjects) {
				Object* obj = new Object();
				memcpy(obj, m_objects[i], sizeof(Object));
				m_objects.push_back(obj);
			}
			m_selectedObjects.clear();
			for (size_t i = size; i < size + nSelected; i++) {
				m_selectedObjects.push_back(i);
			}
		}

		ImGui::SameLine();

		if (ImGui::Button("Delete")) {
			std::sort(m_selectedObjects.begin(), m_selectedObjects.end(), std::greater <int>());
			for (auto i : m_selectedObjects) {
				delete m_objects[i];
				m_objects.erase(m_objects.begin() + i);
			}

			m_selectedObjects.clear();
			if (!m_objects.empty()) {
				m_selectedObjects.push_back(0);
			}
		}

		if (m_selectedObjects.size() == 1) {
			int& selectedObject = m_selectedObjects.front();

			if (selectedObject < m_objects.size()) {
				if (ImGui::Button("Select All Identical")) {
					Object* selected = m_objects[m_selectedObjects.front()];
					m_selectedObjects.clear();
					int index = 0;
					for (auto i : m_objects) {
						if (i->blueprint == selected->blueprint) {
							m_selectedObjects.push_back(index);
						}
						index++;
					}
				}

				ImGui::DragFloat3("Pos", (float*)& m_objects[selectedObject]->transform.pos, 0.1, -100, 100);
				ImGui::DragFloat3("Rot", (float*)& m_objects[selectedObject]->transform.rotation, 0.1, -100, 100);
				ImGui::DragFloat3("Scale", (float*)& m_objects[selectedObject]->transform.scale, 0.1, -100, 100);
			}
		} else {

			if (ImGui::Button("Reset Rotation")) {
				for (auto i : m_selectedObjects) {
					m_objects[i]->transform.rotation = { 0,0,0 };
				}
			}

			if (ImGui::Button("Random X-Rot")) {
				for (auto i : m_selectedObjects) {
					m_objects[i]->transform.rotation.x = (rand() / (float)RAND_MAX) * 2 * 3.15;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Random Y-Rot")) {
				for (auto i : m_selectedObjects) {
					m_objects[i]->transform.rotation.y = (rand() / (float)RAND_MAX) * 2 * 3.15;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Random Z-Rot")) {
				for (auto i : m_selectedObjects) {
					m_objects[i]->transform.rotation.z = (rand() / (float)RAND_MAX) * 2 * 3.15;
				}
			}

			/////////////

			if (ImGui::Button("Reset Position")) {
				for (auto i : m_selectedObjects) {
					m_objects[i]->transform.pos = { 0,0,0 };
				}
			}

			ImGui::DragFloat("Max random spread", &maxRandomPos, 1, 0, 10000);

			if (ImGui::Button("Random X-Pos")) {
				for (auto i : m_selectedObjects) {
					m_objects[i]->transform.pos.x = (rand() / (float)RAND_MAX) * maxRandomPos - maxRandomPos / 2;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Random Y-Pos")) {
				for (auto i : m_selectedObjects) {
					m_objects[i]->transform.pos.y = (rand() / (float)RAND_MAX) * maxRandomPos - maxRandomPos / 2;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Random Z-Pos")) {
				for (auto i : m_selectedObjects) {
					m_objects[i]->transform.pos.z = (rand() / (float)RAND_MAX) * maxRandomPos - maxRandomPos / 2;
				}
			}
		}
	}


	ImGui::EndChild();
	ImGui::NextColumn();

	//Reset
	ImGui::Columns(1);
}

void Game::RenderBlueprintWindow() {
	static std::string selected = "";
	static Blueprint* selectedBP = nullptr;

	ImGui::Columns(2);
	ImGui::Text("Blueprints");
	ImGui::NextColumn();
	ImGui::Text("Other");
	ImGui::NextColumn();
	ImGui::Separator();
	std::filesystem::path clickedItem;

	ImGui::BeginChild("Blueprints pane");
	clickedItem = RecursiveDirectoryList(m_foundBluePrints, "");
	if (clickedItem != "") {
		size_t len1 = m_foundBluePrints.path.string().length();
		size_t len2 = clickedItem.string().length() - len1;

		std::string s = clickedItem.string().substr(len1, len2);
		s = s.substr(0, s.find_last_of("."));

		if (selectedBP = m_rm->GetBlueprint(s)) {
			selected = s;
		} else {
			selected = "";
		}
	}

	ImGui::EndChild();
	ImGui::NextColumn();

	ImGui::BeginChild("Other pane");
	if (selected != "") {
		ImGui::Text(selected.c_str());

		std::string meshName = "";
		if (selectedBP) {
			meshName = m_rm->GetMeshName(selectedBP->mesh);
			meshName = meshName.substr(0, meshName.find_last_of("."));
			if (ImGui::BeginCombo("Mesh Select", meshName.c_str())) {
				std::filesystem::path clickedItem = RecursiveDirectoryList(m_foundMeshes, meshName);
				if (clickedItem != "") {
					size_t len1 = m_foundMeshes.path.string().length();
					size_t len2 = clickedItem.string().length() - len1;

					std::string s = clickedItem.string().substr(len1, len2);

					selectedBP->hasChanged = true;
					selectedBP->mesh = m_rm->GetMesh(s);
					int nNeededMaterials = selectedBP->mesh->GetNumberOfSubMeshes();
					for (size_t i = selectedBP->materials.size(); i < nNeededMaterials; i++) {
						selectedBP->materials.push_back(selectedBP->materials.back());
					}

					if (selectedBP->materials.size() > nNeededMaterials) {
						selectedBP->materials.erase(selectedBP->materials.begin() + nNeededMaterials, selectedBP->materials.end());
					}
				}
				ImGui::EndCombo();
			}
		}

		ImGui::Separator();
		if (ImGui::BeginTabBar("##Tabs3", ImGuiTabBarFlags_None)) {
			if (ImGui::BeginTabItem("Geometries")) {
				RenderGeometryWindow(selectedBP);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		if (ImGui::Button("Revert")) {}
		ImGui::SameLine();
		if (ImGui::Button("Save")) {
			m_rm->SaveBlueprintToFile(selectedBP, m_rm->GetBlueprintName(selectedBP));
		}
	}
	ImGui::EndChild();
	ImGui::NextColumn();
}

void Game::RenderGeometryWindow(Blueprint* bp) {
	ImGui::BeginChild("Geometry left pane", ImVec2(150, 0), true);
	static int selectedGeometry = 0;

	if (bp->mesh) {
		for (int i = 0; i < bp->mesh->GetNumberOfSubMeshes(); i++) {
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

		std::string materialName;
		int materialIndex;

		materialIndex = selectedGeometry;
		materialName = m_rm->GetMaterialName(bp->materials[materialIndex]);
		materialName = materialName.substr(0, materialName.find_last_of("."));
		if (ImGui::BeginCombo("Material: ", materialName.c_str())) {
			std::filesystem::path clickedItem = RecursiveDirectoryList(m_foundMaterials, materialName);
			if (clickedItem != "") {
				size_t len1 = m_foundMaterials.path.string().length();
				size_t len2 = clickedItem.string().length() - len1;

				std::string s = clickedItem.string().substr(len1, len2);

				bp->materials[materialIndex] = m_rm->GetMaterial(s);
				bp->hasChanged = true;
			}
			ImGui::EndCombo();
		}

		ImGui::EndChild();
		ImGui::EndGroup();
	}
}
void Game::RenderLightsAndCameraEditor() {
	if (!m_cameras.empty()) {
		ImGui::DragFloat3("Camera Pos", (float*)& m_cameras.front()->GetPosition(), 0.1, -1000, 1000);
		ImGui::DragFloat3("Camera Targ", (float*)& m_cameras.front()->GetTarget(), 0.1, -1000, 1000);
	} else {
		ImGui::Text("No Cameras Exist");
	}

	if (!m_lights.empty()) {
		ImGui::DragFloat3("Light Center", (float*)& m_lights.front().m_position_center, 0.1, -1000, 1000);
		ImGui::DragFloat3("Light Current", (float*)& m_lights.front().m_position_animated, 0.1, -1000, 1000);
	} else {
		ImGui::Text("No Lights Exist");
	}
}
void Game::RenderSettingWindow() {
	//static bool open = true;
	ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Scene Settings", NULL, ImGuiWindowFlags_MenuBar)) {
		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Scene")) {
				if (ImGui::MenuItem("Refresh Resources")) {
					RefreshSceneList();
				}

				if (ImGui::BeginMenu("Load Settings")) {
					ImGui::Checkbox("Keep Kamera", &m_loadSettingkeepKamera);
					ImGui::EndMenu();
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("Load Scene")) {
					if (ImGui::MenuItem("New Scene")) {
						NewScene();
					}
					ImGui::Separator();
					std::filesystem::path clickedItem = RecursiveDirectoryList(m_foundScenes, m_currentSceneName, true);

					if (clickedItem != "") {
						size_t len1 = m_foundScenes.path.string().length();
						size_t len2 = clickedItem.string().length() - len1;

						std::string s = clickedItem.string().substr(len1, len2);
						s = s.substr(0, s.find_last_of("."));

						if (LoadScene(s)) {
							m_currentSceneName = s;
						} else {
							m_currentSceneName = "";
						}
					}

					ImGui::EndMenu();
				}

				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, m_currentSceneName == "");
				if (ImGui::MenuItem("Save Scene")) {
					SaveScene(false);
				}
				ImGui::PopItemFlag();

				if (ImGui::MenuItem("Save Scene As New")) {
					SaveScene(true);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None)) {
			if (ImGui::BeginTabItem("Edit Blueprints")) {
				RenderBlueprintWindow();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Edit Scene")) {
				if (ImGui::BeginTabBar("##Tabs2", ImGuiTabBarFlags_None)) {
					if (ImGui::BeginTabItem("Objects")) {
						RenderObjectEditor();
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Camera and lights")) {
						RenderLightsAndCameraEditor();
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
void Game::RenderGUI() {
	//UI SETUP HERE
	static bool b = false;
	if (b)
		ImGui::ShowDemoWindow(&b);

	// ===========Menu==============
	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("Project")) {
			if (ImGui::MenuItem("New Project", "")) {

			}
			if (ImGui::MenuItem("Open Project", "")) {}
			if (ImGui::MenuItem("Save", "", false)) {}
			if (ImGui::MenuItem("Save As", "", false)) {}

			//ShowExampleMenuFile();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Scene")) {
			if (ImGui::MenuItem("New Scene", "")) {

			}

			//ShowExampleMenuFile();
			ImGui::EndMenu();
		}
		if (ImGui::BeginMenu("Edit")) {
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

	RenderSettingWindow();


}
void Game::MirrorScene(int lvl) {
	int n_mirrored = 0;
	float dx = 44.587;
	float dz = 48.785;

	int fx;
	int fz;

	for (auto e : m_objects_mirrored) {
		delete e;
	}
	m_objects_mirrored.clear();

	for (int x = -lvl; x <= lvl; x++) {
		for (int z = -lvl; z <= lvl; z++) {
			if (x == 0 && z == 0) {
				continue;
			}

			fx = (((x + lvl * 2) % 2) * 2 - 1) * -1;
			fz = (((z + lvl * 2) % 2) * 2 - 1) * -1;

			for (auto& o : m_objects) {
				Object* m = nullptr;
				if (n_mirrored < m_objects_mirrored.size()) {
					m = m_objects_mirrored[n_mirrored];
				} else {
					m = new Object();
					m_objects_mirrored.push_back(m);
				}
				m->blueprint = o->blueprint;
				m->transform = o->transform;
				m->transform.scale *= Float3(fx, 1, fz); //only work for 3x3
				m->transform.pos *= Float3(fx, 1, fz);
				m->transform.pos += Float3(dx * x, 0, dz * z);
				n_mirrored++;
			}
		}
	}
}

void Game::MirrorScenePermanent() {
	m_objects.reserve(m_objects.size() + m_objects_mirrored.size());
	m_objects.insert(m_objects.end(), m_objects_mirrored.begin(), m_objects_mirrored.end());

	m_mirrorLevel = 1;
	m_mirrorLevel = false;
	m_objects_mirrored.clear();
	m_objects_mirrored.shrink_to_fit();
}


bool Game::SaveScene(bool saveAsNew) {
	if (!std::filesystem::exists(m_sceneFolderPath)) {
		std::filesystem::create_directories(m_sceneFolderPath);
	}

	int i = 0;
	bool nameOK = false;
	std::filesystem::path scenePath;
	std::string sceneName;

	if (saveAsNew || m_currentSceneName == "") {
		while (!nameOK) {
			sceneName = "Scene" + std::to_string(i);
			scenePath = m_sceneFolderPath + sceneName + ".scene";
			if (!std::filesystem::exists(scenePath)) {
				nameOK = true;
			}
			i++;
		}

		m_currentSceneName = sceneName;
	} else {
		scenePath = m_sceneFolderPath + m_currentSceneName + ".scene";
	}

	std::ofstream outFile(scenePath);

	if (m_mirrorScene) {
		outFile << "Mirror\n" << m_mirrorLevel << "\n";
	}

	//Save Camera
	outFile << "Cameras\n";
	outFile << m_cameras.size() << "\n";

	for (auto& e : m_cameras) {
		outFile << e->GetPosition().x << " "
			<< e->GetPosition().y << " "
			<< e->GetPosition().z << "\n";

		outFile << e->GetTarget().x << " "
			<< e->GetTarget().y << " "
			<< e->GetTarget().z << "\n";
	}

	//Save Lights
	outFile << "Lights\n";
	outFile << m_time_lightAnim << "\n";
	outFile << m_lights.size() << "\n";
	for (auto& e : m_lights) {
		outFile << e.m_position_center.x << " "
			<< e.m_position_center.y << " "
			<< e.m_position_center.z << "\n";
	}

	//Save Objects
	outFile << "Objects\n";
	outFile << m_objects.size() << "\n";
	for (auto& e : m_objects) {
		outFile << m_rm->GetBlueprintName(e->blueprint) << "\n";

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

	RefreshSceneList();

	return true;
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

bool Game::LoadScene(const std::filesystem::path& path, bool clearOld) {
	if (clearOld) {
		ClearScene();
	}

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
		} else if (line == "Mirror") {
			m_mirrorScene = true;
			inFile >> m_mirrorLevel;
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

	if (m_mirrorLevel) {
		MirrorScene(m_mirrorLevel);
	}

	return true;
}

bool Game::LoadScene(const std::string& name, bool clearOld) {
	std::filesystem::path scenePath;
	scenePath = (m_sceneFolderPath + name + ".scene");
	m_currentSceneName = name;

	return LoadScene(scenePath, clearOld);
}

void Game::NewScene() {
	ClearScene();

	//==============
	Int2 dim = m_windows[0]->GetDimensions();
	float aspRatio = dim.x / dim.y;

	Camera* cam = m_renderAPI->MakeCamera();
	cam->SetPosition(Float3(0, 10, -10));
	cam->SetTarget(Float3(0, 0, 0));
	cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
	m_cameras.push_back(cam);

	//==============
	LightSource ls;
	ls.m_position_center = (Float3(10, 60, 10));
	m_lights.push_back(ls);
}
void Game::ClearScene(bool clearName) {
	m_mirrorScene = false;
	if (clearName) {
		m_currentSceneName = "";
	}

	for (auto e : m_objects) {
		delete e;
	}
	m_objects.clear();

	for (auto e : m_objects_mirrored) {
		delete e;
	}
	m_objects_mirrored.clear();

	if (!m_loadSettingkeepKamera) {
		for (auto e : m_cameras) {
			delete e;
		}
		m_cameras.clear();
	}

	m_lights.clear();
}
void Game::RefreshSceneList() {
#ifdef PERFORMANCE_TESTING
	FileSystem::ListDirectory(m_TestScenes, m_sceneFolderPath + "TestScenes/", { ".scene" });
#endif // PERFORMANCE_TESTING
	FileSystem::ListDirectory(m_foundScenes, m_sceneFolderPath, { ".scene" });
	FileSystem::ListDirectory(m_foundBluePrints, m_rm->GetAssetPath() + std::string(BLUEPRINT_FOLDER_NAME), { ".bp" });
	FileSystem::ListDirectory(m_foundMeshes, m_rm->GetAssetPath() + std::string(MESH_FOLDER_NAME), { ".obj" });
	FileSystem::ListDirectory(m_foundTextures, m_rm->GetAssetPath() + std::string(TEXTURE_FODLER_NAME), { ".png", ".dds", ".simpleTexture" });
	FileSystem::ListDirectory(m_foundMaterials, m_rm->GetAssetPath() + std::string(MATERIAL_FOLDER_NAME), { ".txt" });
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

//void Game::ReloadShaders(const std::vector<ShaderDefine>& defines) {
//	//m_renderer->Refresh(&defines);
//}
