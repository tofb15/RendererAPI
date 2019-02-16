#pragma once
#include <vector>

class Texture;

/*
	Describes the material and what shaders needed to render this material.
	Could contain data like how reflective the material is etc.
*/
class Material {
public:
	/*
		@param textureList, a pointer to a global texture list.

		If textureList == nullptr, no textures will be loaded from the material info.
		Otherwise the textures will be loaded, only if not found in the "textureList" param, and added to the material.

		@return true if Material was loaded successfully.
	*/
	virtual bool LoadFromFile(const char*, std::vector<Texture*>* textureList = nullptr) = 0;
	//virtual void AddTexture(Texture*);
protected:
	Material();
private:
	//std::vector<Texture*> textures;
};