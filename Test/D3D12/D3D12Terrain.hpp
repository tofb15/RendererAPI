#pragma once
#include "D3D12Mesh.hpp"
#include "..\Terrain.hpp"

class D3D12Terrain : public Terrain
{
public:
	D3D12Terrain(D3D12Renderer* renderer);
	~D3D12Terrain();

	// Inherited via Terrain
	virtual bool InitializeHeightMap(Texture * _texture, float maxHeight) override;

private:


};