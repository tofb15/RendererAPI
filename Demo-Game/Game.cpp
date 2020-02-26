#include "Game.h"


Game::Game()
{
#ifdef AT_OFFICE
	m_sceneFolderPath = "../../Exported_Assets/Scenes/";
#else
	m_sceneFolderPath = "../assets/Scenes/";
#endif //AT_OFFICE

	RefreshSceneList();
}
Game::~Game()
{
	//for (auto e : m_materials) {
	//	delete e;
	//}
	//for (auto e : m_renderStates) {
	//	delete e;
	//}
	for (auto e : m_cameras) {
		delete e;
	}
	for (auto e : m_objects) {
		delete e;
	}
	for (auto e : m_windows) {
		delete e;
	}
	//for (auto e : m_techniques) {
	//	delete e;
	//}

	delete m_renderer;
	delete m_renderAPI;
	delete m_rm;
	delete m_sm;
}
int Game::Initialize()
{
	if (!InitializeRendererAndWindow())
	{
		return -1;
	}

	m_rm = ResourceManager::GetInstance(m_renderAPI);

#ifdef AT_OFFICE
	m_rm->SetAssetPath("../../Exported_Assets/");
#else
	m_rm->SetAssetPath("../assets/");
#endif // AT_OFFICE

	//Preload one texture into memory. The first texture loaded will be used as default or "missing"-texture.
	m_dummyTexture = m_rm->GetTexture("emplyNormal.png");

	if (!InitializeMaterialsAndRenderStates()) {
		return -2;
	}

	if (!InitializeShadersAndTechniques())
	{
		return -3;
	}
	InitializeCameras();
	if (!InitializeBlueprints()) {
		return -4;
	}
	InitializeObjects();
	//InitializeHeightMap();

	m_lights.emplace_back();

	return 0;
}
bool Game::InitializeRendererAndWindow()
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
bool Game::InitializeMaterialsAndRenderStates()
{
	////Create a material
	//Material* mat = m_renderAPI->MakeMaterial();
	//if (!mat->LoadFromFile("generator.mtl")) { return false; }
	//m_materials.push_back(mat);
	//
	////Create RenderState
	//RenderState* renderState = m_renderAPI->MakeRenderState();
	//renderState->SetWireframe(false);
	//renderState->SetOpaque(true);
	//renderState->SetFaceCulling(RenderState::FaceCulling::BACK);
	//m_renderStates.push_back(renderState);
	//
	//renderState = m_renderAPI->MakeRenderState();
	//renderState->SetWireframe(false);
	//renderState->SetOpaque(false);
	//renderState->SetFaceCulling(RenderState::FaceCulling::NONE);
	//renderState->SetUsingDepthBuffer(true);
	//m_renderStates.push_back(renderState);

	return true;
}
bool Game::InitializeShadersAndTechniques()
{
	//TODO: Add support to select Raytracing shaders here
	//m_sm = m_renderAPI->MakeShaderManager();
	//ShaderDescription sd = {};
	//
	//sd.defines = "#define NORMAL\n#define TEXTCOORD\n";
	//sd.name = "VertexShader";
	//sd.type = ShaderType::VS;
	//Shader vs = m_sm->CompileShader(sd);
	//
	//sd.name = "FragmentShader";
	//sd.type = ShaderType::FS;
	//Shader fs = m_sm->CompileShader(sd);
	//
	//ShaderProgram sp = {};
	//
	//sp.VS = vs;
	//sp.FS = fs;
	//
	////Create Technique from renderstate
	//Technique* tech = m_renderAPI->MakeTechnique(m_renderStates[0], &sp, m_sm);
	//m_techniques.push_back(tech);
	//
	//tech = m_renderAPI->MakeTechnique(m_renderStates[1], &sp, m_sm);
	//m_techniques.push_back(tech);

	return true;
}
bool Game::InitializeBlueprints()
{
#ifdef PRELOAD_RESOURCES
#ifdef AT_OFFICE
	if (m_rm->GetBlueprint("map") == nullptr) { return false; }
	if (m_rm->GetBlueprint("tree") == nullptr) { return false; }
	if (m_rm->GetBlueprint("concrete") == nullptr) { return false; }
	if (m_rm->GetBlueprint("sandbag") == nullptr) { return false; }
	if (m_rm->GetBlueprint("floor") == nullptr) { return false; }
	if (m_rm->GetBlueprint("tent") == nullptr) { return false; }
#else
	if (m_rm->GetBlueprint("turret") == nullptr) { return false; }
	if (m_rm->GetBlueprint("antenna") == nullptr) { return false; }
	if (m_rm->GetBlueprint("enemy_flying") == nullptr) { return false; }
#endif
#endif // PRELOAD_RESOURCES

	return true;
}
void Game::InitializeObjects()
{
	Object* object;

#ifdef AT_OFFICE
#ifdef DEBUG_SCENE
	//Tree
	object = new Object;
	object->blueprint = m_rm->GetBlueprint("tree");
	object->transform.scale = { 1.0f, 1.0f, 1.0f };
	object->transform.pos = { 0,0,0 };
	m_objects.push_back(object);

	//Stone
	object = new Object;
	object->blueprint = m_rm->GetBlueprint("concrete");
	object->transform.scale = { 1.0f, 1.0f, 1.0f };
	object->transform.pos = { 0, 0, 0 };
	m_objects.push_back(object);
#else

#ifdef PERFORMACE_TEST
	//LoadScene("Performance");
#else
	//LoadScene("Default");
#endif //PERFORMACE_TEST
#endif // DEBUG_SCENE
#else
	//Turret
	object = new Object;
	object->blueprint = m_rm->GetBlueprint("turret");
	object->transform.scale = { 1.0f, 1.0f, 1.0f };
	object->transform.pos = { 0, 0, 0 };
	m_objects.push_back(object);

	//Antenna
	object = new Object;
	object->blueprint = m_rm->GetBlueprint("antenna");
	object->transform.scale = { 1.0f, 1.0f, 1.0f };
	object->transform.pos = { 5, 0, 5 };
	m_objects.push_back(object);
#endif // AT_OFFICE

}
void Game::InitializeHeightMap() {
	//ShaderDescription sd_terrain = {};
	//sd_terrain.defines = "";
	//sd_terrain.name = "VertexShader_Terrain";
	//sd_terrain.type = ShaderType::VS;
	//Shader vs_terrain = m_sm->CompileShader(sd_terrain);
	//if (vs_terrain.type == ShaderType::UNKNOWN)
	//	return;

	//sd_terrain.defines = "";
	//sd_terrain.name = "GS_Shader_Terrain";
	//sd_terrain.type = ShaderType::GS;
	//Shader gs_terrain = m_sm->CompileShader(sd_terrain);
	//if (gs_terrain.type == ShaderType::UNKNOWN)
	//	return;

	//sd_terrain.defines = "#define NORMAL\n";
	//sd_terrain.name = "FragmentShader";
	//sd_terrain.type = ShaderType::FS;
	//Shader ps_terrain = m_sm->CompileShader(sd_terrain);
	//if (ps_terrain.type == ShaderType::UNKNOWN)
	//	return;

	//ShaderProgram sp_terrain;
	//sp_terrain.VS = vs_terrain;
	//sp_terrain.GS = gs_terrain;
	//sp_terrain.FS = ps_terrain;

	//Technique* tech_terrain = m_renderAPI->MakeTechnique(m_renderStates[0], &sp_terrain, m_sm);
	//m_techniques.push_back(tech_terrain);

	//Texture* tex = m_renderAPI->MakeTexture();
	//tex->LoadFromFile("../assets/Textures/map3.png", Texture::TEXTURE_USAGE_CPU_FLAG);
	//m_textures.push_back(tex);

	//m_terrain = m_renderAPI->MakeTerrain();
	//m_terrain->InitializeHeightMap(tex, 100);

	//m_terrainBlueprint.mesh = m_terrain->GetMesh();
	//m_meshes.push_back(m_terrainBlueprint.mesh);

	//m_terrainBlueprint.technique = tech_terrain;
}
void Game::InitializeCameras()
{
	Int2 dim = m_windows[0]->GetDimensions();
	float aspRatio = dim.x / dim.y;

	Camera* cam = m_renderAPI->MakeCamera();
	cam->SetPosition(Float3(-50, 100, -50));
	cam->SetTarget(Float3(100, 20, 100));
	cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
	m_cameras.push_back(cam);

	//cam = m_renderAPI->MakeCamera();
	//cam->SetPosition(Float3(-5, 5, -5));
	//cam->SetTarget(Float3(0, 0, 0));
	//cam->SetPerspectiveProjection(3.14159265f * 0.5f, 1.0f, 0.1f, 2000.0f);
	//m_cameras.push_back(cam);
}
void Game::Run()
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
void Game::UpdateObjects(double dt)
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
void Game::UpdateInput()
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
void Game::ProcessGlobalInput()
{
	WindowInput& input_Global = Window::GetGlobalWindowInputHandler();
	int techniqueToUse = 0;

	//if (input_Global.IsKeyDown(WindowInput::KEY_CODE_1)) {
	//	m_demoMovement[0] = true;
	//}
	//if (input_Global.IsKeyDown(WindowInput::KEY_CODE_2)) {
	//	m_demoMovement[1] = true;
	//}
	//if (input_Global.IsKeyDown(WindowInput::KEY_CODE_3)) {
	//	m_demoMovement[0] = m_demoMovement[1] = true;
	//}
	//if (input_Global.IsKeyDown(WindowInput::KEY_CODE_Q)) {
	//	//techniqueToUse = (techniqueToUse + 1) % 2;
	//	//m_blueprints[0]->technique = m_techniques[techniqueToUse];
	//	//m_blueprints[1]->technique = m_techniques[techniqueToUse];
	//}

}
void Game::ProcessLocalInput(double dt)
{
	std::vector<WindowInput*> inputs;


	for (size_t i = 0; i < m_windows.size(); i++)
	{
		inputs.push_back(&m_windows[i]->GetLocalWindowInputHandler());
	}

	if (inputs[0]->IsKeyDown(WindowInput::KEY_CODE_R)) {
		ReloadShaders();
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
void Game::RenderWindows()
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
std::filesystem::path Game::RecursiveDirectoryList(const FileSystem::Directory & path, const std::string & selectedItem, const bool isMenuBar, unsigned int currDepth)
{
	std::filesystem::path clicked = "";
	if (path.files.empty() && path.directories.empty()) {
		ImGui::Text("-Empty Directory-");
		return "";
	}

	std::string depthStr;
	for (size_t i = 0; i < currDepth; i++)
	{
		depthStr += "-";
	}

	for (auto& e : path.directories)
	{
		if (isMenuBar) {
			if (ImGui::BeginMenu((e.path.filename().string()).c_str())) {
				clicked = RecursiveDirectoryList(e, selectedItem, isMenuBar, currDepth + 1);
				ImGui::EndMenu();
			}
		}
		else {
			if (ImGui::CollapsingHeader((depthStr + e.path.filename().string()).c_str())) {
				clicked = RecursiveDirectoryList(e, selectedItem, isMenuBar, currDepth + 1);
			}
		}
	}

	for (auto& e : path.files)
	{
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
void Game::RenderObjectEditor()
{
	ImGui::Columns(3);
	ImGui::Text("Available Blueprints");
	ImGui::NextColumn();
	ImGui::Text("Scene Objects");
	ImGui::NextColumn();
	ImGui::Text("Selected Object");
	ImGui::NextColumn();
	ImGui::Separator();

	ImGui::BeginChild("Blueprints pane");
	//for (auto& e : m_foundBluePrints.files)
	//{
   //	 std::string name = e.path.filename().string().substr(0, e.path.filename().string().find_last_of("."));
   //	 if (ImGui::Selectable(name.c_str(), false)) {
   //	 }
	//}

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
	for (auto& e : m_objects)
	{
		if (ImGui::Selectable(("obj#" + std::to_string(i) + " : " + m_rm->GetBlueprintName(e->blueprint)).c_str(), m_selectedObject == i)) {
			m_selectedObject = i;
		}
		i++;
	}
	ImGui::EndChild();
	ImGui::NextColumn();

	ImGui::BeginChild("Object pane");

	if (m_selectedObject >= 0 && m_selectedObject < m_objects.size()) {
		ImGui::DragFloat3("Pos", (float*)& m_objects[m_selectedObject]->transform.pos, 0.1, -100, 100);
		ImGui::DragFloat3("Rot", (float*)& m_objects[m_selectedObject]->transform.rotation, 0.1, -100, 100);
		ImGui::DragFloat3("Scale", (float*)& m_objects[m_selectedObject]->transform.scale, 0.1, -100, 100);
	}

	ImGui::EndChild();
	ImGui::NextColumn();

	//Reset
	ImGui::Columns(1);
}
void Game::RenderBlueprintWindow() {
	static std::string selected = "";
	static Blueprint* selectedBP = nullptr;

	//ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
	//if (ImGui::Begin("Blueprint List", NULL, ImGuiWindowFlags_MenuBar))
	//{
	//if (ImGui::BeginMenuBar())
	//{
	//	if (ImGui::BeginMenu("Blueprint##bpmb"))
	//	{
	//		if (ImGui::MenuItem("Add New Blueprint")) {
	//			Blueprint* bp = nullptr;
	//			if (bp = m_rm->CreateBlueprint("unsavedBP")) {
	//				selected = "unsavedBP";
	//				selectedBP = bp;
	//				m_unSavedBlueprints.push_back("unsavedBP");
	//			}
	//		}
	//		ImGui::EndMenu();
	//	}
	//	ImGui::EndMenuBar();
	//}


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
		}
		else {
			selected = "";
		}
	}

	ImGui::EndChild();
	ImGui::NextColumn();

	ImGui::BeginChild("Other pane");
	if (selected != "") {
		//ImGui::BeginGroup();
		//ImGui::BeginChild("item view", ImVec2(0, -ImGui::GetFrameHeightWithSpacing())); // Leave room for 1 line below us
		ImGui::Text(selected.c_str());

		std::string meshName = "";
		if (selectedBP) {
			meshName = m_rm->GetMeshName(selectedBP->mesh);
			meshName = meshName.substr(0, meshName.find_last_of("."));
		}

		if (ImGui::BeginCombo("Mesh Select", meshName.c_str())) {
			std::filesystem::path clickedItem = RecursiveDirectoryList(m_foundMeshes, meshName);
			if (clickedItem != "") {
				size_t len1 = m_foundMeshes.path.string().length();
				size_t len2 = clickedItem.string().length() - len1;

				std::string s = clickedItem.string().substr(len1, len2);
				//s = s.substr(0, s.find_last_of("."));

				selectedBP->hasChanged = true;
				selectedBP->mesh = m_rm->GetMesh(s);
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
			ImGui::EndCombo();
		}

		ImGui::Separator();
		if (ImGui::BeginTabBar("##Tabs3", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Geometries"))
			{
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
	//}
	//ImGui::End();
}
void Game::RenderGeometryWindow(Blueprint* bp) {
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
		textureName = textureName.substr(0, textureName.find_last_of("."));
		if (ImGui::BeginCombo("Albedo Texture", textureName.c_str())) {
			std::filesystem::path clickedItem = RecursiveDirectoryList(m_foundTextures, textureName);
			if (clickedItem != "") {
				size_t len1 = m_foundTextures.path.string().length();
				size_t len2 = clickedItem.string().length() - len1;

				std::string s = clickedItem.string().substr(len1, len2);
				//s = s.substr(0, s.find_last_of("."));

				bp->textures[textureIndex] = m_rm->GetTexture(s);
				bp->hasChanged = true;
			}
			ImGui::EndCombo();
		}


		textureIndex++;
		textureName = m_rm->GetTextureName(bp->textures[textureIndex]);
		textureName = textureName.substr(0, textureName.find_last_of("."));
		if (ImGui::BeginCombo("NormalMap Texture", textureName.c_str())) {	
			std::filesystem::path clickedItem = RecursiveDirectoryList(m_foundTextures, textureName);
			if (clickedItem != "") {
				size_t len1 = m_foundTextures.path.string().length();
				size_t len2 = clickedItem.string().length() - len1;

				std::string s = clickedItem.string().substr(len1, len2);
				//s = s.substr(0, s.find_last_of("."));

				bp->textures[textureIndex] = m_rm->GetTexture(s);
				bp->hasChanged = true;
			}

			//for (auto const& texture : m_textureList) {
			//	if (ImGui::Selectable(texture.c_str(), texture == textureName)) {
			//		bp->textures[textureIndex] = m_rm->GetTexture(texture);
			//	}
			//}
			ImGui::EndCombo();
		}
		ImGui::EndChild();
		ImGui::EndGroup();
	}
}
void Game::RenderLightsAndCameraEditor()
{
	if (!m_cameras.empty()) {
		ImGui::DragFloat3("Camera Pos", (float*)& m_cameras.front()->GetPosition(), 0.1, -1000, 1000);
		ImGui::DragFloat3("Camera Targ", (float*)& m_cameras.front()->GetTarget(), 0.1, -1000, 1000);
	}
	else {
		ImGui::Text("No Cameras Exist");
	}

	if (!m_lights.empty()) {
		ImGui::DragFloat3("Light Center", (float*)& m_lights.front().m_position_center, 0.1, -1000, 1000);
		ImGui::DragFloat3("Light Current", (float*)& m_lights.front().m_position_animated, 0.1, -1000, 1000);
	}
	else {
		ImGui::Text("No Lights Exist");
	}
}
void Game::RenderSettingWindow() {
	//static bool open = true;
	ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Scene Settings", NULL, ImGuiWindowFlags_MenuBar))
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("Scene"))
			{
				if (ImGui::MenuItem("Refresh Resources")) {
					RefreshSceneList();
				}

				ImGui::Separator();

				if (ImGui::BeginMenu("Load Scene"))
				{
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
						}
						else {
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

		if (ImGui::BeginTabBar("##Tabs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Edit Blueprints"))
			{
				RenderBlueprintWindow();
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Edit Scene"))
			{
				if (ImGui::BeginTabBar("##Tabs2", ImGuiTabBarFlags_None))
				{
					if (ImGui::BeginTabItem("Objects"))
					{
						RenderObjectEditor();
						ImGui::EndTabItem();
					}

					if (ImGui::BeginTabItem("Camera and lights"))
					{
						RenderLightsAndCameraEditor();
						ImGui::EndTabItem();
					}
					ImGui::EndTabBar();
				}
				ImGui::EndTabItem();
			}

			if (ImGui::BeginTabItem("Scene Options"))
			{
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

				ImGui::PushItemFlag(ImGuiItemFlags_Disabled, m_allowAnyhitShaders);
				if (ImGui::Checkbox("CLOSEST_HIT_ALPHA_TEST", &m_def_CLOSEST_HIT_ALPHA_TEST)) {
					m_reloadShaders = true;
				}
				ImGui::PopItemFlag();
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

	RenderSettingWindow();
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
		while (!nameOK)
		{
			sceneName = "Scene" + std::to_string(i);
			scenePath = m_sceneFolderPath + sceneName + ".scene";
			if (!std::filesystem::exists(scenePath)) {
				nameOK = true;
			}
			i++;
		}

		m_currentSceneName = sceneName;
	}
	else {
		scenePath = m_sceneFolderPath + m_currentSceneName + ".scene";
	}

	std::ofstream outFile(scenePath);
	//Save Camera
	outFile << "Cameras\n";
	outFile << m_cameras.size() << "\n";

	for (auto& e : m_cameras)
	{
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
	for (auto& e : m_lights)
	{
		outFile << e.m_position_center.x << " "
			<< e.m_position_center.y << " "
			<< e.m_position_center.z << "\n";
	}

	//Save Objects
	outFile << "Objects\n";
	outFile << m_objects.size() << "\n";
	for (auto& e : m_objects)
	{
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
bool Game::LoadScene(std::string name) {
	ClearScene();

	std::filesystem::path scenePath;
	scenePath = (m_sceneFolderPath + name + ".scene");
	//if (name.find_last_of(".scene") == name.npos) {
	//}
	//else {
	//	scenePath = (m_sceneFolderPath + name);
	//}

	std::ifstream inFile(scenePath);

	if (!std::filesystem::exists(scenePath) || !inFile.is_open()) {
		return false;
	}

	m_currentSceneName = name;

	//Load Camera
	std::stringstream ss;
	std::string line;
	Float3 tempFloat;
	size_t tempSizeT;

	while (std::getline(inFile, line))
	{
		if (line[0] == '#') {
			continue;
		}
		else if (line == "Cameras") {

			inFile >> tempSizeT;

			Int2 dim = m_windows[0]->GetDimensions();
			float aspRatio = dim.x / dim.y;
			for (size_t i = 0; i < tempSizeT; i++)
			{
				Camera* cam = m_renderAPI->MakeCamera();

				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				cam->SetPosition(tempFloat);
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				cam->SetTarget(tempFloat);

				cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
				m_cameras.push_back(cam);
			}
		}
		else if (line == "Lights") {
			inFile >> m_time_lightAnim;
			inFile >> tempSizeT;
			for (size_t i = 0; i < tempSizeT; i++) {
				inFile >> tempFloat.x >> tempFloat.y >> tempFloat.z;
				LightSource ls;
				ls.m_position_center = (tempFloat);
				m_lights.push_back(ls);
			}
		}
		else if (line == "Objects") {
			//Load Objects
			inFile >> tempSizeT;
			Blueprint* bp;
			for (size_t i = 0; i < tempSizeT; i++)
			{
				inFile.ignore();
				std::getline(inFile, line);
				if ((bp = m_rm->GetBlueprint(line)) == nullptr) {
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

	return true;
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
void Game::ClearScene() {
	m_currentSceneName = "";

	for (auto e : m_objects) {
		delete e;
	}
	m_objects.clear();

	for (auto e : m_cameras) {
		delete e;
	}
	m_cameras.clear();

	m_lights.clear();
}
void Game::RefreshSceneList() {
	m_foundScenes.Clear();
	m_foundBluePrints.Clear();
	m_foundMeshes.Clear();
	m_foundTextures.Clear();

	FileSystem::ListDirectory(m_foundScenes, m_sceneFolderPath, { ".scene" });
	FileSystem::ListDirectory(m_foundBluePrints, "../../Exported_Assets/" + std::string(BLUEPRINT_FOLDER_NAME), { ".bp" });
	FileSystem::ListDirectory(m_foundMeshes, "../../Exported_Assets/" + std::string(MESH_FOLDER_NAME), { ".obj" });
	FileSystem::ListDirectory(m_foundTextures, "../../Exported_Assets/" + std::string(TEXTURE_FODLER_NAME), { ".png" });

	//m_foundScenes.path = m_sceneFolderPath;
	//for (auto& file : std::filesystem::directory_iterator(m_sceneFolderPath))
	//{
	//	if (file.path().extension().string() == ".scene") {
	//		std::string sceneName = file.path().filename().string();
	//		sceneName = sceneName.substr(0, sceneName.find_last_of("."));
	//		
	//		m_foundScenes.push_back(sceneName);
	//	}
	//}

	//
	/*m_foundBluePrints.clear();
	for (auto& file : std::filesystem::directory_iterator("../../Exported_Assets/" + std::string(BLUEPRINT_FOLDER_NAME))) {
		if (file.path().extension().string() == ".bp") {
			std::string s = file.path().filename().string();
			s = s.substr(0, s.find_last_of("."));
			m_foundBluePrints.push_back(s);
		}
	}*/

}
void Game::ReloadShaders() {
	m_reloadShaders = false;

	std::vector<ShaderDefine> defines;
	if (m_def_NO_NORMAL_MAP) {
		defines.push_back({ L"NO_NORMAL_MAP" });
	}

	if (m_def_NO_SHADOWS) {
		defines.push_back({ L"NO_SHADOWS" });
	}

	if (m_def_NO_SHADING) {
		defines.push_back({ L"NO_SHADING" });
	}

	if (m_def_CLOSEST_HIT_ALPHA_TEST && !m_allowAnyhitShaders) {
		defines.push_back({ L"CLOSEST_HIT_ALPHA_TEST" });
	}

	if (m_def_TRACE_NON_OPAQUE_SEPARATELY) {
		defines.push_back({ L"TRACE_NON_OPAQUE_SEPARATELY" });
	}

	if (m_def_RAY_GEN_ALPHA_TEST) {
		defines.push_back({ L"RAY_GEN_ALPHA_TEST" });
	}

	if (m_def_DEBUG_RECURSION_DEPTH) {
		defines.push_back({ L"DEBUG_RECURSION_DEPTH" });
	}

	if (m_def_DEBUG_DEPTH) {
		defines.push_back({ L"DEBUG_DEPTH" });
		defines.push_back({ L"DEBUG_DEPTH_EXP", std::to_wstring(m_def_DEBUG_DEPTH_EXP) });
	}

	m_renderer->Refresh(&defines);
}