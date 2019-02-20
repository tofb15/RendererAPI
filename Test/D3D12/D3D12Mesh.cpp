#include "D3D12Mesh.hpp"
#include "D3D12Renderer.hpp"
#include "D3D12VertexBuffer.hpp"

D3D12Mesh::D3D12Mesh(D3D12Renderer* renderer)
{
	this->renderer = renderer;
}

D3D12Mesh::~D3D12Mesh()
{
	for (D3D12VertexBuffer* buffer : vertexBuffers)
	{
		delete buffer;
	}
}

bool D3D12Mesh::LoadFromFile(const char * fileName)
{
	return false;
}

bool D3D12Mesh::InitializeCube(unsigned int vertexBufferFlags)
{
	mVertexBufferFlags = vertexBufferFlags;

	if (vertexBufferFlags & Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_POSITION) {

		Float3 data[] = {
			//Front
			{-0.5f, 0.5, -0.5f}, { 0.5f,  0.5, -0.5f }, { 0.5f,  -0.5, -0.5f },
			{ 0.5f,  -0.5, -0.5f }, { -0.5f,  -0.5, -0.5f }, { -0.5f,  0.5, -0.5f },

			//Right
			{ 0.5f,  0.5, -0.5f }, { 0.5f,  0.5, 0.5f }, { 0.5f,  -0.5, 0.5f },
			{ 0.5f,  -0.5, 0.5f }, { 0.5f,  -0.5, -0.5f }, { 0.5f,  0.5, -0.5f },
	
			//Back
			{ 0.5f,  0.5, 0.5f }, { -0.5f,  -0.5, 0.5f }, { 0.5f,  -0.5, 0.5f },
			{ -0.5f,  -0.5, 0.5f }, { 0.5f,  0.5, 0.5f }, { -0.5f,  0.5, 0.5f },
		
			//Left
			{ -0.5f,  0.5, 0.5f }, { -0.5f,  0.5, -0.5f }, { -0.5f,  -0.5, -0.5f },
			{ -0.5f,  -0.5, -0.5f }, { -0.5f,  -0.5, 0.5f }, { -0.5f,  0.5, 0.5f },
		
			//Top
			{ -0.5f,  0.5, 0.5f}, { 0.5f,  0.5, 0.5f }, { 0.5f,  0.5, -0.5f },
			{ 0.5f,  0.5, -0.5f}, { -0.5f,  0.5, -0.5f }, { -0.5f,  0.5, 0.5f },
		
			//Bottom
			{ 0.5f,  -0.5, -0.5f }, { 0.5f,  -0.5, 0.5f }, { -0.5f,  -0.5, 0.5f },
			{ -0.5f,  -0.5, 0.5f }, { -0.5f,  -0.5, -0.5f }, { 0.5f,  -0.5, -0.5f },
		};

		//Create Vertexbuffer
		D3D12VertexBuffer* vertexBuffer = renderer->MakeVertexBuffer();
		if (!vertexBuffer->Initialize(36, sizeof(Float3), static_cast<void*>(data))) {
			delete vertexBuffer;
			return false;
		}

		vertexBuffers.push_back(vertexBuffer);
	}
	
	if (vertexBufferFlags & Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_NORMAL) {

		Float3 data[] = {
			//Front
			{ 0.0f,  0.0f, -1.0f},{ 0.0f, 0.0f, -1.0f},{ 0.0f, 0.0f, -1.0f},
			{ 0.0f,  0.0f, -1.0f},{ 0.0f, 0.0f, -1.0f},{ 0.0f, 0.0f, -1.0f},

			//Right
			{ 1.0f,  0.0f, 0.0f},{ 1.0f,  0.0f, 0.0f},{ 1.0f,  0.0f, 0.0f},
			{ 1.0f,  0.0f, 0.0f},{ 1.0f,  0.0f, 0.0f},{ 1.0f,  0.0f, 0.0f},

			//Back
			{ 0.0f,  0.0f, 1.0f},{ 0.0f, 0.0f, 1.0f},{ 0.0f, 0.0f, 1.0f},
			{ 0.0f,  0.0f, 1.0f},{ 0.0f, 0.0f, 1.0f},{ 0.0f, 0.0f, 1.0f},

			//Left
			{ -1.0f,  0.0f, 0.0f},{ -1.0f,  0.0f, 0.0f},{ -1.0f,  0.0f, 0.0f},
			{ -1.0f,  0.0f, 0.0f},{ -1.0f,  0.0f, 0.0f},{ -1.0f,  0.0f, 0.0f},

			//Top
			{ 0.0f,  1.0f, 0.0f},{ 0.0f, 1.0f, 0.0f},{ 0.0f, 1.0f, 0.0f},
			{ 0.0f,  1.0f, 0.0f},{ 0.0f, 1.0f, 0.0f},{ 0.0f, 1.0f, 0.0f},

			//Bottom
			{ 0.0f,  -1.0f, 0.0f},{ 0.0f, -1.0f, 0.0f},{ 0.0f, -1.0f, 0.0f},
			{ 0.0f,  -1.0f, 0.0f},{ 0.0f, -1.0f, 0.0f},{ 0.0f, -1.0f, 0.0f},
		};

		//Create Vertexbuffer
		D3D12VertexBuffer* vertexBuffer = renderer->MakeVertexBuffer();
		if (!vertexBuffer->Initialize(36, sizeof(Float3), static_cast<void*>(data))) {
			delete vertexBuffer;
			return false;
		}

		vertexBuffers.push_back(vertexBuffer);
	}

	if (vertexBufferFlags & Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_UV) {

		Float2 data[] = {
			//Front
			{ 0.0f,  0.0f }, { 1.0f,  0.0f }, { 1.0f,  1.0f },
			{ 1.0f,  1.0f }, { 0.0f,  1.0f }, { 0.0f,  0.0f },

			//Right
			{ 0.0f,  0.0f }, { 1.0f,  0.0f }, { 1.0f,  1.0f },
			{ 1.0f,  1.0f }, { 0.0f,  1.0f }, { 0.0f,  0.0f },

			//Back
			{ 0.0f,  0.0f }, { 1.0f,  1.0f }, { 0.0f,  1.0f },
			{ 1.0f,  1.0f }, { 0.0f,  0.0f }, { 1.0f,  0.0f },

			//Left
			{ 0.0f,  0.0f }, { 1.0f,  0.0f }, { 1.0f,  1.0f },
			{ 1.0f,  1.0f }, { 0.0f,  1.0f }, { 0.0f,  0.0f },

			//Top
			{ 0.0f,  0.0f }, { 1.0f,  0.0f }, { 1.0f,  1.0f },
			{ 1.0f,  1.0f }, { 0.0f,  1.0f }, { 0.0f,  0.0f },

			//Bottom
			{ 1.0f,  0.0f }, { 1.0f,  1.0f }, { 0.0f,  1.0f },
			{ 0.0f,  1.0f }, { 0.0f,  0.0f }, { 1.0f,  0.0f },
		};

		//Create Vertexbuffer
		D3D12VertexBuffer* vertexBuffer = renderer->MakeVertexBuffer();
		if (!vertexBuffer->Initialize(36, sizeof(Float2), static_cast<void*>(data))) {
			delete vertexBuffer;
			return false;
		}

		vertexBuffers.push_back(vertexBuffer);
	}

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
