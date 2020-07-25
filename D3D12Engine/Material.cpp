#include "stdafx.h"

#include "Material.hpp"
#include "Loaders/ConfigFileLoader.hpp"
#include "ResourceManager.h"

Material::~Material() {
}

bool Material::LoadFromFile(const char* name, ResourceManager& resourceManager) {
	ConfigLoader::ConfigTreeNode* configRoot = MY_NEW ConfigLoader::ConfigTreeNode();
	std::string errorString;
	bool result = ConfigLoader::Load(name, *configRoot, &errorString);

	if (result) {
		for (auto& setting : configRoot->subnodes) {
			if (setting->type == ConfigLoader::ConfigTreeNodeType::Setting) {
				const std::string& settingName = setting->subnodes[0]->value;
				const std::string& settingValue = setting->subnodes[1]->value;
				const ConfigLoader::ConfigTreeNodeType settingType = setting->subnodes[1]->type;

				if (settingName == "textureColor") {
					if (settingType == ConfigLoader::ConfigTreeNodeType::String) {
						m_materialData.pbrData.color = resourceManager.GetTexture(settingValue);
					}
				} else if (settingName == "textureNormal") {
					if (settingType == ConfigLoader::ConfigTreeNodeType::String) {
						m_materialData.pbrData.normal = resourceManager.GetTexture(settingValue);
					}
				} else if (settingName == "textureRoughness") {
					if (settingType == ConfigLoader::ConfigTreeNodeType::String) {
						m_materialData.pbrData.roughness = resourceManager.GetTexture(settingValue);
					}
				} else if (settingName == "textureMetalness") {
					if (settingType == ConfigLoader::ConfigTreeNodeType::String) {
						m_materialData.pbrData.metalness = resourceManager.GetTexture(settingValue);
					}
				} else if (settingName == "shaderGroup") {
					if (settingType == ConfigLoader::ConfigTreeNodeType::String) {
						m_shaderProgram = resourceManager.GetShaderProgramHandle(settingValue);
					}
				} else {
					//Undefined setting
					std::cout << "Material Loader found unknown setting: \"" + settingName + "=" + settingValue + "\" in file:" + std::string(name) + " \n";
				}
			}
		}
	} else {
		std::cout << "Could not load material: " + errorString + " \n";
	}

	configRoot->Delete();
	return result;
}

bool Material::SaveToFile(const char* fName, ResourceManager& rm) {

	std::ofstream out(fName);
	if (!out.is_open()) {
		//TODO: Log error
		return false;
	}

	out << "#Material Types [pbr, nonpbr](default: non_pbr)\n";
	out << "materialType = pbr\n";
	out << "\n";
	out << "#Textures. Which textures is needed is defined by the materialType. (default: )\n";
	out << "textureColor = " << rm.GetTextureName(m_materialData.pbrData.color) << "\n";
	out << "textureNormal = " << rm.GetTextureName(m_materialData.pbrData.normal) << "\n";
	out << "textureRoughness = " << rm.GetTextureName(m_materialData.pbrData.roughness) << "\n";
	out << "textureMetalness = " << rm.GetTextureName(m_materialData.pbrData.metalness) << "\n";
	out << "\n";
	out << "#shaderGroup\n";
	out << "shaderGroup = " << rm.GetShaderProgramName(m_shaderProgram);
	return true;
}


Material::Material() {
}

ShaderProgramHandle Material::GetShaderProgram() const {
	return m_shaderProgram;
}
void Material::SetShaderProgram(ShaderProgramHandle sp) {
	m_changed = true;
	m_shaderProgram = sp;
}

bool Material::HasChanged() {
	return m_changed;
}
void Material::SetHasChanged(bool b) {
	m_changed = b;
}