#include <vector>

class Texture {
public:
	bool LoadFromFile(const char*);
private:
	Texture();
};

class Material {
public:
	bool LoadFromFile(const char*);
	void AddTexture(Texture*);
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
	bool LoadFromFile(const char*);
	void SetTechnique(Technique*);
private:
	std::vector<VertexBuffer*> vertexBuffers;
	Technique* tech;
};

class Renderer {
public:
	static Renderer* MakeRenderer();
	Texture* MakeTexture();
	Mesh* MakeMesh();
	Material* MakeMaterial();
	RenderState* MakeRenderState();
	Technique* MakeTechnique(Material*, RenderState*);
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

	//Load meshes and materials from file
	Mesh* mesh = renderer->MakeMesh();
	mesh->LoadFromFile(".obj");
	meshes.push_back(mesh);

	Material* mat = renderer->MakeMaterial();
	mat->LoadFromFile(".mtl");
	materials.push_back(mat);

	Texture* tex = renderer->MakeTexture();
	tex->LoadFromFile(".png");
	textures.push_back(tex);

	RenderState* renderState = renderer->MakeRenderState();
	renderStates.push_back(renderState);

	Technique* tech = renderer->MakeTechnique(mat, renderState);
	techniques.push_back(tech);

	//Attach materials to mesh

	return 0;
}