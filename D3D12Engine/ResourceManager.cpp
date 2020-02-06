#include "stdafx.h"
#include "ResourceManager.h"
#include "RenderAPI.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"

ResourceManager* s_instance = nullptr;

ResourceManager* ResourceManager::GetInstance(RenderAPI* api)
{
	if (!s_instance) {
		s_instance = new ResourceManager(api);
	}

	return s_instance;
}

bool ResourceManager::SaveBlueprintToFile(std::vector<BlueprintDescription>& bpDescriptions)
{
	for (auto& e : bpDescriptions)
	{
		std::string fName = BLUEPRINT_PATH + e.blueprintName + ".bp";
		std::ofstream out(fName);

		//out << e.blueprintName << "\n";
		out << e.meshPath << "\n";
		out << ((e.allGeometryIsOpaque) ? "opaque" : "transparent") << "\n";
		out << e.texturePaths.size() << "\n";
		for (auto& t : e.texturePaths)
		{
			out << t << "\n";
		}

		out.close();
	}

	return true;
}

ResourceManager::~ResourceManager()
{
	for (auto& e : m_blueprints){if (e.second) {delete e.second;}}
	for (auto& e : m_meshes)    {if (e.second) {delete e.second;}}
	for (auto& e : m_textures)  {if (e.second) {delete e.second;}}
}

Blueprint* ResourceManager::LoadBlueprintFromFile(std::string name)
{
	Blueprint* bp = MY_NEW Blueprint;
	std::string fName = BLUEPRINT_PATH + name + ".bp";

	std::ifstream in(fName);
	if (!in.is_open()) {
		delete bp;
		return nullptr;
	}
	std::string line;

	std::getline(in, line);
	bp->mesh = GetMesh(line);
	std::getline(in, line);
	if (line == "opaque") {
		bp->allGeometryIsOpaque = true;
	}
	else if (line == "transparent") {
		bp->allGeometryIsOpaque = false;
	}

	int nTextures;
	in >> nTextures;
	in.ignore();
	
	for (size_t i = 0; i < nTextures; i++)
	{
		std::getline(in, line);
		bp->textures.push_back(GetTexture(line));
	}

	return bp;
}

Mesh* ResourceManager::GetMesh(std::string name)
{
	Mesh* mesh = nullptr;
	std::string fName = MESH_PATH + name;

	auto search = m_meshes.find(fName);
	if (search == m_meshes.end()) {
		mesh = m_api->MakeMesh();
		mesh->LoadFromFile(fName.c_str());
		m_meshes[fName] = mesh;
	}
	else {
		mesh = m_meshes[fName];
	}

	return mesh;
}

Texture* ResourceManager::GetTexture(std::string name)
{
	Texture* texture = nullptr;
	std::string fName = TEXTURE_PATH + name;

	auto search = m_meshes.find(fName);
	if (search == m_meshes.end()) {
		texture = m_api->MakeTexture();
		texture->LoadFromFile(fName.c_str(), Texture::TEXTURE_USAGE_CPU_FLAG | Texture::TEXTURE_USAGE_GPU_FLAG);
		m_textures[fName] = texture;
	}
	else {
		texture = m_textures[fName];
	}

	return texture;
}

Blueprint* ResourceManager::GetBlueprint(std::string name)
{
	Blueprint* bp = nullptr;
	auto search = m_blueprints.find(name);
	if (search == m_blueprints.end()) {
		bp = LoadBlueprintFromFile(name.c_str());
		m_blueprints[name] = bp;
	}
	else {
		bp = m_blueprints[name];
	}

	return bp;
}

ResourceManager::ResourceManager(RenderAPI* api) : m_api(api)
{
	
}
