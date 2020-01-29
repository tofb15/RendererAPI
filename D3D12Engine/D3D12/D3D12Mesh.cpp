#include "stdafx.h"

#include "D3D12Mesh.hpp"
#include "D3D12API.hpp"
#include "D3D12VertexBuffer.hpp"
#include "D3D12Texture.hpp"

#include <sstream>
#include <fstream>

D3D12Mesh::D3D12Mesh(D3D12API* renderer, unsigned short id) : m_id(id)
{
	m_renderer = renderer;
}

D3D12Mesh::~D3D12Mesh()
{
	for (D3D12VertexBuffer* buffer : m_vertexBuffers)
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

	bool hasNor, hasUV;
	while (std::getline(inFile, line))
	{
		type = "";

		std::istringstream iss(line);
		iss >> type;

		if (type == "mtllib")
		{
			iss >> line;

			m_DefaultMaterialName = line.c_str();
		}
		else if (type == "v")
		{
			Float3 v;
			iss >> v.x >> v.y >> v.z;
			positions.push_back(v);
		}
		else if (type == "vn")
		{
			hasNor = true;
			Float3 vn;
			iss >> vn.x >> vn.y >> vn.z;
			normals.push_back(vn);
		}
		else if (type == "vt")
		{
			hasUV = true;

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
				if(hasNor)
					str.replace(str.find("/"), 1, " ");
				if(hasUV)
					str.replace(str.find("/"), 1, " ");

				std::istringstream stringStream2(str);

				stringStream2 >> pIdx[nrOfVertsOnFace];
				pIdx[nrOfVertsOnFace]--;

				if (hasUV) {
					stringStream2 >> uvIdx[nrOfVertsOnFace];
					uvIdx[nrOfVertsOnFace]--;
				}
				if (hasNor) {
					stringStream2 >> nIdx[nrOfVertsOnFace];
					nIdx[nrOfVertsOnFace]--;
				}
				
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
		if (!AddVertexBuffer(static_cast<int>(facePositions.size()), sizeof(Float3), facePositions.data(), Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_POSITION))
			return false;
	}
	if (faceNormals.size() > 0)
	{
		if (!AddVertexBuffer(static_cast<int>(faceNormals.size()), sizeof(Float3), faceNormals.data(), Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_NORMAL))
			return false;
	}
	if (faceUVs.size() > 0)
	{
		if (!AddVertexBuffer(static_cast<int>(faceUVs.size()), sizeof(Float2), faceUVs.data(), Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_UV))
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

	if (vertexBufferFlags & Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_TANGENT_BINORMAL) {
		//POS
		Float3 posData[] = {
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
		//UV
		Float2 UVdata[] = {
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

		CalculateTangentAndBinormal(posData, UVdata, 36);
	}

	return true;
}

bool D3D12Mesh::InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections)
{
	return true;
}

bool D3D12Mesh::AddVertexBuffer(int nElements, int elementSize, void* data, Mesh::VertexBufferFlag bufferType)
{
	if (m_VertexBufferFlags & bufferType)
		return false;
	
	m_VertexBufferFlags |= bufferType;

	//Create Vertexbuffer
	D3D12VertexBuffer* vertexBuffer = m_renderer->MakeVertexBuffer();
	if (!vertexBuffer->Initialize(nElements, elementSize, data)) {
		delete vertexBuffer;
		return false;
	}
	m_vertexBuffers.push_back(vertexBuffer);

	return true;
}

bool D3D12Mesh::CalculateTangentAndBinormal(Float3* positions, Float2* uvs, int nElements)
{
	int condition = VertexBufferFlag::VERTEX_BUFFER_FLAG_POSITION | VertexBufferFlag::VERTEX_BUFFER_FLAG_UV;
	if ((m_VertexBufferFlags & condition) != condition || m_VertexBufferFlags & VertexBufferFlag::VERTEX_BUFFER_FLAG_TANGENT_BINORMAL) {
		return false;
	}

	//Needed Variables
	//D3D12VertexBuffer* positions = m_vertexBuffers[0];
	//D3D12VertexBuffer* uvs = m_vertexBuffers[(m_VertexBufferFlags & VertexBufferFlag::VERTEX_BUFFER_FLAG_NORMAL) ? 2 : 1];
	std::vector<Float3> tangentsAndBinormal;

	for (size_t i = 0; i < nElements; i+=3)
	{
		Float3 vL[2];			// Vectors between vertices in Local space
		Float2 vT[2];			// Vectors between vertices in tangent space
		float denominator;
		Float3 tempTangent;
		Float3 tempBinormal;

		vL[0] = Float3(positions[1]) - positions[0];	// Vectors between vertices in Local space
		vL[1] = Float3(positions[2]) - positions[0];
		vT[0] = Float2(uvs[1]) - uvs[0];		// Vectors between vertices in UV space
		vT[1] = Float2(uvs[2]) - uvs[0];

		tempTangent = Float3(
			vT[1].y * vL[0].x - vT[0].y * vL[1].x,
			vT[1].y * vL[0].y - vT[0].y * vL[1].y,
			vT[1].y * vL[0].z - vT[0].y * vL[1].z);

		tempBinormal = Float3(
			vT[1].x * vL[0].x - vT[0].x * vL[1].x,
			vT[1].x * vL[0].y - vT[0].x * vL[1].y,
			vT[1].x * vL[0].z - vT[0].x * vL[1].z);

		denominator = vT[0].x * vT[1].y - vT[1].x * vT[0].y;
		tempTangent /= denominator;
		tempBinormal /= denominator;

		tempTangent.normalize();
		tempBinormal.normalize();

		for (size_t j = 0; j < 3; j++)
		{
			tangentsAndBinormal.push_back({ tempTangent.x, tempTangent.y, tempTangent.z });		//add Tangent
			tangentsAndBinormal.push_back({ tempBinormal.x, tempBinormal.y, tempBinormal.z });	//add Binormal
		}

	}

	if (!AddVertexBuffer(tangentsAndBinormal.size()/2 , sizeof(Float3)*2, (void*)&tangentsAndBinormal[0], Mesh::VERTEX_BUFFER_FLAG_TANGENT_BINORMAL))
		return false;

	return true;
}

std::vector<D3D12VertexBuffer*>* D3D12Mesh::GetVertexBuffers()
{
	return &m_vertexBuffers;
}

unsigned short D3D12Mesh::GetID() const
{
	return m_id;
}