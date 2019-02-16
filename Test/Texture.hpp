#pragma once
/*
	Contain a texture that could be applied to a mesh.
*/
class Texture {
public:
	virtual bool LoadFromFile(const char*) = 0;
protected:
	Texture();
};