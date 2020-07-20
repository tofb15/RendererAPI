#pragma once

#include <vector>
#include <string>
#include <fstream>
#include <unordered_map>
#include <unordered_set>
#include "ShaderManager.hpp"

//#include "RenderAPI.hpp"

class Mesh;
class Texture;
class Material;
class Technique;
class RenderAPI;

const char MESH_FOLDER_NAME[] = "Models/";
const char TEXTURE_FODLER_NAME[] = "Textures/";
const char BLUEPRINT_FOLDER_NAME[] = "Blueprints/";
const char MATERIAL_FOLDER_NAME[] = "Materials/";
const char SHADER_FOLDER_NAME[] = "Shaders/";
const char SHADER_PROGRAMS_FOLDER_NAME[] = "ShaderPrograms/";


typedef size_t MaterialHandle;

enum Asset_Types : unsigned {
	Asset_Type_Texture = 0b001,
	Asset_Type_Model = 0b010,
	Asset_Type_Blueprint = 0b100,
	Asset_Type_Any = 0xffffffff,
};

/*
	Contain data used to describe a object and how it should be rendered.
	This class should ONLY be used as a blueprint/prefab to create other object copying data from this class.
*/
class Blueprint {
public:
	bool hasChanged = false;
	Mesh* mesh = nullptr;
	std::vector<Material*> materials;
};

/*
	More lightwight version of Blueprint which do not require any loaded resources.
*/
struct BlueprintDescription {
	std::string blueprintName;
	std::string meshPath;
	std::vector<bool> alphaTested;
	std::vector<std::string> texturePaths;
};

class ResourceManager {
public:
	static ResourceManager* GetInstance(RenderAPI* api);
	static bool SaveBlueprintToFile(std::vector<BlueprintDescription>& bpDescriptions);
	bool SaveBlueprintToFile(Blueprint* bp, const std::string& bpName);

	~ResourceManager();

	void SetAssetPath(const std::string& s);
	std::string GetAssetPath();
	Blueprint* LoadBlueprintFromFile(const std::string& path);
	Blueprint* GetBlueprint(const std::string& name);
	//MaterialHandle LoadMaterialFromFile(const std::string& path);
	Material* GetMaterial(const std::string& name);
	ShaderProgramHandle LoadShaderProgramFromFile(const std::string& name);
	ShaderProgramHandle GetShaderProgramHandle(const std::string& name);

	bool PreLoadBlueprintFromFile(const std::string& path, Asset_Types assets_to_load);
	bool PreLoadBlueprint(const std::string& name, Asset_Types assets_to_load = Asset_Type_Any);

	Blueprint* CreateBlueprint(const std::string& name);
	Mesh* GetMesh(const std::string& name);
	Texture* GetTexture(const std::string& name);
	/*
		Uses "name" to locate the texture on disk and loads it with the name specified in "copyName".
		If a texture is already loaded with the name specified by "copyName" that texture will be returned inseed.
	*/
	Texture* GetTextureCopy(const std::string& name, const std::string& copyName);

	std::string GetMeshName(Mesh* mesh);
	std::string GetTextureName(Texture* texture);
	std::string GetBlueprintName(Blueprint* bp);
	bool IsBlueprintLoaded(const std::string& name);
	std::unordered_map<std::string, Blueprint*>& GetBlueprints();

	bool DoesFileExist(const std::string& s);

	void WaitUntilResourcesIsLoaded();
public:

private:
	ResourceManager(RenderAPI* api);

private:
	RenderAPI* m_api;
	std::unordered_map<std::string, Mesh*>      m_meshes;
	std::unordered_map<std::string, Texture*>   m_textures;
	std::unordered_map<std::string, Blueprint*> m_blueprints;
	std::unordered_map<std::string, Material*>	m_materials;
	std::unordered_map<std::string, ShaderProgramHandle> m_shaderPrograms;

	std::string m_assetPath = "../assets/";
};