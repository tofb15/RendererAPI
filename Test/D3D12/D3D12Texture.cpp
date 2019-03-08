#include "D3D12Texture.hpp"
#include "External/LodePNG/lodepng.h"
#include <iostream>
#include <d3d12.h>
#include "D3D12Renderer.hpp"
#include "External/D3DX12/d3dx12.h"

D3D12Texture::D3D12Texture(D3D12Renderer* renderer, unsigned short index) : m_Renderer(renderer)
{
	m_index = index;
}

D3D12Texture::~D3D12Texture()
{
}

bool D3D12Texture::LoadFromFile(const char * fileName, unsigned flags)
{
	if (flags == 0)
		return false;
	m_Flags = flags;

	//decode
	unsigned error = lodepng::decode(m_Image_CPU, m_Width, m_Height, fileName);
	m_BytesPerPixel = 4;

	//if there's an error, display it
	if (error) {
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		return false;
	}

	//If GPU_USAGE_FLAG is set, create a texture recorce on the GPU.
	if (flags & Texture_Load_Flags::TEXTURE_USAGE_GPU_FLAG) {
		//This should be done by a copy queue on a seperate thread.
		m_Renderer->GetTextureLoader()->LoadTextureToGPU(this);
		//mRenderer->GetTextureLoader()->SynchronizeWork();//Force this thread to wait until all work is done.
	}

	//If no CPU_USAGE_FLAG is set, remove cpu data to save RAM
	//if (!(flags & Texture_Load_Flags::TEXTURE_USAGE_CPU_FLAG)) {
	//	mImage_CPU.clear();
	//	mImage_CPU.shrink_to_fit();
	//}

	//the pixels are now in the vector "image", 4 bytes per pixel, ordered RGBARGBA..., use it as texture, draw it, ...

	return true;
}

bool D3D12Texture::IsLoaded()
{
	return m_GPU_Loader_index != -1;
}

int D3D12Texture::GetTextureIndex() const
{
	return m_GPU_Loader_index;
}

void D3D12Texture::UpdatePixel(Int2 pos, const unsigned char * data, int size)
{
	if (m_Flags & Texture_Load_Flags::TEXTURE_USAGE_CPU_FLAG) {
		std::memcpy(&m_Image_CPU[m_BytesPerPixel * (pos.y*m_Width + pos.x)], data, size);
		//&m_Image_CPU[m_BytesPerPixel * (pos.y*m_Width + pos.x)] = data * size;
		m_hasChanged = true;
	}
}

void D3D12Texture::ApplyChanges()
{
	if (m_hasChanged) {
		m_Renderer->GetTextureLoader()->LoadTextureToGPU(this);
		m_Renderer->GetTextureLoader()->SynchronizeWork();
		m_hasChanged = false;
	}
}
