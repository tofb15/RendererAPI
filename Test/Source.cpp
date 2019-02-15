#include <vector>

class Texture {
public:
	virtual bool LoadFromFile(const char*)						= 0;
private:
	Texture();
};

class Material {
public:
	virtual bool LoadFromFile(const char*)						= 0;
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
	virtual bool LoadFromFile(const char*) = 0;
	virtual void SetTechnique(Technique*);
private:
	std::vector<VertexBuffer*> vertexBuffers;
	Technique* tech;
};

class Renderer {
public:
	static Renderer* MakeRenderer();
	virtual Texture* MakeTexture()								= 0;
	virtual Mesh* MakeMesh()									= 0;
	virtual Material* MakeMaterial()							= 0;
	virtual RenderState* MakeRenderState()						= 0;
	virtual Technique* MakeTechnique(Material*, RenderState*)	= 0;
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

#pragma region CreateUniqeMesh
	//Load meshes and materials from file
	Mesh* mesh = renderer->MakeMesh();
	mesh->LoadFromFile(".obj");
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

	return 0;
}