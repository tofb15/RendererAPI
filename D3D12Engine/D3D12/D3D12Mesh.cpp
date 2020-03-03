#include "stdafx.h"

#include "D3D12Mesh.hpp"
#include "D3D12API.hpp"
#include "D3D12VertexBuffer.hpp"
#include "D3D12Texture.hpp"
#include "Loaders/OBJ_Loader.h"

#include <sstream>
#include <fstream>

D3D12Mesh::D3D12Mesh(D3D12API* renderer, unsigned short id) : m_id(id)
{
	m_d3d12 = renderer;
}

D3D12Mesh::~D3D12Mesh()
{
	//for (auto& buffer : m_vertexBuffers)
	//{
	//	delete buffer.second;
	//}

	for (auto& obj : m_subObjects)
	{
		for (auto& buffer : obj.second)
		{
			delete buffer.second;
		}
	}
}

bool D3D12Mesh::LoadFromFile(const char * fileName, MeshLoadFlag loadFlag)
{
	// Open .obj file
	// TODO: generalize for other file formats
	//std::string fullPath = fileName;
	//std::ifstream inFile(fullPath);
	//if (!inFile.is_open())
	//	return false;

	//std::string line, type;
	//
	//std::vector<Float3> all_positions;
	//std::vector<Float3> all_normals;
	//std::vector<Float2> all_uvs;

	//std::string currentMaterialName;


	if (loadFlag & MESH_LOAD_FLAG_USE_INDEX) {
		//TODO: FIX INDEXBUFFERS - Indexbuffer not yet supported

		/*
		std::unordered_map<std::string, std::vector<UINT>>   material_faceIndex;
		std::vector<Float3> positions;
		std::vector<Float3> normals;
		std::vector<Float2> uvs;

		if (!LOADER::LoadOBJ(fileName, positions, uvs, normals, material_faceIndex, )) {
			return false;
		}

		D3D12VertexBuffer* posBuffer = m_d3d12->MakeVertexBuffer();
		if (!posBuffer->Initialize(positions.size(), sizeof(Float3), positions.data())) {
			delete posBuffer;
			return false;
		}

		D3D12VertexBuffer* uvBuffer = m_d3d12->MakeVertexBuffer();
		if (!uvBuffer->Initialize(uvs.size(), sizeof(Float2), uvs.data())) {
			delete uvBuffer;
			return false;
		}

		for (auto& e : material_faceIndex)
		{
			if (!AddVertexBuffer(posBuffer, Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_POSITION, e.first))
				return false;

			if (!AddVertexBuffer(normBuffer, Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_NORMAL, e.first))
				return false;

			if (!AddVertexBuffer(posBuffer, Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_POSITION, e.first))
				return false;

			if (!CalculateTangentAndBinormal(material_facePositions[e.first].data(), material_faceUVs[e.first].data(), material_facePositions[e.first].size(), e.first)) {
				return false;
			}
		}
		*/
	}
	else {
		std::unordered_map<std::string, std::vector<Float3>> material_facePositions;
		std::unordered_map<std::string, std::vector<Float3>> material_faceNormals;
		std::unordered_map<std::string, std::vector<Float2>> material_faceUVs;

		if (!LOADER::LoadOBJ(fileName, material_facePositions, material_faceNormals, material_faceUVs)) {
			return false;
		}
	
		for (auto& e : material_facePositions)
		{
			if (!AddVertexBuffer(static_cast<int>(e.second.size()), sizeof(Float3), e.second.data(), Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_POSITION, e.first))
				return false;

			if (material_faceNormals.count(e.first))
			{
				if (!AddVertexBuffer(static_cast<int>(material_faceNormals[e.first].size()), sizeof(Float3), material_faceNormals[e.first].data(), Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_NORMAL, e.first))
					return false;
			}

			if (material_faceUVs.count(e.first))
			{
				if (!AddVertexBuffer(static_cast<int>(material_faceUVs[e.first].size()), sizeof(Float2), material_faceUVs[e.first].data(), Mesh::VertexBufferFlag::VERTEX_BUFFER_FLAG_UV, e.first))
					return false;
			}

			if (!CalculateTangentAndBinormal(material_facePositions[e.first].data(), material_faceUVs[e.first].data(), material_facePositions[e.first].size(), e.first)) {
				return false;
			}
		}
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
		if (!AddVertexBuffer(sizeof(data) / sizeof(Float3), sizeof(Float3), data, Mesh::VERTEX_BUFFER_FLAG_POSITION, "cube"))
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
		if (!AddVertexBuffer(sizeof(data) / sizeof(Float3), sizeof(Float3), data, Mesh::VERTEX_BUFFER_FLAG_NORMAL, "cube"))
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
		if (!AddVertexBuffer(sizeof(data) / sizeof(Float2), sizeof(Float2), data, Mesh::VERTEX_BUFFER_FLAG_UV, "cube"))
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

		if (!CalculateTangentAndBinormal(posData, UVdata, 36, "cube")) {
			return false;
		}
	}

	return true;
}

bool D3D12Mesh::InitializeSphere(const uint16_t verticalSections, const uint16_t horizontalSections)
{
	return true;
}

int D3D12Mesh::GetNumberOfSubMeshes()
{
	return m_subObjects.size();
}

std::string D3D12Mesh::GetSubMesheName(int i)
{
	int j = 0;
	for (auto& e : m_subObjects)
	{
		if (i == j)
			return e.first;
		j++;
	}

	return std::string();
}

bool D3D12Mesh::AddVertexBuffer(int nElements, int elementSize, void* data, Mesh::VertexBufferFlag bufferType, std::string subObject)
{
	std::unordered_map<VertexBufferFlag, D3D12VertexBuffer*>& m_subObject = m_subObjects[subObject];
	if (m_subObject.count(bufferType))
		return false;
	
	//m_VertexBufferFlags |= bufferType;

	//Create Vertexbuffer
	D3D12VertexBuffer* vertexBuffer = m_d3d12->MakeVertexBuffer();
	if (!vertexBuffer->Initialize(nElements, elementSize, data)) {
		delete vertexBuffer;
		return false;
	}
	m_subObject.insert({ bufferType, vertexBuffer });

	return true;
}

bool D3D12Mesh::AddVertexBuffer(D3D12VertexBuffer* buffer, Mesh::VertexBufferFlag bufferType, std::string subObject)
{
	if (!buffer) {
		return false;
	}

	std::unordered_map<VertexBufferFlag, D3D12VertexBuffer*>& m_subObject = m_subObjects[subObject];
	if (m_subObject.count(bufferType))
		return false;

	//Create Vertexbuffer
	D3D12VertexBuffer* vertexBuffer = m_d3d12->MakeVertexBuffer(*buffer);
	m_subObject.insert({ bufferType, vertexBuffer });

	return true;
}

D3D12VertexBuffer* D3D12Mesh::GetVertexBuffer(Mesh::VertexBufferFlag bufferType)
{
	for (auto& e : m_subObjects)
	{
		return e.second[bufferType];
	}

	return nullptr;
}

std::unordered_map<std::string, std::unordered_map<Mesh::VertexBufferFlag, D3D12VertexBuffer*>>& D3D12Mesh::GetSubObjects()
{
	return m_subObjects;
}

bool D3D12Mesh::CalculateTangentAndBinormal(Float3* positions, Float2* uvs, int nElements, std::string subObject )
{
	//int condition = VertexBufferFlag::VERTEX_BUFFER_FLAG_POSITION | VertexBufferFlag::VERTEX_BUFFER_FLAG_UV;
	//if ((m_VertexBufferFlags & condition) != condition || m_VertexBufferFlags & VertexBufferFlag::VERTEX_BUFFER_FLAG_TANGENT_BINORMAL) {
	//	return false;
	//}

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

	if (!AddVertexBuffer(tangentsAndBinormal.size()/2 , sizeof(Float3)*2, (void*)&tangentsAndBinormal[0], Mesh::VERTEX_BUFFER_FLAG_TANGENT_BINORMAL, subObject))
		return false;

	return true;
}

std::unordered_map<Mesh::VertexBufferFlag, D3D12VertexBuffer*>* D3D12Mesh::GetVertexBuffers()
{
	return nullptr;
}

std::vector<D3D12VertexBuffer*> D3D12Mesh::GetVertexBuffers_vec()
{
	std::vector<D3D12VertexBuffer*> vec;

	for (auto& e : m_subObjects)
	{
		if (e.second.count(VERTEX_BUFFER_FLAG_POSITION)) {
			vec.push_back(e.second[VERTEX_BUFFER_FLAG_POSITION]);
		}

		if (e.second.count(VERTEX_BUFFER_FLAG_NORMAL)) {
			vec.push_back(e.second[VERTEX_BUFFER_FLAG_NORMAL]);
		}

		if (e.second.count(VERTEX_BUFFER_FLAG_UV)) {
			vec.push_back(e.second[VERTEX_BUFFER_FLAG_UV]);
		}

		if (e.second.count(VERTEX_BUFFER_FLAG_TANGENT_BINORMAL)) {
			vec.push_back(e.second[VERTEX_BUFFER_FLAG_TANGENT_BINORMAL]);
		}

		break;
	}


	return vec;
}

unsigned short D3D12Mesh::GetID() const
{
	return m_id;
}