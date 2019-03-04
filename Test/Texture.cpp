#include "Texture.hpp"

Texture::~Texture()
{
}

unsigned Texture::GetWidth() const
{
	return m_Width;
}

unsigned Texture::GetHeight() const
{
	return m_Height;
}

unsigned Texture::GetBPP() const
{
	return m_BytesPerPixel;
}

Texture::Texture()
{
}
