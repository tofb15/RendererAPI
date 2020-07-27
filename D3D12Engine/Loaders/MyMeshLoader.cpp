#include "stdafx.h"
#include "MyMeshLoader.h"
#include "BinaryFile.h"
#include <sstream>

constexpr int g_currentVersion = 1;

bool LoadMeshVersion1(BinaryFileIn& file, LOADER::MyMeshLoader::FLOAT3_BUFFER& facePos, LOADER::MyMeshLoader::FLOAT3_BUFFER& faceNorms, LOADER::MyMeshLoader::FLOAT2_BUFFER& faceUVs) {

	std::string line;
	while (file.ReadLine(line)) {
		std::string subMeshName = line;
		unsigned int nVertices;
		file.Read(nVertices);

		std::vector<Float3>& pos = facePos[subMeshName];
		std::vector<Float3>& norms = faceNorms[subMeshName];
		std::vector<Float2>& uvs = faceUVs[subMeshName];

		Float3 f3;
		Float2 f2;
		for (unsigned int i = 0; i < nVertices; i++) {
			//Save Position
			if (!file.Read(f3.x)) {
				//TODO: Log error
				return false;
			}
			if (!file.Read(f3.y)) {
				//TODO: Log error
				return false;
			}
			if (!file.Read(f3.z)) {
				//TODO: Log error
				return false;
			}
			pos.push_back(f3);
			//Save Normal
			if (!file.Read(f3.x)) {
				//TODO: Log error
				return false;
			}
			if (!file.Read(f3.y)) {
				//TODO: Log error
				return false;
			}
			if (!file.Read(f3.z)) {
				//TODO: Log error
				return false;
			}
			norms.push_back(f3);
			//Save Uv
			if (!file.Read(f2.x)) {
				//TODO: Log error
				return false;
			}
			if (!file.Read(f2.y)) {
				//TODO: Log error
				return false;
			}
			uvs.push_back(f2);
		}
	}
	return true;
}

bool LOADER::MyMeshLoader::SaveMesh(const char* fileName, FLOAT3_BUFFER& facePos, FLOAT3_BUFFER& faceNorms, FLOAT2_BUFFER& faceUVs) {
	if (facePos.size() != faceNorms.size() || facePos.size() != faceUVs.size()) {
		//TODO: Log error
		return false;
	}
	//Open File
	BinaryFileOut file;
	if (!file.Open(fileName)) {
		//TODO: Log error
		return false;
	}
	//Save Header
	file.WriteLine("Mesh");
	file.WriteLine("Version " + std::to_string(g_currentVersion));

	for (auto& e : facePos) {
		std::vector<Float3>::iterator pos = facePos[e.first].begin();
		std::vector<Float3>::iterator norms = faceNorms[e.first].begin();
		std::vector<Float2>::iterator uvs = faceUVs[e.first].begin();

		std::vector<Float3>::iterator pos_end = facePos[e.first].end();
		std::vector<Float3>::iterator norms_end = faceNorms[e.first].end();
		std::vector<Float2>::iterator uvs_end = faceUVs[e.first].end();

		file.WriteLine(e.first);
		unsigned int size = (unsigned int)e.second.size();
		file.Write(size);
		while (pos != pos_end && norms != norms_end && uvs != uvs_end) {
			//Save Position
			file.Write((*pos).x);
			file.Write((*pos).y);
			file.Write((*pos).z);
			//Save Normal
			file.Write((*norms).x);
			file.Write((*norms).y);
			file.Write((*norms).z);
			//Save Uv
			file.Write((*uvs).x);
			file.Write((*uvs).y);

			//Fetch next vertex
			pos++;
			norms++;
			uvs++;
		}

		if (pos != pos_end && norms != norms_end && uvs != uvs_end) {
			//TODO:: Log Error, data was corrupted
			return false;
		}
	}

	return true;
}
bool LOADER::MyMeshLoader::LoadMesh(const char* fileName, FLOAT3_BUFFER& facePos, FLOAT3_BUFFER& faceNorms, FLOAT2_BUFFER& faceUVs) {
	facePos.clear();
	faceNorms.clear();
	faceUVs.clear();

	//Open File
	BinaryFileIn file;
	if (!file.Open(fileName)) {
		//TODO: Log error
		return false;
	}

	std::string line;
	file.ReadLine(line);
	if (line != "Mesh") {
		file.Close();
		return false;
	}

	file.ReadLine(line);
	std::stringstream ss(line);

	std::string temp;
	int v;
	ss >> temp >> v;
	if (temp != "Version") {
		file.Close();
		return false;
	}

	switch (v) {
	case 1:
		return LoadMeshVersion1(file, facePos, faceNorms, faceUVs);
		break;
	default:
		//TODO:: Log Error
		return false;
		break;
	}

	return false;
}
