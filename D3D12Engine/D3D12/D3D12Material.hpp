#pragma once
#include "../Material.hpp"
/*
	Describes the material and what shaders needed to render this material.
	Could contain data like how reflective the material is etc.
*/
class D3D12Material : public Material {
public:
	D3D12Material();
	virtual ~D3D12Material();

	//virtual bool LoadFromFile(const char* fileName) override;
};