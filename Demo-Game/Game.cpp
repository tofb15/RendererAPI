#include "Game.h"
#include "../D3D12Engine/Utills/Utills.h"

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

	for (auto e : m_objects_mirrored) {
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

#ifdef DO_TESTING
	
	m_shaderSettings = {
		//{"Minimal-SIMPLE",           {{L"SIMPLE_HIT"}}},
		//{"DICE-SIMPLE",              {{L"SIMPLE_HIT"},               {L"DICE_ANYHIT"}}},
		//{"RayGen-SIMPLE",            {{L"SIMPLE_HIT"},               {L"RAY_GEN_ALPHA_TEST"}}},
		//{"Close1-SIMPLE",            {{L"SIMPLE_HIT"},               {L"CLOSEST_HIT_ALPHA_TEST_1"}}},
		//{"Close2-SIMPLE",            {{L"SIMPLE_HIT"},               {L"CLOSEST_HIT_ALPHA_TEST_2"}}},
		
		{"Minimal-NS",                 {{L"NO_SHADOWS"}} },
		{"DICE-NS",                    {{L"DICE_ANYHIT"},              {L"NO_SHADOWS"}}},
		{"RayGen-NS",                  {{L"RAY_GEN_ALPHA_TEST"},       {L"NO_SHADOWS"}}},
		{"Close1-NS",                  {{L"CLOSEST_HIT_ALPHA_TEST_1"}, {L"NO_SHADOWS"}}},
		{"Close2-NS",                  {{L"CLOSEST_HIT_ALPHA_TEST_2"}, {L"NO_SHADOWS"}}},
		
		//{"Minimal",                  {}},
		//{"DICE",                     {{L"DICE_ANYHIT"}}},
		//{"RayGen",                   {{L"RAY_GEN_ALPHA_TEST"}}},
		//{"Close1",                   {{L"CLOSEST_HIT_ALPHA_TEST_1"}}},
		//{"Close2",                   {{L"CLOSEST_HIT_ALPHA_TEST_2"}}},
	    
		//DEBUG
		{"RayGen-Debug",             {{L"RAY_GEN_ALPHA_TEST"}, {L"DEBUG_RECURSION_DEPTH"}}},
	};

	m_currentTestSceneIndex = 0;
	m_currentShaderDefineTestCase = 0;
	//GenerateGnuPlotScript("TestData/Scripts", "TestData/Data");
	//return -1;
	for (auto e : m_TestScenes.files)
	{
		//Preload all Textures from all Scenes
		PreLoadScene(e.path, Asset_Type_Texture);
		//TODO: WaitFor All
	}
	m_rm->WaitUntilResourcesIsLoaded();
#endif // DO_TESTING
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
	//Camera* cam;
	Window* window;

	size_t nWindows = m_windows.size();
	for (int i = 0; i < nWindows; i++)
	{
		m_renderer->ClearSubmissions();
		m_renderer->SetLightSources(m_lights);
		window = m_windows[i];
		//cam = m_cameras[i];

		for (auto& e : m_objects)
		{
			m_renderer->Submit({ e->blueprint, e->transform }, cam0);
		}

		if (m_mirrorScene) {
			for (auto& e : m_objects_mirrored)
			{
				m_renderer->Submit({ e->blueprint, e->transform }, cam0);
			}
		}

		//m_renderer->Submit({ &m_terrainBlueprint }, m_cameras[0], 1);

		//Draw all meshes in the submit list. Do we want to support multiple frames? What if we want to render split-screen? Could differend threads prepare different frames?
		m_renderer->Frame(window, cam0);
#ifdef NO_GUI
		m_renderer->Present(window, nullptr);
#else
		m_renderer->Present(window, this);
#endif // NO_GUI

	}

#ifdef DO_TESTING
	if (m_nFrames == 1) {
		std::string filename = m_shaderSettings[m_currentShaderDefineTestCase].name + ".png";
		m_renderer->SaveLastFrame("TestData/Images/" + m_currentSceneName.substr(0, m_currentSceneName.find_last_of(".")) + "/" + filename);
	}
	m_nFrames++;

	if (m_nFrames > m_nWarmUpFrames + 1000u) {
		m_nFrames = 0;
		
		if (m_currentSceneName != "") {
			
			int nValues;
			int firstValue;
			double* timerValues = m_renderer->GetGPU_Timers(nValues, firstValue);
			double average = 0;
			std::filesystem::create_directories("TestData/Data");

			std::string fileName = m_currentSceneName.substr(0, m_currentSceneName.find_last_of("."));
			fileName += "#" + std::to_string(m_currentShaderDefineTestCase);
			std::ofstream outF("TestData/Data/" + fileName + ".data");
			outF << m_shaderSettings[m_currentShaderDefineTestCase].name << "\n";

			int index;
			for (int i = 0; i < nValues; i++)
			{
				index = (i + firstValue) % nValues;
				outF << timerValues[index] << "\n";
				average += timerValues[index];
			}
			outF.close();
			average /= nValues;
			std::cout << "Average: " << average << "\n";
			
			m_currentShaderDefineTestCase++;
			if (m_currentShaderDefineTestCase >= m_shaderSettings.size()) {
				m_currentShaderDefineTestCase = 0;
				m_currentTestSceneIndex++;

				if (m_currentTestSceneIndex < m_TestScenes.files.size()) {	
					ClearScene();
					std::filesystem::path scenePath = m_TestScenes.files[m_currentTestSceneIndex].path;
					m_currentSceneName = scenePath.filename().string();
					std::cout << "Loading Scene #" << m_currentTestSceneIndex << " : " << m_currentSceneName << "...";
					if (!LoadScene(scenePath, false)) {
						std::cout << "Failed To Load Scene\n";
						m_currentSceneName = "-Failed-" + std::to_string(m_currentTestSceneIndex);
					}
					std::cout << "OK.      ";
				}
				else {
					m_currentTestSceneIndex = 0;

					FileSystem::Directory dataFiles;
					FileSystem::ListDirectory(dataFiles, "TestData/Data", {".data"});
					std::ifstream* iFiles = new std::ifstream[dataFiles.files.size()];
					std::ofstream oFile(dataFiles.path.string() + "/data.MergedData");
					for (size_t i = 0; i < dataFiles.files.size(); i++)
					{
						iFiles[i] = std::ifstream(dataFiles.files[i].path);
					}

					//oFile << "\n";
					std::string line;
					for (size_t i = 0; i <= nValues; i++)
					{
						for (size_t i = 0; i < dataFiles.files.size(); i++)
						{
							std::getline(iFiles[i], line);
							oFile << line << "\t";
						}
						oFile << "\n";
					}

					for (size_t i = 0; i < dataFiles.files.size(); i++)
					{
						iFiles[i].close();
					}
					oFile.close();
					delete[] iFiles;

					GenerateGnuPlotScript(dataFiles.path.parent_path().string() + "/Scripts", dataFiles.path);
					exit(0);
				}
			}

		}
		else {
			m_currentShaderDefineTestCase = 0;
			m_currentTestSceneIndex = 0;

			ClearScene();
			std::filesystem::path scenePath = m_TestScenes.files[m_currentTestSceneIndex].path;
			m_currentSceneName = scenePath.filename().string();
			std::cout << "Loading Scene #" << m_currentTestSceneIndex << " : " << m_currentSceneName << "...";
			if (!LoadScene(scenePath, false)) {
				std::cout << "Failed To Load Scene\n";
				m_currentSceneName = "-Failed-" + std::to_string(m_currentTestSceneIndex);
			}
			std::cout << "OK.      ";
		}

		ReloadShaders(m_shaderSettings[m_currentShaderDefineTestCase].shaderDefines);
	}
#endif // DO_TESTING

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
	if(ImGui::Checkbox("Mirror Scene", &m_mirrorScene)) {
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

	ImGui::Columns(3);
	ImGui::Text("Available Blueprints");
	ImGui::NextColumn();
	ImGui::Text("Scene Objects");
	ImGui::NextColumn();
	ImGui::Text(("Selected Objects: " + std::to_string(m_selectedObjects.size())).c_str());
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
				for (int a = min; a <= max; a++)
				{
					m_selectedObjects.push_back(a);
				}
			}
			else if (!m_selectedObjects.empty() && Window::GetGlobalWindowInputHandler().IsKeyDown(WindowInput::KEY_CODE_CTRL)) {
				if (!Contains<std::vector, int>(m_selectedObjects, i)) {
					m_selectedObjects.push_back(i);
				}
			}
			else {
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
			for (auto i : m_selectedObjects)
			{
				Object* obj = new Object();
				memcpy(obj, m_objects[i], sizeof(Object));
				m_objects.push_back(obj);
			}
			m_selectedObjects.clear();
			for (size_t i = size; i < size + nSelected; i++)
			{
				m_selectedObjects.push_back(i);
			}
		}
		
		ImGui::SameLine();
		
		if (ImGui::Button("Delete")) {
			std::sort(m_selectedObjects.begin(), m_selectedObjects.end(), std::greater <int>());
			for (auto i : m_selectedObjects)
			{
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
				ImGui::DragFloat3("Pos", (float*)& m_objects[selectedObject]->transform.pos, 0.1, -100, 100);
				ImGui::DragFloat3("Rot", (float*)& m_objects[selectedObject]->transform.rotation, 0.1, -100, 100);
				ImGui::DragFloat3("Scale", (float*)& m_objects[selectedObject]->transform.scale, 0.1, -100, 100);
			}
		}
		else{

			if (ImGui::Button("Reset Rotation")) {
				for (auto i : m_selectedObjects)
				{
					m_objects[i]->transform.rotation = { 0,0,0 };
				}
			}

			if (ImGui::Button("Random X-Rot")) {
				for (auto i : m_selectedObjects)
				{
					m_objects[i]->transform.rotation.x = (rand() / (float)RAND_MAX) * 2 * 3.15;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Random Y-Rot")) {
				for (auto i : m_selectedObjects)
				{
					m_objects[i]->transform.rotation.y = (rand() / (float)RAND_MAX) * 2 * 3.15;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Random Z-Rot")) {
				for (auto i : m_selectedObjects)
				{
					m_objects[i]->transform.rotation.z = (rand() / (float)RAND_MAX) * 2 * 3.15;
				}
			}

			/////////////

			if (ImGui::Button("Reset Position")) {
				for (auto i : m_selectedObjects)
				{
					m_objects[i]->transform.pos = { 0,0,0 };
				}
			}

			if (ImGui::Button("Random X-Pos")) {
				for (auto i : m_selectedObjects)
				{
					m_objects[i]->transform.pos.x = (rand() / (float)RAND_MAX) * 40 - 20;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Random Y-Pos")) {
				for (auto i : m_selectedObjects)
				{
					m_objects[i]->transform.pos.y = (rand() / (float)RAND_MAX) * 40 - 20;
				}
			}

			ImGui::SameLine();

			if (ImGui::Button("Random Z-Pos")) {
				for (auto i : m_selectedObjects)
				{
					m_objects[i]->transform.pos.z = (rand() / (float)RAND_MAX) * 40 - 20;
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

				if (ImGui::BeginMenu("Load Settings")) {
					ImGui::Checkbox("Keep Kamera", &m_loadSettingkeepKamera);
					ImGui::EndMenu();
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
	
#ifndef DO_TESTING
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
#else
	ImGui::SetNextWindowSize(ImVec2(300, 0), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Test", NULL, 
		ImGuiWindowFlags_NoCollapse | 
		ImGuiWindowFlags_NoDecoration |
		ImGuiWindowFlags_NoTitleBar	
	))
	{
		ImGui::Text(("Scene: " + m_currentSceneName).c_str());
		ImGui::Text(("Shader: " + m_shaderSettings[m_currentShaderDefineTestCase].name).c_str());
	}
	ImGui::End();
#endif // DO_TESTING

}
void Game::MirrorScene(int lvl)
{
	int n_mirrored = 0;
	float dx = 44.587;
	float dz = 48.785;

	int fx;
	int fz;

	for (auto e : m_objects_mirrored) {
		delete e;
	}
	m_objects_mirrored.clear();

	for (int x = -lvl; x <= lvl; x++)
	{
		for (int z = -lvl; z <= lvl; z++)
		{
			if (x == 0 && z == 0) {
				continue;
			}

			fx = (((x + lvl * 2) % 2) * 2 - 1) * -1;
			fz = (((z + lvl * 2) % 2) * 2 - 1) * -1;

			for (auto& o : m_objects)
			{
				Object* m = nullptr;
				if (n_mirrored < m_objects_mirrored.size()) {
					m = m_objects_mirrored[n_mirrored];
				}
				else {
					m = new Object();
					m_objects_mirrored.push_back(m);
				}
				m->blueprint = o->blueprint;
				m->transform = o->transform;
				m->transform.scale *= Float3(fx, 1, fz); //only work for 3x3
				m->transform.pos *= Float3(fx, 0, fz);
				m->transform.pos += Float3(dx * x, 0, dz * z);
				
				n_mirrored++;
			}
		}
	}


	//while (n_mirrored < m_objects_mirrored.size())
	//{
	//	Object* o = m_objects_mirrored.back();
	//	delete o;
	//	m_objects_mirrored.erase(m_objects_mirrored.end());
	//}
	
}

#ifdef DO_TESTING
void Game::GenerateGnuPlotScript(const std::filesystem::path& scriptPath, const std::filesystem::path& dataPath)
{
	//scriptPath = scriptPath.lexically_normal();
	//dataPath = dataPath.lexically_normal();
	//std::filesystem::path::preferred_separator = "";
	GenerateGnuPlotScript_full(scriptPath, dataPath);
	GenerateGnuPlotScript_perScene(scriptPath.string()  + "/PerScene", dataPath);
	GenerateGnuPlotScript_perShader(scriptPath.string() + "/PerShader", dataPath);
}
void Game::GenerateGnuPlotScript_full(const std::filesystem::path& scriptPath, const std::filesystem::path& dataPath)
{
	std::filesystem::create_directories(scriptPath);
	std::string dataLexPath = dataPath.lexically_relative(scriptPath).string();
	std::replace(dataLexPath.begin(), dataLexPath.end(), '\\', '/');

	std::string common;
	common += "###!!!                     Autogenerated file                       !!!\n";
	common += "###!!!Any changes made to this file may be overwritten automatically!!!\n";
	common += "\n";
	common += "reset\n";
	common += "file=\"" + dataLexPath + "/data.MergedData\"\n";
	common += "set title \"\" font \", 16\"\n";
	common += "set xlabel \"Frame Number\"\n";
	common += "set ylabel \"Frame Draw Time (ms)\"\n";
	common += "set format x \"%.f frame\"\n";
	common += "set format y \"%.3f ms\"\n";
	common += "set grid ytics mytics  # draw lines for each ytics and mytics\n";
	common += "set mytics 0.1         # set the spacing for the mytics\n";
	common += "set grid\n";
	common += "set key horizontal noinvert left\n";
	common += "set yrange [0:]\n";

	//Plot All
	std::ofstream oFile(scriptPath.string() + "/generated_plotAll.gp");
	oFile << common;
	oFile << "plot \\\n";

	size_t s = m_shaderSettings.size();
	std::string line;
	for (size_t i = 0; i < m_TestScenes.files.size(); i++)
	{
		line = "";
		line += "for[i=1:" + std::to_string(s) + "]";
		while (line.length() < 15)
		{
			line += " ";
		}
		line += "file using i+" + std::to_string(i * s);
		while (line.length() < 35)
		{
			line += " ";
		}	
		line += " w lines lc i t columnheader,\\\n";
		oFile << line;
	}
	oFile.close();


}
void Game::GenerateGnuPlotScript_perScene(const std::filesystem::path& scriptPath, const std::filesystem::path& dataPath)
{
	std::filesystem::create_directories(scriptPath);

	std::string dataLexPath = dataPath.lexically_relative(scriptPath).string();
	std::replace(dataLexPath.begin(), dataLexPath.end(), '\\', '/');

	std::string common;
	common += "###!!!                     Autogenerated file                       !!!\n";
	common += "###!!!Any changes made to this file may be overwritten automatically!!!\n";
	common += "\n";
	common += "reset\n";
	common += "set xlabel \"Frame Number\"\n";
	common += "set ylabel \"Dispatch Time (ms)\"\n";
	common += "set format x \"%.f frame\"\n";
	common += "set format y \"%.3f ms\"\n";
	common += "set grid ytics mytics  # draw lines for each ytics and mytics\n";
	common += "set mytics 0.1         # set the spacing for the mytics\n";
	common += "set grid\n";
	common += "set key horizontal noinvert left\n";
	common += "set yrange [0:10]\n";
	common += "set yrange [0:]\n";
	//common += "file=\"" + dataLexPath + "/data.MergedData\"\n";

	//Plot per Scene
	size_t s = m_shaderSettings.size();
	for (size_t i = 0; i < m_TestScenes.files.size(); i++)
	{
		std::ofstream oFile(scriptPath.string() + "/generated_LinePlot_Scene_" + std::to_string(i) + ".gp");
		oFile << common;
		oFile << "set title \"Dispatch Times for scene: " + m_TestScenes.files[i].path.stem().string() + "\" font \", 16\"\n";
		oFile << "plot \\\n";
		std::string line;
		for (size_t j = 0; j < s; j++)
		{
			line = "";
			line += "\"" + dataLexPath + "/" + m_TestScenes.files[i].path.stem().string() + "#" + std::to_string(j) + ".data\"" + " using " + std::to_string(1);
			while (line.length() < 20)
			{
				line += " ";
			}
			line += " w lines lc "+ std::to_string(j) + " t \"shader-" + m_shaderSettings[j].name + "\",\\\n";
			oFile << line;
		}
		oFile.close();
	}

	/////////////////////////////
	/*BoxPlot*/
	common.clear();
	common += "###!!!                     Autogenerated file                       !!!\n";
	common += "###!!!Any changes made to this file may be overwritten automatically!!!\n";
	common += "\n";
	common += "reset\n";
	common += "set xlabel \"Shader Name\"\n";
	common += "set ylabel \"Dispatch Time (ms)\"\n";
	common += "set boxwidth 0.05\n";
	common += "set style data histogram\n";
	common += "set style histogram cluster\n";
	common += "set style boxplot nooutliers\n";

	//Non Common
	common += "set xtics (";
	for (size_t i = 0; i < s; i++)
	{
		common += "\" " + m_shaderSettings[i].name + "\" " + std::to_string(i * 0.1);
		common += ((i == s - 1) ? ")" : ",");
	}
	common += "\n";
	//common += "file=\"" + dataLexPath + "/data.MergedData\"\n";
	common += "set xrange [-0.1:" + std::to_string(s * 0.1f) + "]\n";
	common += "set yrange [0:]\n";

	for (size_t j = 0; j < m_TestScenes.files.size(); j++)
	{
		std::ofstream oFile(scriptPath.string() + "/generated_BoxPlot_Scene_" + std::to_string(j) + ".gp");
		oFile << common;
		oFile << "set title \"Dispatch Times for scene: " + m_TestScenes.files[j].path.stem().string() + "\" font \", 16\"\n";
		oFile << "plot \\\n";
		std::string line;
		for (size_t i = 0; i < s; i++)
		{
			line = "";
			line += "\"" + dataLexPath + "/" + m_TestScenes.files[j].path.stem().string() + "#" + std::to_string(i) + ".data\"" + " using (" + std::to_string(0.1 * i) + "):" + std::to_string(1) + ":xticlabels(1)";
			//line += "file using (" + std::to_string(0.1 * i) + "):" + std::to_string(i + j * s + 1) + ":xticlabels(1)";
			line += " w boxplot t \"" + m_shaderSettings[i].name + "\",\\\n";
			oFile << line;
		}
		oFile.close();
	}
}
void Game::GenerateGnuPlotScript_perShader(const std::filesystem::path& scriptPath, const std::filesystem::path& dataPath)
{
	std::filesystem::create_directories(scriptPath);

	std::string dataLexPath = dataPath.lexically_relative(scriptPath).string();
	std::replace(dataLexPath.begin(), dataLexPath.end(), '\\', '/');

	std::string common;
	/*LinePlot*/
	common += "###!!!                     Autogenerated file                       !!!\n";
	common += "###!!!Any changes made to this file may be overwritten automatically!!!\n";
	common += "\n";
	common += "reset\n";
	common += "set xlabel \"Frame Number\"\n";
	common += "set ylabel \"Dispatch Time (ms)\"\n";
	common += "set format x \"%.f frame\"\n";
	common += "set format y \"%.3f ms\"\n";
	common += "set grid ytics mytics  # draw lines for each ytics and mytics\n";
	common += "set mytics 0.1         # set the spacing for the mytics\n";
	common += "set grid\n";
	common += "set key horizontal noinvert left\n";
	
	size_t s = m_shaderSettings.size();

	//Plot per Shader
	for (size_t j = 0; j < s; j++)
	{
		std::ofstream oFile(scriptPath.string() + "/generated_LinePlot_Shader_" + std::to_string(j) + ".gp");
		oFile << common;
		oFile << "set title \"Dispatch Times for shader: " + m_shaderSettings[j].name + "\" font \", 16\"\n";
		oFile << "plot \\\n";
		std::string line;
		for (size_t i = 0; i < m_TestScenes.files.size(); i++)
		{
			line = "";
			//line += "file using " + std::to_string(i * s + j + 1);
			line += "\"" + dataLexPath + "/" + m_TestScenes.files[i].path.stem().string() + "#" + std::to_string(j) + ".data\"" + " using " + std::to_string(1);
			while (line.length() < 20)
			{
				line += " ";
			}
			line += " w lines lc " + std::to_string(i) + " t \"" + m_TestScenes.files[i].path.stem().string() + "\",\\\n";
			oFile << line;
		}
		oFile.close();
	}

	/////////////////////////////
	/*BoxPlot*/
	common.clear();
	common += "###!!!                     Autogenerated file                       !!!\n";
	common += "###!!!Any changes made to this file may be overwritten automatically!!!\n";
	common += "\n";
	common += "reset\n";
	common += "set xlabel \"Scene Name\"\n";
	common += "set ylabel \"Dispatch Time (ms)\"\n";
	common += "set boxwidth 0.05\n";
	common += "set style data histogram\n";
	common += "set style histogram cluster\n";
	common += "set style boxplot nooutliers\n";

	//Non Common
	common += "set xtics (";
	for (size_t i = 0; i < m_TestScenes.files.size(); i++)
	{
		common += "\"Scene " + std::to_string(i) + "\" " + std::to_string(i * 0.1);
		common += ((i == m_TestScenes.files.size() - 1) ? ")" : ",");
	}
	common += "\n";
	common += "set xrange [-0.1:" + std::to_string(m_TestScenes.files.size() * 0.1f) + "]\n";
	common += "set yrange [0:]\n";

	for (size_t j = 0; j < s; j++)
	{
		std::ofstream oFile(scriptPath.string() + "/generated_BoxPlot_Shader_" + std::to_string(j) + ".gp");
		oFile << common;
		oFile << "set title \"Dispatch Times for shader: " + m_shaderSettings[j].name + "\" font \", 16\"\n";
		oFile << "plot \\\n";
		std::string line;
		for (size_t i = 0; i < m_TestScenes.files.size(); i++)
		{
			line = "";
			//line += "file using (" + std::to_string(0.1 * i) + "):" + std::to_string(i * s + j + 1) 
				+ ":xticlabels(1)";
			//line += " w boxplot t \"Scene-" + std::to_string(i) + "\",\\\n";

			line += "\"" + dataLexPath + "/" + m_TestScenes.files[i].path.stem().string() + "#" + std::to_string(j) + ".data\"" + " using (" + std::to_string(0.1 * i) + "):" + std::to_string(1) + ":xticlabels(1)";
			//line += "file using (" + std::to_string(0.1 * i) + "):" + std::to_string(i + j * s + 1) + ":xticlabels(1)";
			line += " w boxplot t \"" + m_TestScenes.files[i].path.stem().string() + "\",\\\n";
			oFile << line;
		}
		oFile.close();
	}
}
#endif
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
	
	if (m_mirrorScene) {
		outFile << "Mirror\n" << m_mirrorLevel << "\n";
	}
	
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

bool Game::PreLoadScene(const std::filesystem::path& path, Asset_Types assets_to_load_flag)
{
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

	while (std::getline(inFile, line))
	{
		if (line[0] == '#') {
			continue;
		}
		else if (line == "Objects") {
			//Load Objects
			inFile >> tempSizeT;
			inFile.ignore();
			for (size_t i = 0; i < tempSizeT; i++)
			{
				std::getline(inFile, line);
				if (m_rm->PreLoadBlueprint(line, assets_to_load_flag)) {
					allGood = false;
				}
				for (size_t i = 0; i < 3; i++)
				{
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
	while (std::getline(inFile, line))
	{
		if (line[0] == '#') {
			continue;
		}
		else if (line == "Mirror") {
			m_mirrorScene = true;
			inFile >> m_mirrorLevel;
		}
		else if (line == "Cameras" && !m_loadSettingkeepKamera) {

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
#ifdef DO_TESTING
	FileSystem::ListDirectory(m_TestScenes, m_sceneFolderPath + "TestScenes/", { ".scene" });
#endif // DO_TESTING
	FileSystem::ListDirectory(m_foundScenes, m_sceneFolderPath, { ".scene" });
	FileSystem::ListDirectory(m_foundBluePrints, "../../Exported_Assets/" + std::string(BLUEPRINT_FOLDER_NAME), { ".bp" });
	FileSystem::ListDirectory(m_foundMeshes, "../../Exported_Assets/" + std::string(MESH_FOLDER_NAME), { ".obj" });
	FileSystem::ListDirectory(m_foundTextures, "../../Exported_Assets/" + std::string(TEXTURE_FODLER_NAME), { ".png" });
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

	if (m_def_CLOSEST_HIT_ALPHA_TEST_1) {
		defines.push_back({ L"CLOSEST_HIT_ALPHA_TEST_1"});
	}

	if (m_def_CLOSEST_HIT_ALPHA_TEST_2) {
		defines.push_back({ L"CLOSEST_HIT_ALPHA_TEST_2"});
	}

	if (m_def_TRACE_NON_OPAQUE_SEPARATELY) {
		defines.push_back({ L"TRACE_NON_OPAQUE_SEPARATELY" });
	}

	if (m_def_RAY_GEN_ALPHA_TEST) {
		defines.push_back({ L"RAY_GEN_ALPHA_TEST" });
	}

	if (m_def_DEBUG_RECURSION_DEPTH) {
		defines.push_back({ L"DEBUG_RECURSION_DEPTH" });
		
		if (m_def_DEBUG_RECURSION_DEPTH_MISS_ONLY) {
			defines.push_back({ L"DEBUG_RECURSION_DEPTH_MISS_ONLY" });
		}

		if (m_def_DEBUG_RECURSION_DEPTH_HIT_ONLY) {
			defines.push_back({ L"DEBUG_RECURSION_DEPTH_HIT_ONLY" });
		}
	}

	if (m_def_DEBUG_DEPTH) {
		defines.push_back({ L"DEBUG_DEPTH" });
		defines.push_back({ L"DEBUG_DEPTH_EXP", std::to_wstring(m_def_DEBUG_DEPTH_EXP) });
	}

	m_renderer->Refresh(&defines);
}

void Game::ReloadShaders(const std::vector<ShaderDefine>& defines)
{
	m_renderer->Refresh(&defines);
}
