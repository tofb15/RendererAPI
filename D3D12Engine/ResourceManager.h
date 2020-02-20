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
	std::vector<Technique*> techniques; //used for raster
	std::vector<Texture*>	textures;

	//===TODO: find a place to store these that is not here. They should be integrated in to the Technique
	std::vector<bool> alphaTested;   //for each geometry in the mesh; is it alphatested.
	bool allGeometryIsOpaque = true; //false if at least one geometry in the mesh is alphaTested
};

struct BlueprintDescription
{
	std::string blueprintName;
	std::string meshPath;
	std::vector<bool> alphaTested;
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
	Blueprint* GetBlueprint(std::string name);
	Blueprint* CreateBlueprint(std::string name);
	Mesh* GetMesh(std::string name);
	Texture* GetTexture(std::string name);

	std::string GetMeshName(Mesh* mesh);
	std::string GetTextureName(Texture* texture);
	std::string GetBlueprintName(Blueprint* bp);
	bool IsBlueprintLoaded(std::string name);
	std::unordered_map<std::string, Blueprint*>& GetBlueprints();

	bool DoesFileExist(std::string s);
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