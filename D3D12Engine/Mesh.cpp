#include "stdafx.h"

#include "Mesh.hpp"

Mesh::~Mesh()
{
}

unsigned Mesh::GetVertexBufferFlags() const
{
	return m_VertexBufferFlags;
}

const char * Mesh::GetMaterialName() const
{
	return m_DefaultMaterialName;
}

Mesh::Mesh()
{
	m_VertexBufferFlags = 0;
}
