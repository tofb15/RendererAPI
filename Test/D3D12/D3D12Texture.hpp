#pragma once

#include "../Texture.hpp"

/*
	Contain a texture that could be applied to a mesh.
*/
class D3D12Texture : public Texture {
public:
	D3D12Texture();
	virtual ~D3D12Texture();

	// Inherited via Texture
	virtual bool LoadFromFile(const char *) override;
};