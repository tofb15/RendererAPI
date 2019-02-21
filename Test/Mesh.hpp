#pragma once
#include "Math.hpp"
#include <vector>

/*
	Used to contain a model.
	Basicly just a collection of related vertexbuffers.
*/
class Mesh {
public:

	enum VertexBufferFlag {
		VERTEX_BUFFER_FLAG_POSITION = 1,
		VERTEX_BUFFER_FLAG_NORMAL = 2,
		VERTEX_BUFFER_FLAG_UV = 4,
		VERTEX_BUFFER_FLAG_TANGENT_BINORMAL = 8
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
	virtual bool LoadFromFile(const char* fileName) = 0;

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

	/*
		Add a vertex buffer to this mesh.
		If this function is called several times on the same mesh, the order of the calls is important, and goes as follows:
		Positions, Normals, UVs, (tangent + binormal)

		@param nElements, the number of elements in the buffer
		@param elementSize, the size of each element, in bytes
		@param data, the raw data of the elements
		@param bufferType, the type of the buffer,
		@see Mesh::VertexBufferFlag
	*/
	virtual bool AddVertexBuffer(int nElements, int elementSize, void* data, Mesh::VertexBufferFlag bufferType) = 0;

	unsigned GetVertexBufferFlags() const;
	//virtual void SetTechnique(Technique*);
	//virtual const char* GetMaterialName();
protected:
	Mesh();
	const char* mDefaultMaterialName;
	unsigned mVertexBufferFlags;
private:
	bool mIsCreated;	// vertexbuffers.size() > 0 ish maybe ? idk
	//std::vector<VertexBuffer*> vertexBuffers;
	//Technique* tech; //Moved To blueprint
};