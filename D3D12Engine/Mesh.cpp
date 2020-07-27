#include "stdafx.h"

#include "Mesh.hpp"

Mesh::~Mesh() {
	if (m_boundingVolume) {
		delete m_boundingVolume;
	}
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

BoundingVolume* Mesh::GetBoundingVolume() {
	return m_boundingVolume;
}

void Mesh::SetBoundingVolume(BoundingVolume* bv) {
	m_boundingVolume = bv;
}