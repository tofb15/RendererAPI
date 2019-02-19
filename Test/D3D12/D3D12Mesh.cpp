#include "D3D12Mesh.hpp"
#include "D3D12Renderer.hpp"
#include "D3D12VertexBuffer.hpp"

D3D12Mesh::D3D12Mesh(D3D12Renderer* renderer)
{
	this->renderer = renderer;
}

D3D12Mesh::~D3D12Mesh()
{
}

bool D3D12Mesh::LoadFromFile(const char * fileName)
{
	return false;
}

bool D3D12Mesh::InitializeCube()
{
	return true;
}

bool D3D12Mesh::InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections)
{
	return true;
}

bool D3D12Mesh::InitializePolygonList(std::vector<Polygon>& polygons)
{
	//Create Vertexbuffer
	D3D12VertexBuffer* vertexBuffer = renderer->MakeVertexBuffer();
	if (!vertexBuffer->Initialize(polygons.size() * 3, sizeof(Float3), static_cast<void*>(polygons.data()))) {
		delete vertexBuffer;
		return false;
	}

	vertexBuffers.push_back(vertexBuffer);
	return true;
}

std::vector<D3D12VertexBuffer*>* D3D12Mesh::GetVertexBuffers()
{
	return &vertexBuffers;
}
