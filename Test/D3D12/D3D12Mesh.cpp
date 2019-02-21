#include "D3D12Mesh.hpp"
#include "D3D12Renderer.hpp"
#include "D3D12VertexBuffer.hpp"

#include <sstream>
#include <fstream>

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
	if (!fileName)
		return false;

	// Open .obj file
	// TODO: generalize for other file formats
	std::string fullPath = std::string("../assets/Models/") + fileName;
	std::ifstream inFile(fullPath);
	if (!inFile.is_open())
		return false;

	std::string line, type;
	std::vector<Float3> positions;
	std::vector<Float3> normals;
	std::vector<Float2> uvs;
	std::vector<Float3> facePositions;
	std::vector<Float3> faceNormals;
	std::vector<Float2> faceUVs;

	/*
	Copy vertex data from file to temporary vector
	Load material name
	*/
	while (std::getline(inFile, line))
	{
		type = "";

		std::istringstream iss(line);

		iss >> type;

		if (type == "mtllib")
		{
			iss >> line;

			mDefaultMaterialName = line.c_str();
		}
		else if (type == "v")
		{
			Float3 v;
			iss >> v.x >> v.y >> v.z;
			positions.push_back(v);
		}
		else if (type == "vn")
		{
			Float3 vn;
			iss >> vn.x >> vn.y >> vn.z;
			normals.push_back(vn);
		}
		else if (type == "vt")
		{
			Float2 vt;
			iss >> vt.x >> vt.y;
			uvs.push_back(vt);
		}
		else if (type == "f")
		{
			int pIdx[4];
			int nIdx[4];
			int uvIdx[4];

			int nrOfVertsOnFace = 0;

			std::string str;
			while (iss >> str)
			{
				str.replace(str.find("/"), 1, " ");
				str.replace(str.find("/"), 1, " ");

				std::istringstream stringStream2(str);

				stringStream2 >> pIdx[nrOfVertsOnFace] >> uvIdx[nrOfVertsOnFace] >> nIdx[nrOfVertsOnFace];

				pIdx[nrOfVertsOnFace]--;
				nIdx[nrOfVertsOnFace]--;
				uvIdx[nrOfVertsOnFace]--;
				nrOfVertsOnFace++;
			}

			if (positions.size() > 0)
			{
				facePositions.push_back(positions[pIdx[0]]);
				facePositions.push_back(positions[pIdx[1]]);
				facePositions.push_back(positions[pIdx[2]]);

				if (nrOfVertsOnFace == 4)
				{
					facePositions.push_back(positions[pIdx[2]]);
					facePositions.push_back(positions[pIdx[3]]);
					facePositions.push_back(positions[pIdx[0]]);
				}
			}
			if (normals.size() > 0)
			{
				faceNormals.push_back(normals[nIdx[0]]);
				faceNormals.push_back(normals[nIdx[1]]);
				faceNormals.push_back(normals[nIdx[2]]);

				if (nrOfVertsOnFace == 4)
				{
					faceNormals.push_back(normals[nIdx[2]]);
					faceNormals.push_back(normals[nIdx[3]]);
					faceNormals.push_back(normals[nIdx[0]]);
				}
			}
			if (uvs.size() > 0)
			{
				faceUVs.push_back(uvs[uvIdx[0]]);
				faceUVs.push_back(uvs[uvIdx[1]]);
				faceUVs.push_back(uvs[uvIdx[2]]);

				if (nrOfVertsOnFace == 4)
				{
					faceUVs.push_back(uvs[uvIdx[2]]);
					faceUVs.push_back(uvs[uvIdx[3]]);
					faceUVs.push_back(uvs[uvIdx[0]]);
				}
			}
		}
	}

	if (facePositions.size() > 0)
	{
		if (!AddVertexBuffer(facePositions.size(), sizeof(Float3), facePositions.data(), Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_POSITION))
			return false;
	}
	if (faceNormals.size() > 0)
	{
		if (!AddVertexBuffer(faceNormals.size(), sizeof(Float3), faceNormals.data(), Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_NORMAL))
			return false;
	}
	if (faceUVs.size() > 0)
	{
		if (!AddVertexBuffer(faceUVs.size(), sizeof(Float2), faceUVs.data(), Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_UV))
			return false;
	}

	return true;
}

bool D3D12Mesh::InitializeCube(unsigned int vertexBufferFlags)
{
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

		//Create vertexbuffer for positions
		if (!AddVertexBuffer(sizeof(data) / sizeof(Float3), sizeof(Float3), data, Mesh::VERTEX_BUFFER_FLAG_POSITION))
			return false;
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

		//Create vertexbuffer for normals
		if (!AddVertexBuffer(sizeof(data) / sizeof(Float3), sizeof(Float3), data, Mesh::VERTEX_BUFFER_FLAG_NORMAL))
			return false;
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

		//Create vertexbuffer for UVs
		if (!AddVertexBuffer(sizeof(data) / sizeof(Float2), sizeof(Float2), data, Mesh::VERTEX_BUFFER_FLAG_UV))
			return false;
	}

	return true;
}

bool D3D12Mesh::InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections)
{
	return true;
}

bool D3D12Mesh::AddVertexBuffer(int nElements, int elementSize, void* data, Mesh::VertexBufferFlag bufferType)
{
	if (mVertexBufferFlags & bufferType)
		return false;
	
	mVertexBufferFlags |= bufferType;

	//Create Vertexbuffer
	D3D12VertexBuffer* vertexBuffer = renderer->MakeVertexBuffer();
	if (!vertexBuffer->Initialize(nElements, elementSize, data)) {
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
