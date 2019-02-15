#include <vector>

//class Renderer;

struct Float3
{
	union
	{
		float x, r;
	};
	union
	{
		float y, g;
	};
	union
	{
		float z, b;
	};
};
struct Int2
{
	union
	{
		int x, u, s;
	};
	union
	{
		int y, v, t;
	};
};

/*
	Contain a texture that could be applied to a mesh.
*/
class Texture {
public:
	virtual bool LoadFromFile(const char*)							= 0;
private:
	Texture();
};

/*
	Describes the material and what shaders needed to render this material.
	Could contain data like how reflective the material is etc.
*/
class Material {
public:
	/*		
		@param textureList, a pointer to a global texture list.

		If textureList == nullptr, no textures will be loaded from the material info. 
		Otherwise the textures will be loaded, only if not found in the "textureList" param, and added to the material.

		@return true if Material was loaded successfully.
	*/
	virtual bool LoadFromFile(const char*, std::vector<Texture*>* textureList = nullptr)		= 0;
	//virtual void AddTexture(Texture*);

private:
	//std::vector<Texture*> textures;
};

/*
	Specific rendering states goes here like Wireframe, conservative rasterization etc.
*/
class RenderState {
private:
	bool wireframe;
	//bool useDepthBuffer; //Should this be here maybe?
};

/*
	Describes how a mesh should be rendered.
	Contains a collection of shader data(Material) and specific render states(RenderState)
*/
class Technique {
private:
	Technique(Material*, RenderState*);
	Material* mat;
	RenderState* renderState;
};

/*
	Used to contain a model.
	Basicly just a collection of related vertexbuffers.
*/
class Mesh {
public:
	struct Polygon {
		Float3 p[3];
	};

	/*
		Initialize mesh from file. This function will fill all model vertex data into buffers but skip loading the material and textures related to the model. To load the Material and textures 
		@see Texture
		and
		@see Material


		If called there is not further need to call any initialize function.
		@param fileName, name of the file to load.

		@return true if file was loaded successfully
	*/
	virtual bool LoadFromFile(const char* fileName)														= 0;
	
	// WARNING: Multiple meshes can initilize the same shape.
	// It is the programmer's responsibility to only initialize 1 of each mesh shape and use it multiple times instead.
	
	/*
		Initialize mesh from with the model of a cube. This function will fill all model vertex data into buffers. To load the Material and textures,
		@see Texture
		and
		@see Material
	*/
	virtual void InitializeCube()																		= 0;
	virtual void InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections)	= 0;
	virtual void InitializePolygonList(const std::vector<Polygon>& polygons)							= 0;

	//virtual void SetTechnique(Technique*);
	//virtual const char* GetMaterialName();
private:
	bool isCreated;	// vertexbuffers.size() > 0 ish maybe ? idk
	//std::vector<VertexBuffer*> vertexBuffers;
	//Technique* tech; //Moved To blueprint
};

class Window {
public:
	virtual void SetDimensions(Int2 dimensions)	= 0;
	virtual void SetDimensions(int w, int h)	= 0;
	virtual void SetPosition(Int2 position)		= 0;
	virtual void SetPosition(int x, int y)		= 0;
	virtual void SetTitle(const char* title)	= 0; /*Change the title of the window*/
	virtual bool Create()						= 0; /*This is where the window is actually created*/
	virtual void Show()							= 0; /*Show the window*/
	virtual void Hide()							= 0; /*Hide the window*/
	virtual void HandleWindowEvents()			= 0; /*Should be called every frame to handle the window events.*/
private:
	Window();
	Int2 dimensions;
	Int2 position;
	//unsigned int handle;
	const char* title;

};

struct Transform {
	Float3 pos;
	//Float3 rotation; //rotation should be represented as a quaternion insteed of Float3.
	Float3 scale;
};

class Camera {
public:
	virtual void SetPosition(Float3 position)																= 0;
	virtual void SetTarget(Float3 target)																	= 0;
	virtual void SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane)	= 0;
	virtual void SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane)		= 0;

	Float3 GetPosition() const;
	Float3 GetTarget() const;
	Float3 GetTargetDirection() const;

	bool HasViewChanged() const;
	
private:
	int id;
	int viewMatrixIndex; //Move?
	int perspectiveMatrixIndex;
	bool hasViewChanged;

	Transform transform;
};

struct SubmissionItem {
	Blueprint* mesh;
	Transform transform;
};

/*
	Documentation goes here ^^
*/
class Renderer {
public:
	enum class RendererBackend
	{
		D3D11,
		D3D12,
		Vulcan,
		OpenGL
	};

	/*
		Call to create a instance of a specific renderer class used through out the game.

		@param backend directly specifies which type of renderer to return.

		@return a Renderer* to selected by the RendererBackend param
	*/
	static	Renderer*		MakeRenderer(RendererBackend backend);

	//Builder functions to create classes specific to the renderer instance created by MakeRenderer();
#pragma region Renderers Builder Functions
	virtual Camera*			MakeCamera() = 0;
	virtual Window*			MakeWindow() = 0;
	virtual Texture*		MakeTexture() = 0;
	virtual Mesh*			MakeMesh() = 0;
	virtual Material*		MakeMaterial() = 0;
	virtual RenderState*	MakeRenderState() = 0;
	virtual Technique*		MakeTechnique(Material*, RenderState*) = 0;
#pragma endregion


	virtual void			Submit(SubmissionItem item)				= 0; //How will this work with multi-threaded submissions? Should we submit an "Entity"-class insteed of a "Mesh"-class?
	virtual void			ClearSubmissions()						= 0; //Should we have this?
	virtual void			Frame()									= 0; //How will this work with multi-threading? One thread to rule them all?
	virtual void			Present()								= 0; //How will this work with multi-threading? One thread to rule them all?
	virtual void			ClearFrame()							= 0; //How will this work with multi-threading?
};

/*
	Contain data used to describe a object and how it should be rendered.
	This class should ONLY be used as a blueprint/prefab to create other object copying data from this class.
*/
class Blueprint {
public:
	Mesh*					mesh;
	Technique*				technique;
	std::vector<Texture*>	textures;
};

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

//class Buffer;//Not used for now



/*This main is only an exemple of how this API could/should be used to render a scene.*/
int main() {

//Initialize renderer and window. Maybe we should give more options here to set things like forward/deferred rendering, fullscreen etc.
#pragma region Initialize renderer and window
	Renderer* renderer = Renderer::MakeRenderer(Renderer::RendererBackend::D3D12);	//Specify Forward or Deferred Rendering?
	//renderer->InitForwardRendering();				//Init like this?
	//renderer->InitDeferredRendering();			//Init like this?

	//Init Window. if the window is created this way, how should the rendertarget dimensions be specified? 
	Window*	window = renderer->MakeWindow();
	window->SetDimensions(640, 640);
	window->SetTitle("Renderer API");
	window->Create();
	window->Show();
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