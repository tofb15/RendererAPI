#include "Renderer.hpp"
#include "Window.hpp"
#include "Camera.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "Technique.hpp"
#include "Texture.hpp"
#include <iostream>

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
	static Object* CreateObjectFromBlueprint(Blueprint* bp);
};

//aawdawd

/*This main is only an exemple of how this API could/should be used to render a scene.*/
int main() {

//Initialize renderer and window. Maybe we should give more options here to set things like forward/deferred rendering, fullscreen etc.
#pragma region Initialize renderer and window
	Renderer* renderer = Renderer::MakeRenderer(Renderer::RendererBackend::D3D12);	//Specify Forward or Deferred Rendering?
	if (renderer == nullptr) {
		std::cout << "Selected rendered backend was not implemented and could therefor not be created." << std::endl;
		exit(-1);
	}																				
																					//renderer->InitForwardRendering();				//Init like this?
	//renderer->InitDeferredRendering();			//Init like this?

	//Init Window. if the window is created this way, how should the rendertarget dimensions be specified? 
	Window*	window = renderer->MakeWindow();
	window->SetDimensions(640, 640);
	window->SetTitle("Window 1");
	window->Create();
	window->Show();

	Window*	window2 = renderer->MakeWindow();
	window2->SetDimensions(640, 640);
	window2->SetTitle("Window 2");
	window2->Create();
	window2->Show();

	while (true)
	{
		window->HandleWindowEvents();
		window2->HandleWindowEvents();
	}

#pragma endregion

//Globals. Since these vectors are used by all games using this API, should these maybe we its own class called something like "SceneManager"?
#pragma region Globals
	std::vector<Blueprint*>		blueprints;
	std::vector<Mesh*>			meshes;
	std::vector<Material*>		materials;
	std::vector<Texture*>		textures;
	std::vector<Technique*>		techniques;
	std::vector<RenderState*>	renderStates;
	std::vector<Camera*>		cameras;
	std::vector<Object>			objects;
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
	mesh->LoadFromFile(".obj"); //Vertexbuffer loaded here but should be able to be added seperatly aswell. Should we load material and texture here aswell?
	meshes.push_back(mesh);

	//Create a material
	Material* mat = renderer->MakeMaterial();
	mat->LoadFromFile(".mtl");
	materials.push_back(mat);

	//Create RenderState
	RenderState* renderState = renderer->MakeRenderState();
	renderStates.push_back(renderState);

	//Create Technique from renderstate and material
	Technique* tech = renderer->MakeTechnique(mat, renderState);
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
	blueprint->mesh;
	blueprint->textures.push_back(tex);
	blueprints.push_back(blueprint);

#pragma endregion

	//Game Loop
	while (true)
	{
		//Handle window events to detect window movement, window destruction etc. 
		window->HandleWindowEvents();

		//Render the scene.
#pragma region Render
	//Submit all meshes that should be rendered and the transformation on the mesh.
		for (size_t i = 0; i < objects.size(); i++)
		{
			//Submit one mesh that should be rendered and the transformation on the mesh.
			renderer->Submit({ objects[i].blueprint, objects[i].transform });
		}

		renderer->Frame();	//Draw all meshes in the submit list. Do we want to support multiple frames? What if we want to render split-screen? Could differend threads prepare different frames?
		renderer->Present();//Present frame to screen

#pragma endregion
	}



	return 0;
}