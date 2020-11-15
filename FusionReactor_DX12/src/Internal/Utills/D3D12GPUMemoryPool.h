#pragma once
#include "..\..\D3D12_FDecl.h"
#include <unordered_set>

namespace FusionReactor {
	namespace FusionReactor_DX12 {
		class D3D12API;

		class D3D12GPUMemoryPool {
		public:
			D3D12GPUMemoryPool();
			~D3D12GPUMemoryPool();

			bool Initialize(size_t size, size_t blockSize, D3D12API* pApi);
			bool Allocate(size_t size);

		private:
			D3D12API* m_pApi;

			ID3D12Heap* m_pResourceHeap;
			std::unordered_set<ID3D12Resource*> m_allocatedResources;


			size_t m_blockSize;
			size_t m_heapSize;
			size_t m_nBlocks;
			size_t m_nMaskBytes;

			unsigned char* m_pMemmoryBlocks;
		};
	}
}