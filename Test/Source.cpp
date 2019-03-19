#include "Renderer.hpp"
#include "Window.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Technique.hpp"
#include "RenderState.hpp"
#include "Texture.hpp"
#include "ShaderManager.hpp"
#include "Terrain.hpp"

#include <iostream>
#include <crtdbg.h>
#include <chrono>


const bool SINGLE_WINDOW = true;

//class Renderer;

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


class Game
{
public:
	Game()
	{

	}
	virtual ~Game()
	{
		for (auto e : m_blueprints) {
			delete e;
		}
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
		for (auto e : m_textures) {
			delete e;
		}
		for (auto e : m_windows) {
			delete e;
		}
		for (auto e : m_techniques) {
			delete e;
		}
		for (auto e : m_meshes) {
			delete e;
		}

		if (m_terrain)
		{
			delete m_terrain;
		}
		delete m_renderer;
		delete m_sm;
	}

	bool Initialize()
	{
		if (!InitializeRendererAndWindow())
		{
			return false;
		}

		InitializeMeshesMaterialsAndRenderStates();

		if (!InitializeShadersAndTechniques())
		{
			return false;
		}
		InitializeCameras();
		InitializeTextures();
		InitializeBlueprints();
		InitializeObjects();
		InitializeHeightMap();

		return true;
	}
	bool InitializeRendererAndWindow()
	{
		//Initialize renderer and window. Maybe we should give more options here to set things like forward/deferred rendering, fullscreen etc.

		m_renderer = Renderer::MakeRenderer(Renderer::RendererBackend::D3D12);	//Specify Forward or Deferred Rendering?
		if (m_renderer == nullptr) {
			std::cout << "Selected rendered backend was not implemented and could therefor not be created." << std::endl;
			exit(-1);
		}
		if (!m_renderer->Initialize())
			return false;
		//renderer->InitForwardRendering();				//Init like this?
		//renderer->InitDeferredRendering();			//Init like this?

		//Init Window. if the window is created this way, how should the rendertarget dimensions be specified? 
		Window* window = m_renderer->MakeWindow();
		window->SetTitle("Window 1");
		if (!window->Create(640, 640))
			return false;
		window->Show();
		m_windows.push_back(window);

		if (!SINGLE_WINDOW)
		{
			Window*	window2 = m_renderer->MakeWindow();
			window2->SetTitle("Window 2");
			if (!window2->Create(640, 640))
				return false;
			window2->Show();
			m_windows.push_back(window2);
		}

		return true;
	}

	void InitializeMeshesMaterialsAndRenderStates()
	{
		Mesh* mesh1 = m_renderer->MakeMesh();
		Mesh* mesh2 = m_renderer->MakeMesh();
		Mesh* mesh3 = m_renderer->MakeMesh();
		Mesh* mesh4 = m_renderer->MakeMesh();
		Mesh* mesh5 = m_renderer->MakeMesh();

		//mesh1->LoadFromFile("walker.obj");
		mesh1->InitializeCube(Mesh::VERTEX_BUFFER_FLAG_POSITION | Mesh::VERTEX_BUFFER_FLAG_NORMAL | Mesh::VERTEX_BUFFER_FLAG_UV);

		mesh2->LoadFromFile("turret.obj");
		mesh3->LoadFromFile("enemy_flying.obj");
		mesh4->LoadFromFile("disc.obj");
		mesh5->LoadFromFile("antenna.obj");

		m_meshes.push_back(mesh1);
		m_meshes.push_back(mesh2);
		m_meshes.push_back(mesh3);
		m_meshes.push_back(mesh4);
		m_meshes.push_back(mesh5);


		//Create a material
		Material* mat = m_renderer->MakeMaterial();
		mat->LoadFromFile("generator.mtl");
		m_materials.push_back(mat);

		//Create RenderState
		RenderState* renderState = m_renderer->MakeRenderState();
		renderState->SetWireframe(false);
		renderState->SetFaceCulling(RenderState::FaceCulling::BACK);
		m_renderStates.push_back(renderState);

		renderState = m_renderer->MakeRenderState();
		renderState->SetWireframe(false);
		renderState->SetFaceCulling(RenderState::FaceCulling::BACK);
		renderState->SetUsingDepthBuffer(true);
		m_renderStates.push_back(renderState);
	}

	bool InitializeShadersAndTechniques()
	{
		m_sm = m_renderer->MakeShaderManager();
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
		Technique* tech = m_renderer->MakeTechnique(m_renderStates[0], &sp, m_sm);
		m_techniques.push_back(tech);

		tech = m_renderer->MakeTechnique(m_renderStates[1], &sp, m_sm);
		m_techniques.push_back(tech);

		return true;
	}
	void InitializeTextures()
	{
		//Create a Texture
		Texture* tex;
		tex = m_renderer->MakeTexture();
		tex->LoadFromFile("../assets/Textures/test3.png", Texture::TEXTURE_USAGE_CPU_FLAG | Texture::TEXTURE_USAGE_GPU_FLAG);
		m_textures.push_back(tex);

		tex = m_renderer->MakeTexture();
		tex->LoadFromFile("../assets/Textures/test4.png", Texture::TEXTURE_USAGE_CPU_FLAG | Texture::TEXTURE_USAGE_GPU_FLAG);
		m_textures.push_back(tex);
	}

	void InitializeBlueprints()
	{
		//Create the final blueprint. This could later be used to create objects.
		Blueprint* blueprint;

		for (size_t nTechs = 0; nTechs < 2; nTechs++)
		{
			for (size_t nMeshes = 0; nMeshes < 5; nMeshes++)
			{
				for (size_t nTextures = 0; nTextures < 2; nTextures++)
				{
					blueprint = new Blueprint;
					blueprint->technique = m_techniques[nTechs];
					blueprint->mesh = m_meshes[nMeshes];
					blueprint->textures.push_back(m_textures[nTextures]);
					m_blueprints.push_back(blueprint);
				}
			}
		}
	}

	void InitializeObjects()
	{
		size_t nBlueprints = m_blueprints.size();
		for (size_t i = 0; i < 10240U; i++)
		{
			Object* object = new Object;
			object->blueprint = m_blueprints[i % nBlueprints];
			object->transform.scale = { 1.0f, 1.0f, 1.0f };
			object->transform.pos = { static_cast<float>(i % 100) * 10, 0.0f, static_cast<float>(i / 100) * 10 };
			m_objects.push_back(object);
		}
	}

	void InitializeHeightMap() {

#pragma region HeightMap
		ShaderDescription sd_terrain = {};
		sd_terrain.defines = "";
		sd_terrain.name = "VertexShader_Terrain";
		sd_terrain.type = ShaderType::VS;
		Shader vs_terrain = m_sm->CompileShader(sd_terrain);
		if (vs_terrain.type == ShaderType::UNKNOWN)
			return;

		sd_terrain.defines = "";
		sd_terrain.name = "GS_Shader_Terrain";
		sd_terrain.type = ShaderType::GS;
		Shader gs_terrain = m_sm->CompileShader(sd_terrain);
		if (gs_terrain.type == ShaderType::UNKNOWN)
			return;

		sd_terrain.defines = "#define NORMAL\n";
		sd_terrain.name = "FragmentShader";
		sd_terrain.type = ShaderType::FS;
		Shader ps_terrain = m_sm->CompileShader(sd_terrain);
		if (ps_terrain.type == ShaderType::UNKNOWN)
			return;

		ShaderProgram sp_terrain;
		sp_terrain.VS = vs_terrain;
		sp_terrain.GS = gs_terrain;
		sp_terrain.FS = ps_terrain;

		Technique* tech_terrain = m_renderer->MakeTechnique(m_renderStates[0], &sp_terrain, m_sm);
		m_techniques.push_back(tech_terrain);

		Texture* tex = m_renderer->MakeTexture();
		tex->LoadFromFile("../assets/Textures/map3.png", Texture::TEXTURE_USAGE_CPU_FLAG);
		m_textures.push_back(tex);

		m_terrain = m_renderer->MakeTerrain();
		m_terrain->InitializeHeightMap(tex, 100);

		m_terrainBlueprint.mesh = m_terrain->GetMesh();
		m_meshes.push_back(m_terrainBlueprint.mesh);
		
		m_terrainBlueprint.technique = tech_terrain;
	}

	void InitializeCameras()
	{
		Camera* cam = m_renderer->MakeCamera();
		cam->SetPosition(Float3(-5, 5, -5));
		cam->SetTarget(Float3(0, 0, 0));
		cam->SetPerspectiveProjection(3.14159265f * 0.5f, 1.0f, 0.01f, 1000.0f);
		m_cameras.push_back(cam);

		cam = m_renderer->MakeCamera();
		cam->SetPosition(Float3(-5, 5, -5));
		cam->SetTarget(Float3(0, 0, 0));
		cam->SetPerspectiveProjection(3.14159265f * 0.5f, 1.0f, 0.01f, 1000.0f);
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

			m_time += dt;

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

			if(m_demoMovement[0])
				m_cameras[0]->SetPosition(Float3(sinf(m_time) * 4.0f, 3.0f, cosf(m_time) * 4.0f));
			if (m_demoMovement[1])
				m_cameras[1]->SetPosition(Float3(sinf(m_time) * 4.0f, 3.0f, cosf(m_time) * 4.0f));

			//timeSincePixChange += 0.05f;
			static unsigned char color = 0;
			static short colorDir = 1;

			if (frameCount % 100 == 0) {
				unsigned width = m_textures[0]->GetWidth();
				unsigned height = m_textures[0]->GetHeight();
				for (unsigned int x = 0; x < width; x++)
				{
					for (unsigned int y = 0; y < height / 2; y++)
					{
						unsigned char data[4] = { static_cast<unsigned char>(0), color, static_cast<unsigned char>(255-color), static_cast<unsigned char>(255) };
						m_textures[0]->UpdatePixel(Int2(x, y), data, 4);
					}
				}
				color += 20 * colorDir;
				if (color == 0 || color == 240)
					colorDir *= -1;

				m_textures[0]->ApplyChanges();
			}

			UpdateObjects(dt);
			RenderWindows();
			
		}
	}
	void UpdateObjects(double dt)
	{
		static double t = 0.0f;
		t += dt;
		if (t > 2.0f * 3.14159265f)
		{
			t -= 2.0f * 3.14159265f;
		}

		for (int i = 0; i < m_objects.size(); i++)
		{
			//objects[i]->transform.scale.y = sin(time * 5 + i) * 2 + 2.5f;
			m_objects[i]->transform.rotation.y = sinf(m_time + i) * cosf(m_time * 2 + i) * 3.14159265f * 2.0f;
		}
	}
	void UpdateInput()
	{
		// This input handler is shared between windows
		WindowInput &input_Global = Window::GetGlobalWindowInputHandler();

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
		WindowInput &input_Global = Window::GetGlobalWindowInputHandler();
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
			techniqueToUse = (techniqueToUse + 1) % 2;
			m_blueprints[0]->technique = m_techniques[techniqueToUse];
			m_blueprints[1]->technique = m_techniques[techniqueToUse];
		}
	}
	void ProcessLocalInput(double dt)
	{
		std::vector<WindowInput*> inputs;

		for (size_t i = 0; i < m_windows.size(); i++)
		{
			inputs.push_back(&m_windows[i]->GetLocalWindowInputHandler());
		}

		for (size_t i = 0; i < m_windows.size(); i++)
		{
			if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_W))
			{
				m_cameras[i]->Move(m_cameras[0]->GetTargetDirection().normalized() * (m_ms * dt));
			}
			if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_S))
			{
				m_cameras[i]->Move(m_cameras[0]->GetTargetDirection().normalized() * -(m_ms * dt));
			}
			if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_A))
			{
				m_cameras[i]->Move(m_cameras[0]->GetRight().normalized() * -(m_ms * dt));
			}
			if (inputs[i]->IsKeyDown(WindowInput::KEY_CODE_D))
			{
				m_cameras[i]->Move(m_cameras[0]->GetRight().normalized() * (m_ms * dt));
			}
		}

		// Rotation is based on delta time
		Int2 mouseMovement = inputs[0]->GetMouseMovement();
		m_cameras[0]->Rotate({ 0, 1, 0 }, mouseMovement.x * dt);
		m_cameras[0]->Rotate(m_cameras[0]->GetRight(), mouseMovement.y * dt);
	}
	void RenderWindows()
	{
		Camera* cam0 = m_cameras[0];
		Camera* cam;
		Window* window;
		Object* obj;

		size_t nWindows = m_windows.size();
		size_t nObjects = m_objects.size();
		for (int i = 0; i < nWindows; i++)
		{
			m_renderer->ClearSubmissions();
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
			m_renderer->Present(window);
		}
	}

private:
	Renderer*					m_renderer;
	ShaderManager*				m_sm;
	std::vector<Window*>		m_windows;

	//Globals. Since these vectors are used by all games using this API, should these maybe we its own class called something like "SceneManager"?

	std::vector<Blueprint*>		m_blueprints;
	std::vector<Mesh*>			m_meshes;
	std::vector<Material*>		m_materials;
	std::vector<Texture*>		m_textures;
	std::vector<Technique*>		m_techniques;
	std::vector<RenderState*>	m_renderStates;
	std::vector<Camera*>		m_cameras;
	std::vector<Object*>		m_objects;
	Terrain* m_terrain;
	Blueprint m_terrainBlueprint;

	double m_time = 0.0;
	double m_ms = 300.0;
	bool m_demoMovement[2];
};

/*This main is only an exemple of how this API could/should be used to render a scene.*/
int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Game game;
	game.Initialize();
	game.Run();

	return 0;
}