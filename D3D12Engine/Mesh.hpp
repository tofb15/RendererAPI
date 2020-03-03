#pragma once
#include "Math.hpp"

class Texture;
class VertexBuffer;

/*
	Used to contain a model.
	Basicly just a collection of related vertexbuffers.
*/
class Mesh {
public:

	enum VertexBufferFlag {
		VERTEX_BUFFER_FLAG_INDEX            = 1,
		VERTEX_BUFFER_FLAG_POSITION         = 2,
		VERTEX_BUFFER_FLAG_NORMAL           = 4,
		VERTEX_BUFFER_FLAG_UV               = 8,
		VERTEX_BUFFER_FLAG_TANGENT_BINORMAL = 16
	};

	enum MeshLoadFlag {
		MESH_LOAD_FLAG_NONE = 0,
		MESH_LOAD_FLAG_USE_INDEX = 1,
	};

	virtual ~Mesh();

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
	virtual bool LoadFromFile(const char* fileName, MeshLoadFlag loadFlag) = 0;

	// WARNING: Multiple meshes can initilize the same shape.
	// It is the programmer's responsibility to only initialize 1 of each mesh shape and use it multiple times instead.

	/*
		Initialize mesh from with the model of a cube. This function will fill all model vertex data into buffers. To load the Material and textures,
		@see Texture
		and
		@see Material

		@param vertexBufferFlag, specifies which buffers to create for the mesh.
		@see Mesh::VertexBufferFlag
	*/
	virtual bool InitializeCube(unsigned int vertexBufferFlags) = 0;
	virtual bool InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections) = 0;

	unsigned GetVertexBufferFlags() const;
	//virtual void SetTechnique(Technique*);
	const char* GetMaterialName() const;
	std::string GetName() const;
	void SetName(std::string name);

	virtual int GetNumberOfSubMeshes() = 0;
	virtual std::string GetSubMesheName(int i) = 0;

protected:
	Mesh();
	const char* m_DefaultMaterialName;
	unsigned m_VertexBufferFlags;
private:
	std::string m_name;
	bool m_IsCreated;	// vertexbuffers.size() > 0 ish maybe ? idk
	//std::vector<VertexBuffer*> vertexBuffers;
	//Technique* tech; //Moved To blueprint
};