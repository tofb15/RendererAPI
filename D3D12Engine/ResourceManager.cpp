#include "stdafx.h"
#include "ResourceManager.h"
#include "RenderAPI.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include <filesystem>

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
		std::string fName = std::string(BLUEPRINT_FOLDER_NAME) + e.blueprintName + ".bp";
		std::ofstream out(fName);

		//out << e.blueprintName << "\n";
		out << e.meshPath << "\n";
		out << e.alphaTested.size() << "\n";
		for (auto b : e.alphaTested)
		{
			out << (b ? "alphaTested" : "opaque") << "\n";
		}

		out << e.texturePaths.size() << "\n";
		for (auto& t : e.texturePaths)
		{
			out << t << "\n";
		}

		out.close();
	}

	return true;
}

bool ResourceManager::SaveBlueprintToFile(Blueprint* bp, std::string bpName)
{
	std::string fName = m_assetPath + std::string(BLUEPRINT_FOLDER_NAME) + bpName + ".bp";
	std::ofstream out(fName);

	out << GetMeshName(bp->mesh) << "\n";
	//out << ((bp->allGeometryIsOpaque) ? "opaque" : "transparent") << "\n";

	out << bp->alphaTested.size() << "\n";
	for (auto b : bp->alphaTested)
	{
		out << (b ? "alphaTested" : "opaque") << "\n";
	}

	out << bp->textures.size() << "\n";
	for (auto& t : bp->textures)
	{
		out << GetTextureName(t) << "\n";
	}

	out.close();

	return true;
}

ResourceManager::~ResourceManager()
{
	for (auto& e : m_blueprints){if (e.second) {delete e.second;}}
	for (auto& e : m_meshes)    {if (e.second) {delete e.second;}}
	for (auto& e : m_textures)  {if (e.second) {delete e.second;}}
}

void ResourceManager::SetAssetPath(std::string s) {
	m_assetPath = s;
}

Blueprint* ResourceManager::LoadBlueprintFromFile(std::string name)
{
	Blueprint* bp = MY_NEW Blueprint;
	std::string fName = m_assetPath + std::string(BLUEPRINT_FOLDER_NAME) + name + ".bp";

	std::ifstream in(fName);
	if (!in.is_open()) {
		delete bp;
		return nullptr;
	}
	std::string line;

	std::getline(in, line);
	bp->mesh = GetMesh(line);
	if (!bp->mesh) {
		delete bp;
		return nullptr;
	}

	int tempInt;
	in >> tempInt;
	in.ignore();
	for (size_t i = 0; i < tempInt; i++)
	{
		std::getline(in, line);
		bool b = (line == "alphaTested");

		bp->alphaTested.push_back(b);
		if (b) {
			bp->allGeometryIsOpaque = false;
		}
	}

	in >> tempInt;
	in.ignore();	
	for (size_t i = 0; i < tempInt; i++)
	{
		std::getline(in, line);
		bp->textures.push_back(GetTexture(line));
	}

	return bp;
}

Mesh* ResourceManager::GetMesh(std::string name)
{
	Mesh* mesh = nullptr;
	std::string fName = m_assetPath + MESH_FOLDER_NAME + name;

	auto search = m_meshes.find(name);
	if (search == m_meshes.end()) {
		if (!DoesFileExist(fName)) {
			return nullptr;
		}
		
		mesh = m_api->MakeMesh();
		if (!mesh->LoadFromFile(fName.c_str())) {
			delete mesh;
			return nullptr;
		}
		m_meshes[name] = mesh;
	}
	else {
		mesh = m_meshes[name];
	}

	return mesh;
}

std::string ResourceManager::GetMeshName(Mesh* mesh) {
	for (auto& e : m_meshes)
	{
		if (e.second == mesh) {
			return e.first;
		}
	}
	return std::string();
}

Texture* ResourceManager::GetTexture(std::string name)
{
	Texture* texture = nullptr;
	std::string fName = m_assetPath + TEXTURE_FODLER_NAME + name;

	auto search = m_textures.find(name);
	if (search == m_textures.end()) {
		if (!DoesFileExist(fName)) {
			return nullptr;
		}

		texture = m_api->MakeTexture();
		texture->LoadFromFile(fName.c_str(), Texture::TEXTURE_USAGE_GPU_FLAG);
		m_textures[name] = texture;
	}
	else {
		texture = m_textures[name];
	}

	return texture;
}

std::string ResourceManager::GetTextureName(Texture* texture) {
	for (auto& e : m_textures)
	{
		if (e.second == texture) {
			return e.first;
		}
	}
	return std::string();
}

bool ResourceManager::IsBlueprintLoaded(std::string name){
	return m_blueprints.find(name) != m_blueprints.end();
}

Blueprint* ResourceManager::GetBlueprint(std::string name)
{
	Blueprint* bp = nullptr;
	auto search = m_blueprints.find(name);
	if (search == m_blueprints.end()) {
		bp = LoadBlueprintFromFile(name.c_str());
		if (bp) {	
			m_blueprints[name] = bp;
		}
	}
	else {
		bp = m_blueprints[name];
	}

	return bp;
}

bool ResourceManager::PreLoadBlueprintFromFile(std::string path, Asset_Types assets_to_load)
{
	bool allGood = true;
	std::string fName = m_assetPath + std::string(BLUEPRINT_FOLDER_NAME) + path + ".bp";

	std::ifstream in(fName);
	if (!in.is_open()) {
		return false;
	}
	std::string line;

	std::getline(in, line);
	if (assets_to_load & Asset_Type_Model) {
		Mesh* tempMesh = GetMesh(line);
		if (!tempMesh) {
			allGood = false;
		}
	}

	if (assets_to_load & Asset_Type_Texture) {
		int tempInt;
		in >> tempInt;
		in.ignore();
		for (size_t i = 0; i < tempInt; i++)
		{
			//Skip lines
			std::getline(in, line);
		}

		in >> tempInt;
		in.ignore();
		for (size_t i = 0; i < tempInt; i++)
		{
			std::getline(in, line);
			Texture* tempTexture = GetTexture(line);
			if (!tempTexture) {
				allGood = false;
			}
		}
	}

	return allGood;
}

bool ResourceManager::PreLoadBlueprint(std::string name, Asset_Types assets_to_load)
{
	auto search = m_blueprints.find(name);
	if (search == m_blueprints.end()) {
		return PreLoadBlueprintFromFile(name.c_str(), assets_to_load);
	}

	return true;
}

Blueprint* ResourceManager::CreateBlueprint(std::string name) {
	Blueprint* bp = nullptr;
	auto search = m_blueprints.find(name);
	if (search == m_blueprints.end()) {
		bp = new Blueprint;
		m_blueprints[name] = bp;
	}

	return bp;
}

std::string ResourceManager::GetBlueprintName(Blueprint* bp)
{
	for (auto& e : m_blueprints)
	{
		if (e.second == bp) {
			return e.first;
		}
	}
	return std::string();
}

std::unordered_map<std::string, Blueprint*>& ResourceManager::GetBlueprints() {
	return m_blueprints;
}

bool ResourceManager::DoesFileExist(std::string s)
{
	return std::filesystem::exists(s);
}

void ResourceManager::WaitUntilResourcesIsLoaded() {
	//for (auto& e : m_textures)
	//{
	//	while (e.second->is)
	//	{

	//	}
	//}
}


ResourceManager::ResourceManager(RenderAPI* api) : m_api(api)
{
}
