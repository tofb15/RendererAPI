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

		ShaderManager* sm = renderer->MakeShaderManager();
		ShaderDescription sd = {};


		sd.defines = "";
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
		cam->SetPosition(Float3());
		cam->SetTarget(Float3());
		cam->SetPerspectiveProjection(3.14159265f * 0.5f, 1.0f, 0.01f, 1000.0f);
		cameras.push_back(cam);
#pragma endregion

		//Create all the blueprints for the scene. One blueprint should/could be used to create one or many copies of gameobjects cloneing the appearance of the specific blueprint.
#pragma region CreateUniqueBlueprint
	//Load meshes and materials from file
		Mesh* mesh = renderer->MakeMesh();
		//mesh->LoadFromFile(".obj"); //Vertexbuffer loaded here but should be able to be added seperatly aswell. Should we load material and texture here aswell?
		std::vector<Mesh::Polygon> polygons;
		Mesh::Polygon polygon = { -0.5f, -0.5f, 0.0f, 0.0f, 0.5f, 0.0f, 0.5f, -0.5f, 0.0f };
		polygons.push_back(polygon);
		mesh->InitializePolygonList(polygons);
		meshes.push_back(mesh);

		//Create a material
		Material* mat = renderer->MakeMaterial();
		mat->LoadFromFile(".mtl");
		materials.push_back(mat);



		//Create RenderState
		RenderState* renderState = renderer->MakeRenderState();
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
		Texture* tex = renderer->MakeTexture();
		tex->LoadFromFile(".png");
		textures.push_back(tex);

		//Create the final blueprint. This could later be used to create objects.
		Blueprint* blueprint = new Blueprint;
		blueprint->technique = tech;
		blueprint->mesh = mesh;
		blueprint->textures.push_back(tex);
		blueprints.push_back(blueprint);

#pragma endregion

		Object* object = new Object;
		object->blueprint = blueprint;
		object->transform.scale = { 1.0f, 1.0f, 1.0f };
		object->transform.pos = { 1.0f, 1.0f, 1.0f };

		objects.push_back(object);

		return true;
	}

	void Run()
	{
		//Game Loop
		while (!windows[0]->WindowClosed())
		{
			//Handle window events to detect window movement, window destruction etc. 
			windows[0]->HandleWindowEvents();
			windows[1]->HandleWindowEvents();

			//Render the scene.
#pragma region Render
	//Submit all meshes that should be rendered and the transformation on the mesh.
		//for (size_t i = 0; i < objects.size(); i++)
		//{
		//	//Submit one mesh that should be rendered and the transformation on the mesh.
		//	renderer->Submit({ objects[i].blueprint, objects[i].transform });
		//}

			renderer->ClearSubmissions();

			for (size_t i = 0; i < objects.size(); i++)
			{
				renderer->Submit({ objects[i]->blueprint, objects[i]->transform });
			}


			renderer->Frame(windows[0]);	//Draw all meshes in the submit list. Do we want to support multiple frames? What if we want to render split-screen? Could differend threads prepare different frames?
			renderer->Present(windows[0]);//Present frame to screen

			renderer->Frame(windows[1]);	//Draw all meshes in the submit list. Do we want to support multiple frames? What if we want to render split-screen? Could differend threads prepare different frames?
			renderer->Present(windows[1]);//Present frame to screen

#pragma endregion
		}
	}

private:
	Renderer*					renderer;
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
};

/*This main is only an exemple of how this API could/should be used to render a scene.*/
int main() {
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);

	Game game;
	game.Initialize();
	game.Run();

	return 0;
}