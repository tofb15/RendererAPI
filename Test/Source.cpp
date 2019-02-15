#include <vector>

class Texture {
public:
	virtual bool LoadFromFile(const char*)							= 0;
private:
	Texture();
};

class Material {
public:
	virtual bool LoadFromFile(const char*)							= 0;
	virtual void AddTexture(Texture*);
private:
	std::vector<Texture*> textures;
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
	virtual bool LoadFromFile(const char*)							= 0;
	virtual void SetTechnique(Technique*);
private:
	std::vector<VertexBuffer*> vertexBuffers;
	Technique* tech;
};

class Renderer {
public:
	static	Renderer*		MakeRenderer();
	virtual Texture*		MakeTexture()							= 0;
	virtual Mesh*			MakeMesh()								= 0;
	virtual Material*		MakeMaterial()							= 0;
	virtual RenderState*	MakeRenderState()						= 0;
	virtual Technique*		MakeTechnique(Material*, RenderState*)	= 0;


	virtual void			Submit(Mesh*)							= 0; //How will this work with multi-threaded submitions? Should we submit an "Entity"-class insteed of a "Mesh"-class?
	virtual void			ClearSubmitions()						= 0; //Should we have this?
	virtual void			Frame()									= 0; //How will this work with multi-threading? One thread to rule them all?
	virtual void			Present()								= 0; //How will this work with multi-threading? One thread to rule them all?
	virtual void			ClearFrame()							= 0; //How will this work with multi-threading?
};


class Buffer;
class VertexBuffer;
class Entity;




int main() {

	//Init Renderer with Window
	Renderer* renderer = Renderer::MakeRenderer();

	//Globals
	std::vector<Mesh*> meshes;
	std::vector<Material*> materials;
	std::vector<Texture*> textures;
	std::vector<Technique*> techniques;
	std::vector<RenderState*> renderStates;

#pragma region CreateUniqueMesh
	//Load meshes and materials from file
	Mesh* mesh = renderer->MakeMesh();
	mesh->LoadFromFile(".obj"); //Vertexbuffer loaded here but should be able to be added seperatly aswell. Should we load material and texture here aswell?
	meshes.push_back(mesh);

	//Create a material
	Material* mat = renderer->MakeMaterial();
	mat->LoadFromFile(".mtl");
	materials.push_back(mat);

	//Create a Texture
	Texture* tex = renderer->MakeTexture();
	tex->LoadFromFile(".png");
	textures.push_back(tex);

	//Add texture to material
	mat->AddTexture(tex);

	//Create RenderState
	RenderState* renderState = renderer->MakeRenderState();
	renderStates.push_back(renderState);

	//Create Technique from renderstate and material
	Technique* tech = renderer->MakeTechnique(mat, renderState);
	techniques.push_back(tech);

	//Attach Technique to mesh
	mesh->SetTechnique(tech);
#pragma endregion

#pragma region Render
	for (size_t i = 0; i < meshes.size(); i++)
	{
		//Submit all meshes that should be rendered.
		renderer->Submit(meshes[i]);
	}

	renderer->Frame();	//Draw all meshes in the submit list. Do we want to support multiple frames? What if we want to render split-screen? Could differend threads prepare different frames?
	renderer->Present();//Present frame to screen

#pragma endregion

	return 0;
}