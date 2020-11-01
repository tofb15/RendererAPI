#include "stdafx.h"

#include "D3D12Texture.hpp"
#include "D3D12API.hpp"
#include "External/LodePNG/lodepng.h"
#include "External/D3DX12/d3dx12.h"
#include "External/DDS/DDSTextureLoader12.h"
#include "FusionReactor/src/Loaders/SimpleTexture.h"
#include "DXR/D3D12Utils.h"

#include <iostream>
#include <d3d12.h>
#include <cassert>

D3D12Texture::D3D12Texture(D3D12API* d3d12, unsigned short index) : m_d3d12(d3d12) {
	m_index = index;
}

D3D12Texture::~D3D12Texture() {
}

bool D3D12Texture::LoadFromFile(const char* fileName, unsigned flags) {
	if (flags == 0)
		return false;

	m_Flags = flags;
	m_BytesPerPixel = 4;

	std::string temp_path = fileName;
	for (auto& e : temp_path) {
		e = std::tolower(e);
	}
	m_fileName = temp_path;

	if (m_fileName.extension() == ".dds") {
		m_isDDS = true;
	}

	//If GPU_USAGE_FLAG is set, create a texture recorce on the GPU.
	if (flags & Texture_Load_Flags::TEXTURE_USAGE_GPU_FLAG) {
		//This should be done by a copy queue on a seperate thread.
		m_d3d12->GetTextureLoader()->LoadTextureToGPU(this);
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

bool D3D12Texture::IsLoaded() {
	return m_GPU_Loader_index != -1;
}

bool D3D12Texture::IsDDS() {
	return m_isDDS;
}

int D3D12Texture::GetTextureIndex() const {
	return m_GPU_Loader_index;
}

void D3D12Texture::UpdatePixel(const Int2& pos, const unsigned char* data, int size) {
	assert(pos.x >= 0 && pos.y >= 0 && "Pixel pos must be positive!");

	if (m_Flags & Texture_Load_Flags::TEXTURE_USAGE_CPU_FLAG) {
		std::memcpy(&m_Image_CPU[m_BytesPerPixel * ((unsigned int)pos.y * m_Width + (unsigned int)pos.x)], data, size);
		//&m_Image_CPU[m_BytesPerPixel * (pos.y*m_Width + pos.x)] = data * size;
		m_hasChanged = true;
	}
}

void D3D12Texture::ApplyChanges() {
	if (m_hasChanged) {
		m_d3d12->GetTextureLoader()->LoadTextureToGPU(this);
		m_d3d12->GetTextureLoader()->SynchronizeWork();
		m_hasChanged = false;
	}
}

std::vector<unsigned char>& D3D12Texture::GetData_addr() {
	return m_Image_CPU;
}

const std::vector<unsigned char>& D3D12Texture::GetData_addr_const() const {
	return m_Image_CPU;
}

std::vector<unsigned char> D3D12Texture::GetData_cpy() const {
	return m_Image_CPU;
}

std::vector<D3D12_SUBRESOURCE_DATA>& D3D12Texture::GetSubResourceData_DDS() {
	return m_subresources;
}

D3D12_RESOURCE_DESC D3D12Texture::GetTextureDescription() {
	return m_textureDesc;
}

bool D3D12Texture::LoadFromFile_Blocking(ID3D12Resource** ddsResource) {
	if (m_fileName.extension() == ".png") {
		unsigned error = lodepng::decode(m_Image_CPU, m_Width, m_Height, m_fileName.string());

		//if there's an error, display it
		if (error) {
			std::cout << "decoder error " << error << ": " << lodepng_error_text(error) << std::endl;
			return false;
		}

		m_textureDesc = {};
		m_textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM; // TODO: read this from texture data
		m_textureDesc.Width = m_Width;
		m_textureDesc.Height = m_Height;
		m_textureDesc.Alignment = 0;
		m_textureDesc.DepthOrArraySize = 1;
		m_textureDesc.SampleDesc.Count = 1;
		m_textureDesc.SampleDesc.Quality = 0;
		m_textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		m_textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		m_textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		m_textureDesc.MipLevels = 1;

		m_BytesPerPixel = 4;
	} else if (m_fileName.extension() == ".simpleTexture") {
		unsigned int nChannels;
		bool b = LOADER::SimpleTexture::LoadTexture(m_fileName.string(), m_Image_CPU, m_Width, m_Height, nChannels);

		if (b) {
			m_textureDesc = {};
			switch (nChannels) {
			case 1:
				m_textureDesc.Format = DXGI_FORMAT_R8_UNORM;
				m_BytesPerPixel = 1;
				break;
			case 2:
				m_textureDesc.Format = DXGI_FORMAT_R8G8_UNORM;
				m_BytesPerPixel = 2;
				break;
			case 4:
				m_textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
				m_BytesPerPixel = 4;
				break;
			default:
				break;
			}
			m_textureDesc.Width = m_Width;
			m_textureDesc.Height = m_Height;
			m_textureDesc.Alignment = 0;
			m_textureDesc.DepthOrArraySize = 1;
			m_textureDesc.SampleDesc.Count = 1;
			m_textureDesc.SampleDesc.Quality = 0;
			m_textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			m_textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			m_textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			m_textureDesc.MipLevels = 1;
		} else {
			//TODO:: Handle Error
		}
	} else if (m_isDDS) {
		std::wstring wide_string = m_fileName.c_str();

		HRESULT hr = DirectX::LoadDDSTextureFromFile(m_d3d12->GetDevice(), wide_string.c_str(), ddsResource, ddsData, m_subresources);
		if (FAILED(hr)) {
			ddsResource = nullptr;
			return false;
		}

		//m_d3d12->GetDevice()->CreateCommittedResource(&D3D12Utils::sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &m_textureDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(ddsResource));
		//ddsData.release();
		m_textureDesc = (*ddsResource)->GetDesc();
	} else {
		return false;
	}

	return true;
}
