#include "stdafx.h"
#include "OBJ_Loader.h"
#include <fstream>
#include <iomanip>      // std::setprecision

bool LOADER::LoadOBJ(const char* fileName, FLOAT3_BUFFER& material_facePositions, FLOAT3_BUFFER& material_faceNormals, FLOAT2_BUFFER& material_faceUVs)
{

	if (!fileName)
		return false;

	// Open .obj file
	// TODO: generalize for other file formats
	std::string fullPath = fileName;
	std::ifstream inFile(fullPath);
	if (!inFile.is_open())
		return false;

	std::string line, type;

	std::vector<Float3> all_positions;
	std::vector<Float3> all_normals;
	std::vector<Float2> all_uvs;

	std::string currentMaterialName;

	Float3 extream_pos_max = Float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	Float3 extream_pos_min = Float3(FLT_MAX, FLT_MAX, FLT_MAX);
	Float3 mesh_origo;
	Float3 boundingBox_size;
	std::iostream::binary;
	//std::unordered_map<std::string, std::vector<Float3>> material_facePositions;
	//std::unordered_map<std::string, std::vector<Float3>> material_faceNormals;
	//std::unordered_map<std::string, std::vector<Float2>> material_faceUVs;

	//Name of the current subobject
	std::string currentSubobject;

	bool hasNor, hasUV;
	while (std::getline(inFile, line))
	{
		type = "";

		std::istringstream iss(line);
		iss >> type;

		if (type == "mtllib")
		{
			iss >> line;
			//m_DefaultMaterialName = line.c_str();
		}
		else if (type == "usemtl") {
			iss >> currentMaterialName;
		}
		else if (type == "v")
		{
			Float3 v;
			iss >> v.x >> v.y >> v.z;
			all_positions.push_back(v);

			if (v.x > extream_pos_max.x) {
				extream_pos_max.x = v.x;
			}
			if (v.y > extream_pos_max.y) {
				extream_pos_max.y = v.y;
			}
			if (v.z > extream_pos_max.z) {
				extream_pos_max.z = v.z;
			}
			if (v.x < extream_pos_min.x) {
				extream_pos_min.x = v.x;
			}
			if (v.y < extream_pos_min.y) {
				extream_pos_min.y = v.y;
			}
			if (v.z < extream_pos_min.z) {
				extream_pos_min.z = v.z;
			}
		}
		else if (type == "vn")
		{
			hasNor = true;
			Float3 vn;
			iss >> vn.x >> vn.y >> vn.z;
			all_normals.push_back(vn);
		}
		else if (type == "vt")
		{
			hasUV = true;

			Float2 vt;
			iss >> vt.x >> vt.y;
			all_uvs.push_back(vt);
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
				if (hasNor)
					str.replace(str.find("/"), 1, " ");
				if (hasUV)
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

			if (nrOfVertsOnFace > 4) {
				return false;
				//MessageBoxA(NULL, "To many verticies on one face", "Error", 0);
			}

			if (all_positions.size() > 0)
			{
				material_facePositions[currentMaterialName].push_back(all_positions[pIdx[0]]);
				material_facePositions[currentMaterialName].push_back(all_positions[pIdx[1]]);
				material_facePositions[currentMaterialName].push_back(all_positions[pIdx[2]]);

				if (nrOfVertsOnFace == 4)
				{
					material_facePositions[currentMaterialName].push_back(all_positions[pIdx[2]]);
					material_facePositions[currentMaterialName].push_back(all_positions[pIdx[3]]);
					material_facePositions[currentMaterialName].push_back(all_positions[pIdx[0]]);
				}
			}
			if (all_normals.size() > 0)
			{
				material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[0]]);
				material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[1]]);
				material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[2]]);

				if (nrOfVertsOnFace == 4)
				{
					material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[2]]);
					material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[3]]);
					material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[0]]);
				}
			}
			if (all_uvs.size() > 0)
			{
				material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[0]]);
				material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[1]]);
				material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[2]]);

				if (nrOfVertsOnFace == 4)
				{
					material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[2]]);
					material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[3]]);
					material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[0]]);
				}
			}
		}
	}

	mesh_origo = (extream_pos_max + extream_pos_min)/2;
	boundingBox_size = (extream_pos_max - mesh_origo) * 2;

	return true;
}

/*
	LOAD using indexBuffer. This is not done so dont use this.
*/
bool LOADER::LoadOBJ(const char* fileName, std::vector<Float3>& allPos, std::vector<Float2>& allUV, INT_BUFFER& faceIndex, FLOAT3_BUFFER& faceNorms)
{
	if (!fileName)
		return false;

	allPos.clear();
	allUV.clear();

	// Open .obj file
	// TODO: generalize for other file formats
	std::string fullPath = fileName;
	std::ifstream inFile(fullPath);
	if (!inFile.is_open())
		return false;

	std::string line, type;

	std::vector<Float3> all_normals;

	std::string currentMaterialName;

	//Name of the current subobject
	std::string currentSubobject;

	bool hasNor, hasUV;
	while (std::getline(inFile, line))
	{
		type = "";

		std::istringstream iss(line);
		iss >> type;

		if (type == "mtllib")
		{
			iss >> line;
			//m_DefaultMaterialName = line.c_str();
		}
		else if (type == "usemtl") {
			iss >> currentMaterialName;
		}
		else if (type == "v")
		{
			Float3 v;
			iss >> v.x >> v.y >> v.z;
			allPos.push_back(v);
		}
		else if (type == "vn")
		{
			hasNor = true;
			Float3 vn;
			iss >> vn.x >> vn.y >> vn.z;
			all_normals.push_back(vn);
		}
		else if (type == "vt")
		{
			hasUV = true;

			Float2 vt;
			iss >> vt.x >> vt.y;
			allUV.push_back(vt);
		}
		else if (type == "f")
		{
			int pIdx[4] = {0};
			int nIdx[4] = {0};
			int uvIdx[4] = {0};

			int nrOfVertsOnFace = 0;

			std::string str;
			while (iss >> str)
			{
				if (hasNor)
					str.replace(str.find("/"), 1, " ");
				if (hasUV)
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

			if (nrOfVertsOnFace > 4) {
				return false;
				//MessageBoxA(NULL, "To many verticies on one face", "Error", 0);
			}

			faceIndex[currentMaterialName].push_back(pIdx[0]);
			faceIndex[currentMaterialName].push_back(pIdx[1]);
			faceIndex[currentMaterialName].push_back(pIdx[2]);

			if (nrOfVertsOnFace == 4)
			{
				faceIndex[currentMaterialName].push_back(pIdx[2]);
				faceIndex[currentMaterialName].push_back(pIdx[3]);
				faceIndex[currentMaterialName].push_back(pIdx[0]);
			}		

			if (all_normals.size() > 0)
			{
				faceNorms[currentMaterialName].push_back(all_normals[nIdx[0]]);
				faceNorms[currentMaterialName].push_back(all_normals[nIdx[1]]);
				faceNorms[currentMaterialName].push_back(all_normals[nIdx[2]]);

				if (nrOfVertsOnFace == 4)
				{
					faceNorms[currentMaterialName].push_back(all_normals[nIdx[2]]);
					faceNorms[currentMaterialName].push_back(all_normals[nIdx[3]]);
					faceNorms[currentMaterialName].push_back(all_normals[nIdx[0]]);
				}
			}
		}
	}

	return true;
}

bool LOADER::SaveOBJ(const char* fileName, FLOAT3_BUFFER& material_facePositions, FLOAT3_BUFFER& material_faceNormals, FLOAT2_BUFFER& material_faceUVs)
{
	//Not optimized! Just for testing.

	if (!fileName)
		return false;

	// Open .obj file
	// TODO: generalize for other file formats
	std::string fullPath = fileName;
	std::ofstream file(fullPath);
	if (!file.is_open())
		return false;

	size_t j = 1;

	file << "mtllib cover.mtl\n";
	//file << std::setprecision(5);
	file << "o dummy" << "\n";
	for (auto& e: material_facePositions)
	{
		//size_t j = i;
		for (auto& v : e.second)
		{
			file << "v " << v.x << " " << v.y << " " << v.z << "\n";
		}
		for (auto& v : material_faceUVs[e.first])
		{
			file << "vt " << v.x << " " << v.y << "\n";
		}
		for (auto& v : material_faceNormals[e.first])
		{
			file << "vn " << v.x << " " << v.y << " " << v.z << "\n";
		}

		file << "usemtl " << e.first << "\n";
		file << "s " << 1 << "\n";
		auto e2 = e.second.begin();

		for (auto it = e.second.begin(); it != e.second.end();)
		{
			file << "f ";

			file << j << "/" << j << "/" << j << " ";
			j++;
			file << j << "/" << j << "/" << j << " ";
			j++;
			file << j << "/" << j << "/" << j << "\n";
			j++;

			it += 3;
		}

		//i += j;
	}

	file.close();

	//while (std::getline(inFile, line))
	//{
	//	type = "";

	//	std::istringstream iss(line);
	//	iss >> type;

	//	if (type == "mtllib")
	//	{
	//		iss >> line;
	//		//m_DefaultMaterialName = line.c_str();
	//	}
	//	else if (type == "usemtl") {
	//		iss >> currentMaterialName;
	//	}
	//	else if (type == "v")
	//	{
	//		Float3 v;
	//		iss >> v.x >> v.y >> v.z;
	//		all_positions.push_back(v);
	//	}
	//	else if (type == "vn")
	//	{
	//		hasNor = true;
	//		Float3 vn;
	//		iss >> vn.x >> vn.y >> vn.z;
	//		all_normals.push_back(vn);
	//	}
	//	else if (type == "vt")
	//	{
	//		hasUV = true;

	//		Float2 vt;
	//		iss >> vt.x >> vt.y;
	//		all_uvs.push_back(vt);
	//	}
	//	else if (type == "f")
	//	{
	//		int pIdx[4];
	//		int nIdx[4];
	//		int uvIdx[4];

	//		int nrOfVertsOnFace = 0;

	//		std::string str;
	//		while (iss >> str)
	//		{
	//			if (hasNor)
	//				str.replace(str.find("/"), 1, " ");
	//			if (hasUV)
	//				str.replace(str.find("/"), 1, " ");

	//			std::istringstream stringStream2(str);

	//			stringStream2 >> pIdx[nrOfVertsOnFace];
	//			pIdx[nrOfVertsOnFace]--;

	//			if (hasUV) {
	//				stringStream2 >> uvIdx[nrOfVertsOnFace];
	//				uvIdx[nrOfVertsOnFace]--;
	//			}
	//			if (hasNor) {
	//				stringStream2 >> nIdx[nrOfVertsOnFace];
	//				nIdx[nrOfVertsOnFace]--;
	//			}

	//			nrOfVertsOnFace++;
	//		}

	//		if (nrOfVertsOnFace > 4) {
	//			return false;
	//			//MessageBoxA(NULL, "To many verticies on one face", "Error", 0);
	//		}

	//		if (all_positions.size() > 0)
	//		{
	//			material_facePositions[currentMaterialName].push_back(all_positions[pIdx[0]]);
	//			material_facePositions[currentMaterialName].push_back(all_positions[pIdx[1]]);
	//			material_facePositions[currentMaterialName].push_back(all_positions[pIdx[2]]);

	//			if (nrOfVertsOnFace == 4)
	//			{
	//				material_facePositions[currentMaterialName].push_back(all_positions[pIdx[2]]);
	//				material_facePositions[currentMaterialName].push_back(all_positions[pIdx[3]]);
	//				material_facePositions[currentMaterialName].push_back(all_positions[pIdx[0]]);
	//			}
	//		}
	//		if (all_normals.size() > 0)
	//		{
	//			material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[0]]);
	//			material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[1]]);
	//			material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[2]]);

	//			if (nrOfVertsOnFace == 4)
	//			{
	//				material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[2]]);
	//				material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[3]]);
	//				material_faceNormals[currentMaterialName].push_back(all_normals[nIdx[0]]);
	//			}
	//		}
	//		if (all_uvs.size() > 0)
	//		{
	//			material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[0]]);
	//			material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[1]]);
	//			material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[2]]);

	//			if (nrOfVertsOnFace == 4)
	//			{
	//				material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[2]]);
	//				material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[3]]);
	//				material_faceUVs[currentMaterialName].push_back(all_uvs[uvIdx[0]]);
	//			}
	//		}
	//	}
	//}

	return true;
}