#pragma once
#include "D3D12Engine/Material.hpp"
class D3D12API;

/*
	Describes the material and what shaders needed to render this material.
	Could contain data like how reflective the material is etc.
*/
class D3D12Material : public Material {
public:
	D3D12Material(D3D12API* d3d12);
	virtual ~D3D12Material();

	virtual bool LoadFromFile(const char* name, ResourceManager& resourceManager) override;
	virtual void SetShaderProgram(ShaderProgramHandle sp) override;

	bool IsOpaque();
private:
	D3D12API* m_d3d12;
	bool m_isOpaque = true;
};