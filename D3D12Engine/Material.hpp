#pragma once
#include <vector>
#include <string>
#include "Math.hpp"
#include "ShaderManager.hpp"

class Texture;
class ResourceManager;

enum class MaterialType {
	PBR,
	Normal
};

struct PBR_Material_Data {
	Texture* color;
	Texture* normal;
	Texture* roughness;
	Texture* metalness;
};

/*
	Describes the material and what shaders needed to render this material.
	Could contain data like how reflective the material is etc.
*/
class Material {
public:
	virtual ~Material();
	/*
		@param name, the file name of the material

		@return true if Material was loaded successfully.
	*/
	virtual bool LoadFromFile(const char* name, ResourceManager& resourceManager);
	//const MaterialData& GetMtlData() const;
	union {
		PBR_Material_Data pbrData;
		//Non_PBR_Material_Data nonPbrData;
	} m_materialData;

	ShaderProgramHandle m_shaderProgram;
protected:
	Material();
	//MaterialType type;
private:
};