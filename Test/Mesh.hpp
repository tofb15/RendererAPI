#pragma once
#include "Math.hpp"
#include <vector>

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
	virtual bool LoadFromFile(const char* fileName) = 0;

	// WARNING: Multiple meshes can initilize the same shape.
	// It is the programmer's responsibility to only initialize 1 of each mesh shape and use it multiple times instead.

	/*
		Initialize mesh from with the model of a cube. This function will fill all model vertex data into buffers. To load the Material and textures,
		@see Texture
		and
		@see Material
	*/
	virtual void InitializeCube() = 0;
	virtual void InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections) = 0;
	virtual void InitializePolygonList(const std::vector<Polygon>& polygons) = 0;

	//virtual void SetTechnique(Technique*);
	//virtual const char* GetMaterialName();
private:
	bool isCreated;	// vertexbuffers.size() > 0 ish maybe ? idk
	//std::vector<VertexBuffer*> vertexBuffers;
	//Technique* tech; //Moved To blueprint
};