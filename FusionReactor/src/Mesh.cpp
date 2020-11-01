#include "stdafx.h"

#include "Mesh.hpp"

namespace FusionReactor {

	Mesh::~Mesh() {
	}

	unsigned Mesh::GetVertexBufferFlags() const {
		return m_VertexBufferFlags;
	}

	const char* Mesh::GetMaterialName() const {
		return m_DefaultMaterialName;
	}

	std::string Mesh::GetName() const {

		return m_name;
	}

	void Mesh::SetName(const std::string& name) {
		m_name = name;
	}

	Mesh::Mesh() {
		m_VertexBufferFlags = 0;
	}
}