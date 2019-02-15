#include <vector>

class Renderer;

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

class Texture {
public:
	virtual bool LoadFromFile(const char*)							= 0;
private:
	Texture();
};

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

class RenderState {
private:
	bool wireframe;
};

class Technique {
private:
	Technique(Material*, RenderState*);
	Material* mat;
	RenderState* renderState;
};

class Mesh {
public:
	struct Polygon {
		Float3 p[3];
	};

	/*
		@param 
		If called there is not further need to call any initialize function.
	*/
	virtual bool LoadFromFile(const char* fileName)														= 0;
	
	// Multiple meshes can initilize a shape.
	// It is the programmer's responsibility to only initialize 1 of each mesh shape and use it multiple times instead.

	virtual void InitializeCube()																		= 0;
	virtual void InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections)	= 0;
	virtual void InitializePolygonList(const std::vector<Polygon>& polygons)							= 0;

	virtual void SetTechnique(Technique*);
	virtual const char* GetMaterialName();
private:
	bool isCreated;	// vertexbuffers.size() > 0 ish maybe ? idk
	//std::vector<VertexBuffer*> vertexBuffers;
	Technique* tech;
};

class Window {
public:
	virtual void SetDimensions(Int2 dimensions)	= 0;
	virtual void SetDimensions(int w, int h)	= 0;
	virtual void SetPosition(Int2 position)		= 0;
	virtual void SetPosition(int x, int y)		= 0;
	virtual void SetTitle(const char* title)	= 0;
	virtual bool Create()						= 0;
	virtual void Show()							= 0;
	virtual void Hide()							= 0;

private:
	Window();
	Int2 dimensions;
	Int2 position;
	//unsigned int handle;
	const char* title;

};

struct Transform {
	Float3 pos;
	//Float4 rotation;
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

class Renderer {
public:
	static	Renderer*		MakeRenderer();
	virtual Camera*			MakeCamera()							= 0;
	virtual Window*			MakeWindow()							= 0;
	virtual Texture*		MakeTexture()							= 0;
	virtual Mesh*			MakeMesh()								= 0;
	virtual Material*		MakeMaterial()							= 0;
	virtual RenderState*	MakeRenderState()						= 0;
	virtual Technique*		MakeTechnique(Material*, RenderState*)	= 0;


	virtual void			Submit(SubmissionItem item)				= 0; //How will this work with multi-threaded submissions? Should we submit an "Entity"-class insteed of a "Mesh"-class?
	virtual void			ClearSubmissions()						= 0; //Should we have this?
	virtual void			Frame()									= 0; //How will this work with multi-threading? One thread to rule them all?
	virtual void			Present()								= 0; //How will this work with multi-threading? One thread to rule them all?
	virtual void			ClearFrame()							= 0; //How will this work with multi-threading?
};

class Blueprint {
public:
	Mesh*					mesh;
	Technique*				technique;
	std::vector<Texture*>	textures;
};

struct Object {
	Blueprint* blueprint;
	Transform transform;

	static Object* CreateObjectFromBlueprint(Blueprint* bp);
};

class Buffer;


int main() {

	//Init Renderer with Window
	Renderer* renderer = Renderer::MakeRenderer();


#pragma region Globals
	//Globals
	std::vector<Blueprint*>		blueprints;
	std::vector<Mesh*>			meshes;
	std::vector<Material*>		materials;
	std::vector<Texture*>		textures;
	std::vector<Technique*>		techniques;
	std::vector<RenderState*>	renderStates;
	std::vector<Camera*>		cameras;
	std::vector<Object>			objects;


#pragma region CreateCamera
	//Create Camera
	Camera* cam = renderer->MakeCamera();
	cam->SetPosition(Float3());
	cam->SetTarget(Float3());
	cam->SetPerspectiveProjection(3.14159265f * 0.5f, 1.0f, 0.01f, 1000.0f);
	cameras.push_back(cam);
#pragma endregion

#pragma region CreateUniqueBlue
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
	mesh->SetTechnique(tech);

	//Create a Texture
	Texture* tex = renderer->MakeTexture();
	tex->LoadFromFile(".png");
	textures.push_back(tex);

	//Create the final blueprint
	Blueprint* blueprint = new Blueprint;
	blueprint->technique = tech;
	blueprint->mesh;
	blueprint->textures.push_back(tex);
	blueprints.push_back(blueprint);

#pragma endregion

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

	return 0;
}