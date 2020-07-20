#include "stdafx.h"
#include "ResourceManager.h"
#include "RenderAPI.hpp"
#include "Mesh.hpp"
#include "Texture.hpp"
#include "Material.hpp"

#include <filesystem>
#include <thread>
#include <chrono>
#include "Loaders/ConfigFileLoader.hpp"

ResourceManager* s_instance = nullptr;

ResourceManager* ResourceManager::GetInstance(RenderAPI* api) {
	if (!s_instance) {
		s_instance = MY_NEW ResourceManager(api);
	}

	return s_instance;
}

bool ResourceManager::SaveBlueprintToFile(std::vector<BlueprintDescription>& bpDescriptions) {
	for (auto& e : bpDescriptions) {
		std::string fName = std::string(BLUEPRINT_FOLDER_NAME) + e.blueprintName + ".bp";
		std::ofstream out(fName);

		//out << e.blueprintName << "\n";
		out << e.meshPath << "\n";
		out << e.alphaTested.size() << "\n";
		for (auto b : e.alphaTested) {
			out << (b ? "alphaTested" : "opaque") << "\n";
		}

		out << e.texturePaths.size() << "\n";
		for (auto& t : e.texturePaths) {
			out << t << "\n";
		}

		out.close();
	}

	return true;
}

bool ResourceManager::SaveBlueprintToFile(Blueprint* bp, const std::string& bpName) {
	std::string fName = m_assetPath + std::string(BLUEPRINT_FOLDER_NAME) + bpName + ".bp";
	std::ofstream out(fName);

	out << GetMeshName(bp->mesh) << "\n";
	//out << ((bp->allGeometryIsOpaque) ? "opaque" : "transparent") << "\n";

	//out << bp->alphaTested.size() << "\n";
	//for (auto b : bp->alphaTested) {
	//	out << (b ? "alphaTested" : "opaque") << "\n";
	//}

	//out << bp->textures.size() << "\n";
	//for (auto& t : bp->textures) {
	//	out << GetTextureName(t) << "\n";
	//}

	out.close();

	return true;
}

ResourceManager::~ResourceManager() {
	for (auto& e : m_blueprints) { if (e.second) { delete e.second; } }
	for (auto& e : m_meshes) { if (e.second) { delete e.second; } }
	for (auto& e : m_textures) { if (e.second) { delete e.second; } }
	for (auto& e : m_materials) { if (e.second) { delete e.second; } }
}

void ResourceManager::SetAssetPath(const std::string& s) {
	m_assetPath = s;
}

std::string ResourceManager::GetAssetPath() {
	return m_assetPath;
}

Blueprint* ResourceManager::LoadBlueprintFromFile(const std::string& name) {
	Blueprint* bp = MY_NEW Blueprint;
	std::string fName = m_assetPath + std::string(BLUEPRINT_FOLDER_NAME) + name + ".bp";

	ConfigLoader::ConfigTreeNode* configRoot = MY_NEW ConfigLoader::ConfigTreeNode;

	std::string errorString;
	bool result = ConfigLoader::Load(fName.c_str(), *configRoot, &errorString);

	if (result) {
		for (auto& setting : configRoot->subnodes) {
			if (!bp) {
				break;
			}

			if (setting->type == ConfigLoader::ConfigTreeNodeType::Setting) {
				const std::string& settingName = setting->subnodes[0]->value;
				const std::string& settingValue = setting->subnodes[1]->value;
				const ConfigLoader::ConfigTreeNodeType settingType = setting->subnodes[1]->type;

				if (settingName == "mesh") {
					//Load mesh
					if (settingType == ConfigLoader::ConfigTreeNodeType::String) {
						bp->mesh = GetMesh(settingValue);
						if (!bp->mesh) {
							delete bp;
							bp = nullptr;
						}
					}
				} else if (settingName == "materials") {
					//Load material(s)
					if (settingType == ConfigLoader::ConfigTreeNodeType::Array) {
						for (auto& mat : setting->subnodes[1]->subnodes) {
							if (mat->type == ConfigLoader::ConfigTreeNodeType::String) {
								Material* mt = GetMaterial(mat->value);
								bp->materials.push_back(mt);
							}
						}
					}
				} else {
					//Undefined setting
					std::cout << "Blueprint loader found unknown setting: \"" + settingName + "=" + (settingType == ConfigLoader::ConfigTreeNodeType::Array ? "{@Array}" : settingValue) + "\" in file: \"" + std::string(fName) + "\" \n";
				}
			}
		}
	} else {
		std::cout << "Could not load blueprint: " + errorString + " \n";
	}

	//TODO:: Remove this printing
	std::vector<bool> b = { false };
	configRoot->Print(b);

	configRoot->Delete();
	return bp;
}

Mesh* ResourceManager::GetMesh(const std::string& name) {
	Mesh* mesh = nullptr;
	std::string fName = m_assetPath + MESH_FOLDER_NAME + name;

	auto search = m_meshes.find(name);
	if (search == m_meshes.end()) {
		if (!DoesFileExist(fName)) {
			return nullptr;
		}

		mesh = m_api->MakeMesh();
		if (!mesh->LoadFromFile(fName.c_str(), Mesh::MESH_LOAD_FLAG_NONE)) {
			delete mesh;
			return nullptr;
		}
		m_meshes[name] = mesh;
	} else {
		mesh = m_meshes[name];
	}

	return mesh;
}

std::string ResourceManager::GetMeshName(Mesh* mesh) {
	for (auto& e : m_meshes) {
		if (e.second == mesh) {
			return e.first;
		}
	}
	return std::string();
}

Texture* ResourceManager::GetTexture(const std::string& name) {
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
	} else {
		texture = m_textures[name];
	}

	return texture;
}

Texture* ResourceManager::GetTextureCopy(const std::string& name, const std::string& copyName) {
	Texture* texture = nullptr;
	std::string fName = m_assetPath + TEXTURE_FODLER_NAME + name;

	auto search = m_textures.find(copyName);
	if (search == m_textures.end()) {
		if (!DoesFileExist(fName)) {
			return nullptr;
		}

		texture = m_api->MakeTexture();
		texture->LoadFromFile(fName.c_str(), Texture::TEXTURE_USAGE_GPU_FLAG);
		m_textures[copyName] = texture;
	} else {
		texture = m_textures[copyName];
	}

	return texture;
}

std::string ResourceManager::GetTextureName(Texture* texture) {
	for (auto& e : m_textures) {
		if (e.second == texture) {
			return e.first;
		}
	}
	return std::string();
}

bool ResourceManager::IsBlueprintLoaded(const std::string& name) {
	return m_blueprints.find(name) != m_blueprints.end();
}

Blueprint* ResourceManager::GetBlueprint(const std::string& name) {
	Blueprint* bp = nullptr;
	auto search = m_blueprints.find(name);
	if (search == m_blueprints.end()) {
		bp = LoadBlueprintFromFile(name.c_str());
		if (bp) {
			m_blueprints[name] = bp;
		}
	} else {
		bp = m_blueprints[name];
	}

	return bp;
}

Material* ResourceManager::GetMaterial(const std::string& name) {
	Material* e = nullptr;
	std::string fName = m_assetPath + MATERIAL_FOLDER_NAME + name;

	auto search = m_materials.find(name);
	if (search == m_materials.end()) {
		if (!DoesFileExist(fName)) {
			return nullptr;
		}

		e = m_api->MakeMaterial();
		e->LoadFromFile(fName.c_str(), *this);
		m_materials[name] = e;
	} else {
		e = m_materials[name];
	}

	return e;
}

ShaderProgramHandle ResourceManager::LoadShaderProgramFromFile(const std::string& fName) {
	ConfigLoader::ConfigTreeNode* configRoot = MY_NEW ConfigLoader::ConfigTreeNode;

	std::string errorString;
	bool result = ConfigLoader::Load(fName.c_str(), *configRoot, &errorString);

	ShaderDescription sd;
	ShaderProgramDescription spd;
	ShaderProgramHandle sph = -1;
	std::string entrypoint;
	std::string shaderFile;

	if (!result) {
		std::cout << "Could not load blueprint: " + errorString + " \n";
	} else {
		for (auto& setting : configRoot->subnodes) {
			if (setting->type == ConfigLoader::ConfigTreeNodeType::Setting) {
				const std::string& settingName = setting->subnodes[0]->value;
				const std::string& settingValue = setting->subnodes[1]->value;
				const ConfigLoader::ConfigTreeNodeType settingType = setting->subnodes[1]->type;

				if (settingName == "closesthit") {
					if (settingType == ConfigLoader::ConfigTreeNodeType::Array) {
						auto& arrayNodes = setting->subnodes[1]->subnodes;
						if (arrayNodes.size() == 2) {
							entrypoint = arrayNodes[0]->value;
							shaderFile = m_assetPath + std::string(SHADER_FOLDER_NAME) + arrayNodes[1]->value;

							if (!DoesFileExist(shaderFile)) {
								std::cout << "ShaderProgram loader tried to load non existing shader: \"" + settingName + "=" + "{" + entrypoint + ", " + shaderFile + "}" + "\" in file: \"" + std::string(fName) + "\" \n";
							} else {
								sd.defines.clear();
								sd.type = ShaderType::CLOSEST_HIT;
								sd.entrypoint = std::wstring(entrypoint.begin(), entrypoint.end());
								sd.filename = std::wstring(shaderFile.begin(), shaderFile.end());

								//Register shader to shader the manager
								spd.CHS = m_api->GetShaderManager()->RegisterShader(sd);
							}
						} else {
							std::cout << "ShaderProgram loader detected invalid setting arguments for: \"" + settingName + "=" + (settingType == ConfigLoader::ConfigTreeNodeType::Array ? "{@Array}" : settingValue) + "\" in file: \"" + std::string(fName) + "\". Expected an array with two elements.\n";
						}
					}
				} else if (settingName == "anyhit") {
					if (settingType == ConfigLoader::ConfigTreeNodeType::Array) {
						auto& arrayNodes = setting->subnodes[1]->subnodes;
						if (arrayNodes.size() == 2) {
							entrypoint = arrayNodes[0]->value;
							shaderFile = m_assetPath + std::string(SHADER_FOLDER_NAME) + arrayNodes[1]->value;

							if (!DoesFileExist(shaderFile)) {
								std::cout << "ShaderProgram loader tried to load non existing shader: \"" + settingName + "=" + "{" + entrypoint + ", " + shaderFile + "}" + "\" in file: \"" + std::string(fName) + "\" \n";
							} else {
								sd.defines.clear();
								sd.type = ShaderType::ANY_HIT;
								sd.entrypoint = std::wstring(entrypoint.begin(), entrypoint.end());
								sd.filename = std::wstring(shaderFile.begin(), shaderFile.end());
								//Register shader to shader the manager
								spd.AHS = m_api->GetShaderManager()->RegisterShader(sd);
							}
						} else {
							std::cout << "ShaderProgram loader detected invalid setting arguments for: \"" + settingName + "=" + (settingType == ConfigLoader::ConfigTreeNodeType::Array ? "{@Array}" : settingValue) + "\" in file: \"" + std::string(fName) + "\". Expected an array with two elements.\n";
						}
					}
				} else if (settingName == "intersection") {
					if (settingType == ConfigLoader::ConfigTreeNodeType::Array) {
						auto& arrayNodes = setting->subnodes[1]->subnodes;
						if (arrayNodes.size() == 2) {
							entrypoint = arrayNodes[0]->value;
							shaderFile = m_assetPath + std::string(SHADER_FOLDER_NAME) + arrayNodes[1]->value;

							if (!DoesFileExist(shaderFile)) {
								std::cout << "ShaderProgram loader tried to load non existing shader: \"" + settingName + "=" + "{" + entrypoint + ", " + shaderFile + "}" + "\" in file: \"" + std::string(fName) + "\" \n";
							} else {
								sd.defines.clear();
								sd.type = ShaderType::INTERSECTION;
								sd.entrypoint = std::wstring(entrypoint.begin(), entrypoint.end());
								sd.filename = std::wstring(shaderFile.begin(), shaderFile.end());
								//Register shader to shader the manager
								spd.IS = m_api->GetShaderManager()->RegisterShader(sd);
							}
						} else {
							std::cout << "ShaderProgram loader detected invalid setting arguments for: \"" + settingName + "=" + (settingType == ConfigLoader::ConfigTreeNodeType::Array ? "{@Array}" : settingValue) + "\" in file: \"" + std::string(fName) + "\". Expected an array with two elements.\n";
						}
					}
				} else {
					//Undefined setting
					std::cout << "ShaderProgram loader found unknown setting: \"" + settingName + "=" + (settingType == ConfigLoader::ConfigTreeNodeType::Array ? "{@Array}" : settingValue) + "\" in file: \"" + std::string(fName) + "\" \n";
				}
			}
		}

		sph = m_api->GetShaderManager()->RegisterShaderProgram(spd);
	}

	if (sph < 0) {
		std::cout << "ShaderProgram loader failed to register shader program to the shader manager. ShaderProgram file: \"" + std::string(fName) + "\" \n";
	}

	configRoot->Delete();
	return sph;
}

ShaderProgramHandle ResourceManager::GetShaderProgramHandle(const std::string& name) {
	ShaderProgramHandle sph = -1;
	std::string fName = m_assetPath + SHADER_PROGRAMS_FOLDER_NAME + name + ".txt";

	auto search = m_shaderPrograms.find(name);
	if (search == m_shaderPrograms.end()) {
		if (!DoesFileExist(fName)) {
			return sph;
		}

		sph = LoadShaderProgramFromFile(fName);
		if (sph >= 0) {
			m_shaderPrograms[name] = sph;
		}
	} else {
		sph = m_shaderPrograms[name];
	}

	return sph;
}

bool ResourceManager::PreLoadBlueprintFromFile(const std::string& path, Asset_Types assets_to_load) {
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
		for (size_t i = 0; i < tempInt; i++) {
			//Skip lines
			std::getline(in, line);
		}

		in >> tempInt;
		in.ignore();
		for (size_t i = 0; i < tempInt; i++) {
			std::getline(in, line);
			Texture* tempTexture = GetTexture(line);
			if (!tempTexture) {
				allGood = false;
			}
		}
	}

	return allGood;
}

bool ResourceManager::PreLoadBlueprint(const std::string& name, Asset_Types assets_to_load) {
	auto search = m_blueprints.find(name);
	if (search == m_blueprints.end()) {
		return PreLoadBlueprintFromFile(name.c_str(), assets_to_load);
	}

	return true;
}

Blueprint* ResourceManager::CreateBlueprint(const std::string& name) {
	Blueprint* bp = nullptr;
	auto search = m_blueprints.find(name);
	if (search == m_blueprints.end()) {
		bp = MY_NEW Blueprint;
		m_blueprints[name] = bp;
	}

	return bp;
}

std::string ResourceManager::GetBlueprintName(Blueprint* bp) {
	for (auto& e : m_blueprints) {
		if (e.second == bp) {
			return e.first;
		}
	}
	return std::string();
}

std::unordered_map<std::string, Blueprint*>& ResourceManager::GetBlueprints() {
	return m_blueprints;
}

bool ResourceManager::DoesFileExist(const std::string& s) {
	return std::filesystem::exists(s);
}

void ResourceManager::WaitUntilResourcesIsLoaded() {
	for (auto& e : m_textures) {
		while (!e.second->IsLoaded()) {
			std::this_thread::sleep_for(std::chrono::milliseconds(10));
		}
	}
}


ResourceManager::ResourceManager(RenderAPI* api) : m_api(api) {
}
