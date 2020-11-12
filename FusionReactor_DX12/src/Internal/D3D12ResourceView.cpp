#include "stdafx.h"
#include "D3D12ResourceView.h"
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		D3D12ResourceView::D3D12ResourceView(DESCRIPTOR_HEAP_TYPE type, D3D12Utils::D3D12_DESCRIPTOR_HANDLE startSlot, uint32_t descriptorSize, uint32_t count) {
			m_type = type;
			m_descriptorSize = descriptorSize;
			m_startSlot = m_nextSlot = startSlot;
			m_descriptorCount = 0;
			m_descriptorCountMax = count;
		}

		DESCRIPTOR_HEAP_TYPE D3D12ResourceView::GetType()  const {
			return m_type;
		}

		size_t D3D12ResourceView::GetDescriptorCount()  const {
			return m_descriptorCount;
		}

		size_t D3D12ResourceView::GetDescriptorSize() const {
			return m_descriptorSize;
		}

		D3D12_CPU_DESCRIPTOR_HANDLE D3D12ResourceView::GetCPUHandle(uint32_t slotIndex) const {
			D3D12_CPU_DESCRIPTOR_HANDLE slot = m_startSlot.cdh;
			slot.ptr += (size_t)m_descriptorSize * slotIndex;
			return slot;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE D3D12ResourceView::GetGPUHandle(uint32_t slotIndex) const {
			D3D12_GPU_DESCRIPTOR_HANDLE slot = m_startSlot.gdh;
			slot.ptr += (size_t)m_descriptorSize * slotIndex;
			return slot;
		}

		D3D12Utils::D3D12_DESCRIPTOR_HANDLE D3D12ResourceView::GetHandle(uint32_t slotIndex) const {
			D3D12Utils::D3D12_DESCRIPTOR_HANDLE slot = m_startSlot;
			slot += m_descriptorSize * slotIndex;
			return slot;
		}

		D3D12Utils::D3D12_DESCRIPTOR_HANDLE D3D12ResourceView::GetNextHandle() const {
			return m_nextSlot;
		}

		D3D12Utils::D3D12_DESCRIPTOR_HANDLE D3D12ResourceView::AllocateSlot() {
			if ((m_descriptorCount + 1) > m_descriptorCountMax) {
				//TODO: Throw Error
				throw(false);
				return D3D12Utils::D3D12_DESCRIPTOR_HANDLE();
			}

			m_descriptorCount++;
			D3D12Utils::D3D12_DESCRIPTOR_HANDLE slot = m_nextSlot;
			m_nextSlot += m_descriptorSize;

			return slot;
		}

		D3D12ResourceView D3D12ResourceView::AllocateSlots(uint32_t nSlots) {

			if ((m_descriptorCount + nSlots) > m_descriptorCountMax) {
				//TODO: Throw Error
				throw(false);
				return D3D12ResourceView();
			}

			m_descriptorCount += nSlots;
			D3D12Utils::D3D12_DESCRIPTOR_HANDLE slot = m_nextSlot;
			m_nextSlot += m_descriptorSize * nSlots;

			return std::move(D3D12ResourceView(m_type, slot, m_descriptorSize, nSlots));
		}

		void D3D12ResourceView::Reset() {
			m_nextSlot = m_startSlot;
			m_descriptorCount = 0;
		}

		D3D12DescriptorHeap::~D3D12DescriptorHeap() {
			if (m_heapResource) {
				m_heapResource->Release();
				m_heapResource = nullptr;
			}
		}

		bool D3D12DescriptorHeap::Initialize(D3D12API* d3d12, DESCRIPTOR_HEAP_TYPE type, uint32_t numStatic, uint32_t numDynamic) {
			m_d3d12 = d3d12;

			D3D12_DESCRIPTOR_HEAP_DESC heapDesc;
			heapDesc.NumDescriptors = numStatic + numDynamic * NUM_GPU_BUFFERS;
			heapDesc.NodeMask = 0;
			switch (type) {
			case DESCRIPTOR_TYPE_UNKOWN:
				break;
			case DESCRIPTOR_TYPE_CBV_SRV_UAV:
				heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
				break;
			case DESCRIPTOR_TYPE_SAMPLER:
				heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
				break;
			case DESCRIPTOR_TYPE_RTV:
				heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
				break;
			case DESCRIPTOR_TYPE_DSV:
				heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
				heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
				break;
			default:
				return false;
				break;
			}

			HRESULT hr = d3d12->GetDevice()->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_heapResource));
			if (FAILED(hr)) {
				return false;
			}

			D3D12Utils::D3D12_DESCRIPTOR_HANDLE rangeDescriptorHandle = { m_heapResource->GetCPUDescriptorHandleForHeapStart(), m_heapResource->GetGPUDescriptorHandleForHeapStart() };
			m_descriptorSize = d3d12->GetDevice()->GetDescriptorHandleIncrementSize(heapDesc.Type);

			m_staticRange.m_startSlot = m_staticRange.m_nextSlot = rangeDescriptorHandle;
			m_staticRange.m_descriptorCount = 0;
			m_staticRange.m_descriptorCountMax = numStatic;
			m_staticRange.m_descriptorSize = m_descriptorSize;
			m_staticRange.m_type = type;

			rangeDescriptorHandle += m_descriptorSize * numStatic;

			//Create one dynamic ranges per GPU BUFFER
			for (size_t i = 0; i < NUM_GPU_BUFFERS; i++) {
				m_dynamicRange[i].m_startSlot = m_dynamicRange[i].m_nextSlot = rangeDescriptorHandle;
				m_dynamicRange[i].m_descriptorCount = 0;
				m_dynamicRange[i].m_descriptorCountMax = numDynamic;
				m_dynamicRange[i].m_descriptorSize = m_descriptorSize;
				m_dynamicRange[i].m_type = type;

				rangeDescriptorHandle += m_descriptorSize * numDynamic;
			}

			return true;
		}

		void D3D12DescriptorHeap::BeginFrame() {
			m_dynamicRange[m_d3d12->GetGPUBufferIndex()].Reset();
		}

		ID3D12DescriptorHeap* D3D12DescriptorHeap::GetDescriptorHeap() {
			return m_heapResource;
		}

		size_t D3D12DescriptorHeap::GetDescriptorSize() const {
			return m_descriptorSize;
		}

		D3D12ResourceView& D3D12DescriptorHeap::GetStaticRange() {
			return m_staticRange;
		}

		D3D12ResourceView& D3D12DescriptorHeap::GetDynamicRange() {
			return m_dynamicRange[m_d3d12->GetGPUBufferIndex()];
		}
	}
}