#pragma once

#include "Internal/D3D12TextureLoader.hpp"
#include "FusionReactor/src/Texture.hpp"
#include "Internal/D3D12ResourceView.h"
#include "D3D12_FDecl.h"

#include <vector>
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		class D3D12API;
		class D3D12TextureLoader;

		/*
			Contain a texture that could be applied to a mesh.
		*/
		class D3D12Texture : public Texture {
		public:
			D3D12Texture(D3D12API* d3d12, unsigned short index);
			virtual ~D3D12Texture();

			// Inherited via Texture
			virtual bool LoadFromFile(const char* fileName, unsigned flags) override;
			bool IsLoaded() override;
			bool IsDDS();
			int GetTextureIndex() const;
			void UpdatePixel(const Int2& pos, const unsigned char* data, int size) override;
			void ApplyChanges() override;

			std::vector<unsigned char>& GetData_addr();
			const std::vector<unsigned char>& GetData_addr_const() const;
			std::vector<unsigned char> GetData_cpy() const;

			std::vector<D3D12_SUBRESOURCE_DATA>& GetSubResourceData_DDS();
			D3D12_RESOURCE_DESC GetTextureDescription();

			bool InitEmpty(uint32_t width, uint32_t heigth, DXGI_FORMAT format, D3D12_RESOURCE_FLAGS flag);
			void CreateSRV(D3D12ResourceView& view, uint32_t slotIndex) const;
			void CreateUAV(D3D12ResourceView& view, uint32_t slotIndex) const;

		private:
			friend D3D12TextureLoader;
			//Called from D3D12TextureLoader
			bool LoadFromFile_Blocking(ID3D12Resource** ddsResource = nullptr);

			//void load();
			//bool CreateGPUTextureResource();

			bool m_IsLoaded = false;
			bool m_isDDS = false;
			bool m_hasChanged = false;

			std::unique_ptr<uint8_t[]> ddsData;
			std::vector<D3D12_SUBRESOURCE_DATA> m_subresources;

			std::vector<unsigned char> m_Image_CPU; //the raw pixels stored on the CPU.
			D3D12API* m_d3d12;

			D3D12_RESOURCE_DESC m_textureDesc;
			/*Used by the Texture loader to find the right GPU address for this specific texture*/
			int m_GPU_Loader_index = -1;
			std::filesystem::path m_fileName;
		};
	}
}