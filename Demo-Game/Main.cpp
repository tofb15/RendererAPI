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
#include <crtdbg.h>
#include <chrono>

//#define AT_OFFICE
#define PERFORMACE_TEST
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
#ifdef AT_OFFICE
		m_rm->SetAssetPath("../../ExportedAssets/");
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
		if (!InitializeBlueprints()) {
			return -4;
		}
		InitializeObjects();
		//InitializeHeightMap();

		m_lights.emplace_back();

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

	bool InitializeBlueprints()
	{
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
		return true;
	}

	void InitializeObjects()
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
		for (size_t i = 0; i < 10; i++)
		{
			object = new Object;
			object->blueprint = m_rm->GetBlueprint("tent");
			object->transform.scale = { 1.0f, 1.0f, 1.0f };
			object->transform.pos = { 0, i * 0.1f, 0 };
			m_objects.push_back(object);
		}
#else
		//Map
		object = new Object;
		object->blueprint = m_rm->GetBlueprint("map");
		object->transform.scale = { 1.0f, 1.0f, 1.0f };
		object->transform.pos = { 0, 0, 0 };
		m_objects.push_back(object);

		//Stone
		object = new Object;
		object->blueprint = m_rm->GetBlueprint("concrete");
		object->transform.scale = { 1.0f, 1.0f, 1.0f };
		object->transform.pos = { 0, 0, 0 };
		m_objects.push_back(object);

		//Sandbag
		object = new Object;
		object->blueprint = m_rm->GetBlueprint("sandbag");
		object->transform.scale = { 1.0f, 1.0f, 1.0f };
		object->transform.pos = { 0, 0, 0 };
		m_objects.push_back(object);

		//Floor
		object = new Object;
		object->blueprint = m_rm->GetBlueprint("floor");
		object->transform.scale = { 1.0f, 1.0f, 1.0f };
		object->transform.pos = { 0, 0, 0 };
		m_objects.push_back(object);

		//Tent
		object = new Object;
		object->blueprint = m_rm->GetBlueprint("tent");
		object->transform.scale = { 1.0f, 1.0f, 1.0f };
		object->transform.pos = { 0, 0, 0 };
		m_objects.push_back(object);
#endif //PERFORMACE_TEST

		static constexpr UINT nObjects_X = 1U;
		static constexpr UINT nObjects = nObjects_X * nObjects_X;
		for (size_t i = 0; i < nObjects; i++)
		{
			object = new Object;
			object->blueprint = m_rm->GetBlueprint("tree");
			object->transform.scale = { 1.0f, 1.0f, 1.0f };
			object->transform.pos = {
				(static_cast<float>(i % nObjects_X) - nObjects_X / 2) * 10,
				0.0f,
				(static_cast<float>(i / nObjects_X) - nObjects_X / 2) * 10
			};
			m_objects.push_back(object);
		}
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

	void InitializeHeightMap() {
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

	void InitializeCameras()
	{
		Int2 dim = m_windows[0]->GetDimensions();
		float aspRatio = dim.x / dim.y;

		Camera* cam = m_renderAPI->MakeCamera();
		cam->SetPosition(Float3(-50, 100, -50));
		cam->SetTarget(Float3(100, 20, 100));
		cam->SetPerspectiveProjection(3.14159265f * 0.5f, aspRatio, 0.1f, 2000.0f);
		m_cameras.push_back(cam);

		cam = m_renderAPI->MakeCamera();
		cam->SetPosition(Float3(-5, 5, -5));
		cam->SetTarget(Float3(0, 0, 0));
		cam->SetPerspectiveProjection(3.14159265f * 0.5f, 1.0f, 0.1f, 2000.0f);
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

		Float3 lightCenter(0, 50, 0);
		float lightRad = 50;
		float lightSpeedMulti = 15;
		m_lights.back().setPosition(lightCenter + Float3(cos(m_time_lightAnim * lightSpeedMulti) * lightRad, 0, sin(m_time_lightAnim * lightSpeedMulti) * lightRad));
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
		int techniqueToUse = 0;

		if (input_Global.IsKeyDown(WindowInput::KEY_CODE_1)) {
			m_demoMovement[0] = true;
		}
		if (input_Global.IsKeyDown(WindowInput::KEY_CODE_2)) {
			m_demoMovement[1] = true;
		}
		if (input_Global.IsKeyDown(WindowInput::KEY_CODE_3)) {
			m_demoMovement[0] = m_demoMovement[1] = true;
		}
		if (input_Global.IsKeyDown(WindowInput::KEY_CODE_Q)) {
			//techniqueToUse = (techniqueToUse + 1) % 2;
			//m_blueprints[0]->technique = m_techniques[techniqueToUse];
			//m_blueprints[1]->technique = m_techniques[techniqueToUse];
		}
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

	void RenderSettingWindow() {
		static bool open = true;
		ImGui::SetNextWindowSize(ImVec2(500, 440), ImGuiCond_FirstUseEver);
		if (ImGui::Begin("Scene Settings", &open, ImGuiWindowFlags_MenuBar))
		{
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
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
				if (ImGui::BeginTabItem("Scene"))
				{
					ImGui::Text("Scene Options");
					if (ImGui::Checkbox("Animated Light", &m_animateLight)) {}
					ImGui::SameLine();
					ImGui::DragFloat("Light Anim time", &m_time_lightAnim, 0.001, 0, 1);
					
					if (ImGui::Checkbox("Allow Anyhit Shaders", &m_allowAnyhitShaders)) {
						m_renderer->SetSetting("anyhit", m_allowAnyhitShaders);
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

	void RenderGUI() override {
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

	void ReloadShaders() {
		m_reloadShaders = false;
		std::vector<std::wstring> defines;
		if (m_def_NO_NORMAL_MAP) {
			defines.push_back(L"NO_NORMAL_MAP");
		}

		if (m_def_NO_SHADOWS) {
			defines.push_back(L"NO_SHADOWS");
		}

		if (m_def_NO_SHADING) {
			defines.push_back(L"NO_SHADING");
		}

		if (m_def_CLOSEST_HIT_ALPHA_TEST && !m_allowAnyhitShaders) {
			defines.push_back(L"CLOSEST_HIT_ALPHA_TEST");
		}

		m_renderer->Refresh(&defines);
	}

private:
	RenderAPI* m_renderAPI;
	Renderer* m_renderer;

	ShaderManager* m_sm;
	std::vector<Window*>		m_windows;

	//Globals. Since these vectors are used by all games using this API, should these maybe we its own class called something like "SceneManager"?

	//std::vector<Blueprint*>	m_blueprints;
	//std::vector<Mesh*>		m_meshes;
	//std::vector<Texture*>		m_textures;
	std::vector<Material*>		m_materials;
	std::vector<Technique*>		m_techniques;
	std::vector<RenderState*>	m_renderStates;
	std::vector<Camera*>		m_cameras;
	std::vector<Object*>		m_objects;

	//Terrain* m_terrain;
	Blueprint m_terrainBlueprint;

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
	bool m_reloadShaders = false;
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