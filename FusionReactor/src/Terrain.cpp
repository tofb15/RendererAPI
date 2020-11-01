#include "stdafx.h"

#include "Terrain.hpp"
#include "Mesh.hpp"
namespace FusionReactor {

	Terrain::Terrain() {
	}

	Terrain::~Terrain() {
	}

	Mesh* Terrain::GetMesh() {
		return m_mesh;
	}
}