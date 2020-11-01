#include "stdafx.h"

#include "Texture.hpp"
namespace FusionReactor {

	Texture::~Texture() {
	}

	unsigned Texture::GetWidth() const {
		return m_Width;
	}

	unsigned Texture::GetHeight() const {
		return m_Height;
	}

	unsigned Texture::GetBPP() const {
		return m_BytesPerPixel;
	}

	unsigned short Texture::GetIndex() {
		return m_index;
	}

	unsigned Texture::GetFlags() const {
		return m_Flags;
	}

	Texture::Texture() {
	}
}