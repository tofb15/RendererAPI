#pragma once
#include <vector>
#include <string>
#include "Math.hpp"

class Texture;

struct MtlData
{
	std::string materialName;
	int illuminationModel;
	Float3 diffuseReflectivity;
	Float3 ambientReflectivity;
	Float3 specularReflectivity;
	Float3 transmissionFilter;
	std::vector<std::string> defaultTextureNames;
	int opticalDensity;
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
	virtual bool LoadFromFile(const char* name) = 0;
	//virtual void AddTexture(Texture*);

	const MtlData& GetMtlData() const;

protected:
	Material();

	MtlData mMtlData;
private:
	//std::vector<Texture*> textures;
};