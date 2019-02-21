#include "Mesh.hpp"

Mesh::~Mesh()
{
}

unsigned Mesh::GetVertexBufferFlags() const
{
	return mVertexBufferFlags;
}

const char * Mesh::GetMaterialName() const
{
	return mDefaultMaterialName;
}

Mesh::Mesh()
{
}
