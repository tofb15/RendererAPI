#include "Renderer.hpp"
#include "Window.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Technique.hpp"
#include "RenderState.hpp"
#include "Texture.hpp"
#include "ShaderManager.hpp"
#include <iostream>

#include <crtdbg.h>
#include <chrono>

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
		for (auto e : blueprints) {
			delete e;
		}
		for (auto e : materials) {
			delete e;
		}
		for (auto e : techniques) {
			delete e;
		}
		for (auto e : renderStates) {
			delete e;
		}
		for (auto e : cameras) {
			delete e;
		}
		for (auto e : objects) {
			delete e;
		}
		for (auto e : textures) {
			delete e;
		}
		for (auto e : meshes) {
			delete e;
		}
		for (auto e : windows) {
			delete e;
		}

		delete renderer;
		delete sm;
	}

	bool Initialize()
	{
		//Initialize renderer and window. Maybe we should give more options here to set things like forward/deferred rendering, fullscreen etc.
#pragma region Initialize renderer and window
		renderer = Renderer::MakeRenderer(Renderer::RendererBackend::D3D12);	//Specify Forward or Deferred Rendering?
		if (renderer == nullptr) {
			std::cout << "Selected rendered backend was not implemented and could therefor not be created." << std::endl;
			exit(-1);
		}
		renderer->Initialize();																				//renderer->InitForwardRendering();				//Init like this?
		//renderer->InitDeferredRendering();			//Init like this?

		//Init Window. if the window is created this way, how should the rendertarget dimensions be specified? 
		Window* window = renderer->MakeWindow();
		window->SetDimensions(640, 640);
		window->SetTitle("Window 1");
		window->Create();
		window->Show();
		windows.push_back(window);

		Window*	window2 = renderer->MakeWindow();
		window2->SetDimensions(640, 640);
		window2->SetTitle("Window 2");
		window2->Create();
		window2->Show();
		windows.push_back(window2);

		sm = renderer->MakeShaderManager();
		ShaderDescription sd = {};


		sd.defines = "#define NORMAL\n#define TEXTCOORD\n";
		sd.name = "VertexShader";
		sd.type = ShaderType::VS;
		Shader vs = sm->CompileShader(sd);

		sd.name = "FragmentShader";
		sd.type = ShaderType::FS;
		Shader fs = sm->CompileShader(sd);

#pragma endregion

		//Here we create the camera(s) that should/could be used in this scene
#pragma region CreateCamera
	//Create Camera
		Camera* cam = renderer->MakeCamera();
		cam->SetPosition(Float3(0, 5, -5));
		cam->SetTarget(Float3(0, 0, 0));
		cam->SetPerspectiveProjection(3.14159265f * 0.5f, 1.0f, 0.01f, 1000.0f);
		cameras.push_back(cam);

		cam = renderer->MakeCamera();
		cam->SetPosition(Float3(0, 5, -5));
		cam->SetTarget(Float3(0, 0, 0));
		cam->SetPerspectiveProjection(3.14159265f * 0.5f, 1.0f, 0.01f, 1000.0f);
		cameras.push_back(cam);
#pragma endregion

		//Create all the blueprints for the scene. One blueprint should/could be used to create one or many copies of gameobjects cloneing the appearance of the specific blueprint.
#pragma region CreateUniqueBlueprint
	//Load meshes and materials from file
		Mesh* meshRect	= renderer->MakeMesh();
		Mesh* meshCube	= renderer->MakeMesh();
		
		Float3 rect[] = { 
			Float3(-0.5f, -0.5f, 0.0f), Float3(-0.5f, 0.5f, 0.0f), Float3(0.5f, -0.5f, 0.0f),
			Float3(0.5f, -0.5f, 0.0f), Float3(-0.5f, 0.5f, 0.0f), Float3(0.5f, 0.5f, 0.0f),
		};

		int nElems = sizeof(rect) / sizeof(Float3);
		meshRect->AddVertexBuffer(nElems, sizeof(Float3), rect, Mesh::VERTEX_BUFFER_FLAG_POSITION);
		
		meshCube->LoadFromFile("cube_uv.obj");//Vertexbuffer loaded here but should be able to be added seperatly aswell. Should we load material and texture here aswell?
		//meshCube->InitializeCube(Mesh::VERTEX_BUFFER_FLAG_POSITION | Mesh::VERTEX_BUFFER_FLAG_NORMAL | Mesh::VERTEX_BUFFER_FLAG_UV);

		meshes.push_back(meshRect);
		meshes.push_back(meshCube);

		//Create a material
		Material* mat = renderer->MakeMaterial();
		mat->LoadFromFile("generator.mtl");
		materials.push_back(mat);

		//Create RenderState
		RenderState* renderState = renderer->MakeRenderState();
		renderState->SetWireframe(true);
		renderState->SetFaceCulling(RenderState::FaceCulling::NONE);
		renderStates.push_back(renderState);

		renderState = renderer->MakeRenderState();
		renderState->SetWireframe(false);
		renderState->SetFaceCulling(RenderState::FaceCulling::NONE);
		renderState->SetUsingDepthBuffer(true);
		renderStates.push_back(renderState);

		ShaderProgram sp = {};

		sp.VS = vs;
		sp.FS = fs;

		//Create Technique from renderstate and material
		Technique* tech = renderer->MakeTechnique(renderStates[0], &sp, sm);
		techniques.push_back(tech);

		tech = renderer->MakeTechnique(renderStates[1], &sp, sm);
		techniques.push_back(tech);


		//Attach Technique to mesh
		//mesh->SetTechnique(tech); // A mesh could be renderer using more than one Technique. This is set in the blueprint insteed

		//Create a Texture
		Texture* tex;
		tex = renderer->MakeTexture();
		tex->LoadFromFile("../assets/Textures/test3.png", Texture::TEXTURE_USAGE_CPU_FLAG | Texture::TEXTURE_USAGE_GPU_FLAG);
		textures.push_back(tex);

		tex = renderer->MakeTexture();
		tex->LoadFromFile("../assets/Textures/test4.png", Texture::TEXTURE_USAGE_CPU_FLAG | Texture::TEXTURE_USAGE_GPU_FLAG);
		textures.push_back(tex);

		//Create the final blueprint. This could later be used to create objects.
		Blueprint* blueprint = new Blueprint;
		blueprint->technique = techniques[0];
		blueprint->mesh = meshes[0];
		blueprint->textures.push_back(textures[0]);
		blueprints.push_back(blueprint);

		blueprint = new Blueprint;
		blueprint->technique = techniques[0];
		blueprint->mesh = meshes[1];
		blueprint->textures.push_back(textures[1]);
		blueprints.push_back(blueprint);
#pragma endregion

		for (int i = 0; i < 10240; i++)
		{
			Object* object = new Object;
			object->blueprint = blueprints[i % 2];
			object->transform.scale = { 1.0f, 1.0f, 1.0f };
			object->transform.pos = { static_cast<float>(i % 100) * 4, 0.0f, static_cast<float>(i / 100) * 4 };
			objects.push_back(object);
		}

		//object = new Object;
		//object->blueprint = blueprints[1];
		//object->transform.scale = { 1.0f, 1.0f, 1.0f };
		////object->transform.pos = { 1.0f, 1.0f, 1.0f };
		//object->transform.pos = { 0.0f, 0.0f, 0.0f };
		//objects.push_back(object);

		return true;
	}

	void Run()
	{
		//Get the Global input handler for all window. global input handlers is shared by all windows!
		WindowInput &input_Global = Window::GetGlobalWindowInputHandler();
		//Get the input handler for window 1. local input handlers is unique for each window!
		WindowInput &input1 = windows[0]->GetLocalWindowInputHandler();
		//Get the input handler for window 1. local input handlers is unique for each window!
		WindowInput &input2 = windows[1]->GetLocalWindowInputHandler();

		bool demoMovement[2];
		demoMovement[0] = true;
		demoMovement[1] = true;

		int techniqueToUse = 0;

		//Game Loop
		while (!windows[0]->WindowClosed())
		{
			static Time t1, t2;

			static int frame = 0;
			frame++;

			if (frame > 1000)
			{
				t2 = t1;
				t1 = Clock::now();

				std::string str = "FPS: " + std::to_string(1000000 / ((t1 - t2).count() / 1000 / 1000));
				windows[0]->SetTitle(str.c_str());
				frame = 0;
			}

			//Handle window events to detect window movement, window destruction, input etc. 
			input_Global.Reset();//The global input has to be reseted each frame. It is important that this is done before any HandleWindowEvents() is called.
			windows[0]->HandleWindowEvents();
			windows[1]->HandleWindowEvents();

#pragma region INPUT_DEMO
			//Global Input
			if (input_Global.IsKeyDown(WindowInput::KEY_CODE_W)) {
				cameras[0]->Move({0, 0, 0.1});
				cameras[1]->Move({0, 0, 0.1});
				demoMovement[0] = demoMovement[1] = false;
			}
			if (input_Global.IsKeyDown(WindowInput::KEY_CODE_S)) {
				cameras[0]->Move({ 0, 0, -0.1 });
				cameras[1]->Move({ 0, 0, -0.1 });
				demoMovement[0] = demoMovement[1] = false;
			}
			if (input_Global.IsKeyDown(WindowInput::KEY_CODE_A)) {
				cameras[0]->Move({ -0.1, 0, 0 });
				cameras[1]->Move({ -0.1, 0, 0 });
				demoMovement[0] = demoMovement[1] = false;
			}
			if (input_Global.IsKeyDown(WindowInput::KEY_CODE_D)) {
				cameras[0]->Move({ 0.1, 0, 0 });
				cameras[1]->Move({ 0.1, 0, 0 });
				demoMovement[0] = demoMovement[1] = false;
			}
			if (input_Global.IsKeyDown(WindowInput::KEY_CODE_1)) {
				demoMovement[0] = true;
			}
			if (input_Global.IsKeyDown(WindowInput::KEY_CODE_2)) {
				demoMovement[1] = true;
			}
			if (input_Global.IsKeyDown(WindowInput::KEY_CODE_3)) {
				demoMovement[0] = demoMovement[1] = true;
			}
			if (input_Global.IsKeyDown(WindowInput::KEY_CODE_Q)) {
				techniqueToUse = (techniqueToUse+1)%2;
				blueprints[0]->technique = techniques[techniqueToUse];
				blueprints[1]->technique = techniques[techniqueToUse];
			}

			//Window 1
			if (input1.IsKeyDown(WindowInput::KEY_CODE_UP)) {
				cameras[0]->Move({ 0, 0, 0.1 });
				demoMovement[0] = false;
			}
			if (input1.IsKeyDown(WindowInput::KEY_CODE_DOWN)) {
				cameras[0]->Move({ 0, 0, -0.1 });
				demoMovement[0] = false;
			}
			if (input1.IsKeyDown(WindowInput::KEY_CODE_LEFT)) {
				cameras[0]->Move({ -0.1, 0, 0 });
				demoMovement[0] = false;
			}
			if (input1.IsKeyDown(WindowInput::KEY_CODE_RIGHT)) {
				cameras[0]->Move({ 0.1, 0, 0 });
				demoMovement[0] = false;
			}

			//Window 1
			if (input2.IsKeyDown(WindowInput::KEY_CODE_UP)) {
				cameras[1]->Move({ 0, 0, 0.1 });
				demoMovement[1] = false;
			}
			if (input2.IsKeyDown(WindowInput::KEY_CODE_DOWN)) {
				cameras[1]->Move({ 0, 0, -0.1 });
				demoMovement[1] = false;
			}
			if (input2.IsKeyDown(WindowInput::KEY_CODE_LEFT)) {
				cameras[1]->Move({ -0.1, 0, 0 });
				demoMovement[1] = false;
			}
			if (input2.IsKeyDown(WindowInput::KEY_CODE_RIGHT)) {
				cameras[1]->Move({ 0.1, 0, 0 });
				demoMovement[1] = false;
			}
#pragma endregion


			//Render the scene.
#pragma region Rendera

			renderer->ClearSubmissions();

			//for (size_t i = 0; i < objects.size(); i++)
			//{
			//	renderer->Submit({ objects[i]->blueprint, objects[i]->transform });
			//}


			time += 0.05;
			if(demoMovement[0])
				cameras[0]->SetPosition(Float3(sin(time) * 4, 3, cos(time) * 4));
			if (demoMovement[1])
				cameras[1]->SetPosition(Float3(sin(time) * 4, 3, cos(time) * 4));


			//Render Window 1
			renderer->ClearSubmissions();
			for (int i = 0; i < objects.size(); i++)
			{
				renderer->Submit({ objects[i]->blueprint, objects[i]->transform });
			}

			renderer->Frame(windows[0], cameras[0]);	//Draw all meshes in the submit list. Do we want to support multiple frames? What if we want to render split-screen? Could differend threads prepare different frames?
			renderer->Present(windows[0]);//Present frame to screen

			//Render Window 2
			//renderer->ClearSubmissions();
			//for (int i = 0; i < objects.size(); i++)
			//{
			//	renderer->Submit({ objects[i]->blueprint, objects[i]->transform });
			//}

			//renderer->Frame(windows[1], cameras[1]);	//Draw all meshes in the submit list. Do we want to support multiple frames? What if we want to render split-screen? Could differend threads prepare different frames?
			//renderer->Present(windows[1]);//Present frame to screen

#pragma endregion
		}
	}

private:
	Renderer*					renderer;
	ShaderManager* sm;
	std::vector<Window*>		windows;

	//Globals. Since these vectors are used by all games using this API, should these maybe we its own class called something like "SceneManager"?
	std::vector<Blueprint*>		blueprints;
	std::vector<Mesh*>			meshes;
	std::vector<Material*>		materials;
	std::vector<Texture*>		textures;
	std::vector<Technique*>		techniques;
	std::vector<RenderState*>	renderStates;
	std::vector<Camera*>		cameras;
	std::vector<Object*>		objects;

	float time = 0;
};

/*This main is only an exemple of how this API could/should be used to render a scene.*/
int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Game game;
	game.Initialize();
	game.Run();

	return 0;
}