#pragma once

class Texture;
class Mesh;

class Terrain
{
public:
	Terrain();
	~Terrain();

	/*
		Initialize mesh from a heightMap.
	*/
	virtual bool InitializeHeightMap(Texture* _texture, float maxHeight) = 0;
	Mesh* GetMesh();

protected:
	Mesh* m_mesh;
};