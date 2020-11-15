#include "stdafx.h"
#include <d3d12.h>

#include "D3D12GPUMemoryPool.h"
#include "D3D12API.hpp"

namespace FusionReactor {
	namespace FusionReactor_DX12 {



		D3D12GPUMemoryPool::D3D12GPUMemoryPool() {
			m_pApi = nullptr;
			m_pResourceHeap = nullptr;
			m_pMemmoryBlocks = nullptr;
		}

		D3D12GPUMemoryPool::~D3D12GPUMemoryPool() {

		}

		bool D3D12GPUMemoryPool::Initialize(size_t size, size_t blockSize, D3D12API* pApi) {

			m_blockSize = blockSize;
			// Allign size to blockSize
			m_nBlocks = (size + blockSize - 1) / blockSize;
			m_heapSize = m_nBlocks * blockSize;
			m_nMaskBytes = (m_nBlocks + 7) / 8;

			m_pApi = pApi;
			D3D12_HEAP_DESC desc = { 0 };
			desc.SizeInBytes = m_heapSize;
			desc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;
			desc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			desc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
			desc.Properties.VisibleNodeMask = 1; // Bitmask For multi GPU. passing 0 here is equivalent to passing 1 for d3d12 simplicity
			desc.Properties.CreationNodeMask = 1;
			desc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
			desc.Flags = D3D12_HEAP_FLAG_NONE;

			m_pApi->GetDevice()->CreateHeap(&desc, IID_PPV_ARGS(&m_pResourceHeap));

			//bitmask
			m_pMemmoryBlocks = new unsigned char[m_nMaskBytes];
			memset(m_pMemmoryBlocks, 0, m_nMaskBytes);

			return true;
		}

		bool D3D12GPUMemoryPool::Allocate(size_t size) {

			//1 = 1
			//2 = 10
			//3 = 11
			//4 = 100
			//5 = 101

			// [1][1][1][1][1][0][0][0] Mask
			// [0][0][0][0][0][1][1][1]	Bytemaks

			bool found = false;
			size_t currentBlockEnd = 0;
			size_t nBlocks = (size + m_blockSize - 1) / m_blockSize;
			size_t blocksLeft;

			size_t bitIndexAllocStart = 0;
			size_t bitIndexAllocEnd;

			while (bitIndexAllocStart < m_nBlocks && !found) {
				blocksLeft = nBlocks;
				bitIndexAllocEnd = bitIndexAllocStart;
				bool suffichentMemmory = true;
				while (bitIndexAllocEnd < m_nBlocks && !found && suffichentMemmory) {
					unsigned char blocksToAlloc = min(8 - bitIndexAllocEnd % 8, blocksLeft);
					unsigned char localBitIndex = bitIndexAllocStart % 8;
					unsigned char getMask = (0xFF >> (8 - blocksToAlloc)) << localBitIndex;
					unsigned char delta = (unsigned char)(getMask & m_pMemmoryBlocks[bitIndexAllocEnd / 8]);
					suffichentMemmory = delta == 0;

					if (suffichentMemmory) {
						blocksLeft -= blocksToAlloc;
						bitIndexAllocEnd += blocksToAlloc;

						if (blocksLeft == 0) {
							found = true;
						}
					} else {
						bitIndexAllocStart += (unsigned char)(log2(delta) + 0.5f) - localBitIndex;
					}
				}
			}

			if (found) {
				size_t setBitStartIndex = bitIndexAllocStart;
				unsigned char blocksToSet = nBlocks;
				while (blocksToSet) {
					unsigned char localBitIndex = setBitStartIndex % 8;
					unsigned char blocksToSetInByte = min(8 - localBitIndex, blocksToSet);

					unsigned char setMask = (0xFF >> (8 - blocksToSetInByte)) << localBitIndex;

					size_t setByteIndex = setBitStartIndex / 8;
					m_pMemmoryBlocks[setByteIndex] |= setMask;
					setBitStartIndex += blocksToSetInByte;

					blocksToSet -= blocksToSetInByte;
				}
			}

			return true;
		}

	}
}