#pragma once
#include <vector>
#include <d3d12.h>
#include <thread>
#include <mutex>
#include <filesystem>
#include <vector>
#include "D3D12_FDecl.h"

namespace FusionReactor {
	namespace FusionReactor_DX12 {
		class D3D12Texture;
		class D3D12API;

		class D3D12TextureLoader {
		public:

			D3D12TextureLoader(D3D12API* renderer);
			~D3D12TextureLoader();

			bool Initialize();

			bool CreateEmptyTexture(D3D12Texture* texture);
			void LoadTextureToGPU(D3D12Texture* texture);

			D3D12_GPU_DESCRIPTOR_HANDLE GetSpecificTextureGPUAdress(D3D12Texture* texture);
			D3D12_CPU_DESCRIPTOR_HANDLE GetSpecificTextureCPUAdress(D3D12Texture* texture);

			ID3D12Resource* GetResource(int index);

			//ID3D12DescriptorHeap* GetDescriptorHeap();
			unsigned GetNumberOfHeaps();
			ID3D12DescriptorHeap** GetAllHeaps();

			void SynchronizeWork();
			void Kill();

		private:
			void GPUUploaderDoWork();
			void RAMUploaderDoWork();

			void SignalAndWaitForCopy();
			void WaitForCopy(UINT64 fence);
			bool AddDescriptorHeap();

			static const int N_RAM_LOADER_THREADS = 2;

			std::mutex m_mutex_TextureResources;
			//std::mutex m_mutex_TextureResources2;

			std::condition_variable m_cv_gpu_not_empty;	//Notify that there is work to be done.
			std::condition_variable m_cv_gpu_empty;		//Notify that there is no work to be done.

			std::condition_variable m_cv_ram_not_empty;		//Notify that there is work to be done.
			std::condition_variable m_cv_ram_empty;			//Notify that there is no work to be done.

			std::thread m_gpu_upload_Worker;
			std::thread m_ram_upload_Worker[N_RAM_LOADER_THREADS];

			bool m_stop = false;
			bool m_atLeastOneTextureIsLoaded = false;

			//Temp Storage
			std::vector<D3D12Texture*> m_texturesToLoadToGPU;
			std::vector<D3D12Texture*> m_texturesToLoadToRAM;

			//Permanent Storage
			const unsigned MAX_SRVs_PER_DESCRIPTOR_HEAP = 100;
			unsigned int m_nrOfTextures = 0;
			std::vector<ID3D12DescriptorHeap*> m_descriptorHeaps;
			std::vector<ID3D12Resource*> m_textureResources;

			unsigned int m_CBV_SRV_UAV_DescriptorSize = 0;

			ID3D12Resource* m_uploadResource = nullptr;
			UINT m_uploadResource_Size = 0;

			D3D12API* m_d3d12;
			ID3D12CommandQueue* m_commandQueue = nullptr;
			ID3D12CommandAllocator* m_commandAllocator = nullptr;
			ID3D12GraphicsCommandList3* m_commandList = nullptr;

			//Fences for the render targets
			ID3D12Fence1* m_fence = nullptr;
			HANDLE						m_eventHandle = nullptr;
			UINT64						m_fenceValue = 0;
		};
	}
}