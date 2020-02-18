#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
//#include "RenderAPI.hpp"

class Mesh;
class Texture;
class Technique;
class RenderAPI;

const char MESH_FOLDER_NAME[]      = "Models/";
const char TEXTURE_FODLER_NAME[]   = "Textures/";
const char BLUEPRINT_FOLDER_NAME[] = "Blueprints/";

/*
	Contain data used to describe a object and how it should be rendered.
	This class should ONLY be used as a blueprint/prefab to create other object copying data from this class.
*/
class Blueprint {
public:
	bool hasChanged = false;
	Mesh* mesh = nullptr;
	std::vector<Technique*> techniques;
	std::vector<Texture*>	textures;

	//RTX defines. TODO: find a place to store these that is not here.
	bool allGeometryIsOpaque = true;
};

struct BlueprintDescription
{
	std::string blueprintName;
	std::string meshPath;
	bool allGeometryIsOpaque = true;
	std::vector<std::string> texturePaths;
};

class ResourceManager
{
public:
	static ResourceManager* GetInstance(RenderAPI* api);
	static bool SaveBlueprintToFile(std::vector<BlueprintDescription>& bpDescriptions);
	bool SaveBlueprintToFile(Blueprint* bp, std::string bpName);

	~ResourceManager();

	void SetAssetPath(std::string s);
	Blueprint* LoadBlueprintFromFile(std::string path);
	Mesh* GetMesh(std::string name);
	std::string GetMeshName(Mesh* mesh);

	Texture* GetTexture(std::string name);
	std::string GetTextureName(Texture* texture);

	Blueprint* GetBlueprint(std::string name);
	Blueprint* CreateBlueprint(std::string name);

	std::string GetBlueprintName(Blueprint* bp);
	bool IsBlueprintLoaded(std::string name);
	std::unordered_map<std::string, Blueprint*>& GetBlueprints();

public:

private:
	ResourceManager(RenderAPI* api);

private:
	RenderAPI* m_api;
	std::unordered_map<std::string, Mesh*>      m_meshes;
	std::unordered_map<std::string, Texture*>   m_textures;
	std::unordered_map<std::string, Blueprint*> m_blueprints;

	std::string m_assetPath = "../assets/";
};