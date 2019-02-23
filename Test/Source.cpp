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
		renderState->SetFaceCulling(RenderState::FaceCulling::BACK);
		renderStates.push_back(renderState);

		ShaderProgram sp = {};

		sp.VS = vs;
		sp.FS = fs;

		//Create Technique from renderstate and material
		Technique* tech = renderer->MakeTechnique(renderState, &sp, sm);
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
		blueprint->technique = tech;
		blueprint->mesh = meshes[1];
		blueprint->textures.push_back(textures[0]);
		blueprints.push_back(blueprint);

		blueprint = new Blueprint;
		blueprint->technique = tech;
		blueprint->mesh = meshes[1];
		blueprint->textures.push_back(textures[1]);
		blueprints.push_back(blueprint);
#pragma endregion

		Object* object = new Object;
		object->blueprint = blueprints[0];
		object->transform.scale = { 1.0f, 1.0f, 1.0f };
		object->transform.pos = { 1.0f, 1.0f, 1.0f };
		objects.push_back(object);

		object = new Object;
		object->blueprint = blueprints[1];
		object->transform.scale = { 1.0f, 1.0f, 1.0f };
		object->transform.pos = { 1.0f, 1.0f, 1.0f };
		objects.push_back(object);

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

		//Game Loop
		while (!windows[0]->WindowClosed())
		{
			//Handle window events to detect window movement, window destruction, input etc. 
			input_Global.Reset();//The global input has to be reseted each frame. It is important that this is done before any HandleWindowEvents() is called.
			windows[0]->HandleWindowEvents();
			windows[1]->HandleWindowEvents();

			//Global Input
			if (input_Global.IsKeyDown(65)) {
				std::cout << "Global" << ": A is down" << std::endl;
			}
			if (input_Global.IsKeyPressed(65)) {
				std::cout << "Global" << ": A was Pressed" << std::endl;
			}
			//Window 1 Input
			if (input1.IsKeyDown(65)) {
				std::cout << windows[0]->GetTitle() << ": A is down" << std::endl;
			}
			if (input1.IsKeyPressed(65)) {
				std::cout << windows[0]->GetTitle() << ": A was Pressed" << std::endl;
			}
			//Window 2 Input
			if (input2.IsKeyDown(65)) {
				std::cout << windows[1]->GetTitle() << ": A is down" << std::endl;
			}
			if (input2.IsKeyPressed(65)) {
				std::cout << windows[1]->GetTitle() << ": A was Pressed" << std::endl;
			}


			//Render the scene.
#pragma region Render

			renderer->ClearSubmissions();

			//for (size_t i = 0; i < objects.size(); i++)
			//{
			//	renderer->Submit({ objects[i]->blueprint, objects[i]->transform });
			//}


			time += 0.001;
			cameras[0]->SetPosition(Float3(sin(time), sin(time)*2, -5));

			//Render Window 1
			renderer->ClearSubmissions();
			renderer->Submit({ objects[0]->blueprint, objects[0]->transform });

			renderer->Frame(windows[0], cameras[0]);	//Draw all meshes in the submit list. Do we want to support multiple frames? What if we want to render split-screen? Could differend threads prepare different frames?
			renderer->Present(windows[0]);//Present frame to screen

			//Render Window 2
			renderer->ClearSubmissions();
			renderer->Submit({ objects[1]->blueprint, objects[1]->transform });

			renderer->Frame(windows[1], cameras[0]);	//Draw all meshes in the submit list. Do we want to support multiple frames? What if we want to render split-screen? Could differend threads prepare different frames?
			renderer->Present(windows[1]);//Present frame to screen

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