#pragma once
#include "D3D12Mesh.hpp"
#include "D3D12Engine/Terrain.hpp"

class D3D12Terrain : public Terrain {
public:
	D3D12Terrain(D3D12API* renderer);
	~D3D12Terrain();

	// Inherited via Terrain
	virtual bool InitializeHeightMap(Texture* _texture, float maxHeight) override;

private:


};