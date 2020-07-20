#include "stdafx.h"

#include "D3D12Material.hpp"
#include "D3D12API.hpp"
#include "D3D12ShaderManager.hpp"

#include <sstream>
#include <fstream>


D3D12Material::D3D12Material(D3D12API* d3d12) {
	m_d3d12 = d3d12;
}

D3D12Material::~D3D12Material() {
}

bool D3D12Material::LoadFromFile(const char* name, ResourceManager& resourceManager) {
	bool result = Material::LoadFromFile(name, resourceManager);

	if (result) {
		m_isOpaque = m_d3d12->GetShaderManager_D3D12()->IsHitGroupOpaque(m_shaderProgram);
	}

	return result;
}

bool D3D12Material::IsOpaque() {
	return m_isOpaque;
}
