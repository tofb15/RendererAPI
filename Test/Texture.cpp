#include "Texture.hpp"

Texture::~Texture()
{
}

unsigned Texture::GetWidth() const
{
	return mWidth;
}

unsigned Texture::GetHeight() const
{
	return mHeight;
}

unsigned Texture::GetBPP() const
{
	return mBytesPerPixel;
}

Texture::Texture()
{
}
