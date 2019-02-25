#include "D3D12Texture.hpp"
#include "External/LodePNG/lodepng.h"
#include <iostream>
#include <d3d12.h>
#include "D3D12Renderer.hpp"
#include "External/D3DX12/d3dx12.h"

D3D12Texture::D3D12Texture(D3D12Renderer* renderer) : mRenderer(renderer)
{

}

D3D12Texture::~D3D12Texture()
{
}

bool D3D12Texture::LoadFromFile(const char * fileName, unsigned flags)
{
	if (flags == 0)
		return false;

	//decode
	unsigned error = lodepng::decode(mImage_CPU, mWidth, mHeight, fileName);
	mBytesPerPixel = 4;

	//if there's an error, display it
	if (error) {
		std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
		return false;
	}

	//If GPU_USAGE_FLAG is set, create a texture recorce on the GPU.
	if (flags & Texture_Load_Flags::TEXTURE_USAGE_GPU_FLAG) {
		//This should be done by a copy queue on a seperate thread.
		mRenderer->GetTextureLoader()->LoadTextureToGPU(this);
		mRenderer->GetTextureLoader()->SynchronizeWork();//Force this thread to wait until all work is done.
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
	return mIsLoaded;
}
