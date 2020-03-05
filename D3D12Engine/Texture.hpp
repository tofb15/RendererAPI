#pragma once
/*
	Contain a texture that could be applied to a mesh.
*/

#include "Math.hpp"

class Texture {
public:
	enum Texture_Load_Flags {
		TEXTURE_USAGE_CPU_FLAG = 1,
		TEXTURE_USAGE_GPU_FLAG = 2,
	};

	virtual ~Texture();

	/*

		Load a PNG to memory and stores it on the CPU, GPU or both depending on the flags.

		@param fileName, relative path to the image
		@param flags, a bitwise combinations of Texture::Texture_Load_Flags that specifies the usage of the texture

	*/
	virtual bool LoadFromFile(const char* fileName, unsigned flags) = 0;

	/*
		@Return Width of texture
	*/
	virtual unsigned GetWidth()		const;

	/*
		@Return Height of texture
	*/
	virtual unsigned GetHeight()	const;

	/*
		@Return Bytes Per Pixel (sizeof(format))
	*/
	virtual unsigned GetBPP()		const;

	virtual void UpdatePixel(Int2 pos, const unsigned char* data, int size) = 0;
	virtual void ApplyChanges() = 0;
	virtual unsigned short GetIndex();
	virtual bool IsLoaded() = 0;
	unsigned GetFlags() const;

protected:
	unsigned m_Flags;
	unsigned short m_index;
	unsigned m_Width, m_Height;
	unsigned m_BytesPerPixel;
	Texture();
};