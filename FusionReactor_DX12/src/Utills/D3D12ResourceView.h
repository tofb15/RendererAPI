#pragma once
#include "../D3D12_FDecl.h"
#include "../DXR/D3D12Utils.h"

namespace FusionReactor {
	namespace FusionReactor_DX12 {

		class D3D12API;
		class D3D12DescriptorHeap;

		typedef enum DESCRIPTOR_HEAP_TYPE {
			DESCRIPTOR_TYPE_UNKOWN = 0,
			DESCRIPTOR_TYPE_CBV_SRV_UAV = (DESCRIPTOR_TYPE_UNKOWN + 1),
			DESCRIPTOR_TYPE_SAMPLER = (DESCRIPTOR_TYPE_CBV_SRV_UAV + 1),
			DESCRIPTOR_TYPE_RTV = (DESCRIPTOR_TYPE_SAMPLER + 1),
			DESCRIPTOR_TYPE_DSV = (DESCRIPTOR_TYPE_RTV + 1),
			DESCRIPTOR_TYPE_NUM_TYPES = (DESCRIPTOR_TYPE_DSV + 1)
		} 	DESCRIPTOR_HEAP_TYPE;

		class D3D12ResourceView {
		public:
			D3D12ResourceView() {};
			~D3D12ResourceView() {};

			DESCRIPTOR_HEAP_TYPE GetType() const;
			size_t GetDescriptorCount() const;
			size_t GetDescriptorSize() const;

			D3D12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t slot = 0) const;
			D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t slot = 0) const;
			D3D12Utils::D3D12_DESCRIPTOR_HANDLE GetHandle(uint32_t slot = 0) const;
			D3D12Utils::D3D12_DESCRIPTOR_HANDLE GetNextHandle() const;

			D3D12Utils::D3D12_DESCRIPTOR_HANDLE AllocateSlot();
			D3D12ResourceView AllocateSlots(uint32_t nSlots = 1);
			/*
				Resets the pointer to the next free slot to point at the descriptor heaps first slot.
				Warning, calling reset will invalidate all view-slots previously allocaded from this Resource View.
			*/
			void Reset();

		protected:
			friend class D3D12DescriptorHeap;

			uint32_t m_descriptorSize = 0;
			size_t m_descriptorCount = 0;
			size_t m_descriptorCountMax = 0;
			DESCRIPTOR_HEAP_TYPE m_type = DESCRIPTOR_TYPE_UNKOWN;

			D3D12Utils::D3D12_DESCRIPTOR_HANDLE m_startSlot = { 0 };
			D3D12Utils::D3D12_DESCRIPTOR_HANDLE m_nextSlot = { 0 };

			D3D12ResourceView(DESCRIPTOR_HEAP_TYPE type, D3D12Utils::D3D12_DESCRIPTOR_HANDLE startSlot, uint32_t descriptorSize, uint32_t count);
		};

		class D3D12DescriptorHeap {
		public:
			D3D12DescriptorHeap() {};
			~D3D12DescriptorHeap();
			bool Initialize(D3D12API* d3d12, DESCRIPTOR_HEAP_TYPE type, uint32_t numStatic, uint32_t numDynamic);
			void BeginFrame();
			ID3D12DescriptorHeap* GetDescriptorHeap();
			size_t GetDescriptorSize() const;

			/*
				Get the static part of the DescriptorHeap.
				The returned D3D12ResourceView and any content allocated on it will stay valid for the rest of the D3D12DescriptorHeaps lifetime.
			*/
			D3D12ResourceView& GetStaticRange();
			/*
				Get the dynamic part of the DescriptorHeap.
				The returned resource view and all content placed on it will be invalid after the next call to BeginFrame() and should not be cached for longer than the duration of the currently recorded frame.

				Use this to allocate descriptors that only require the lifetime of one frame.
			*/
			D3D12ResourceView& GetDynamicRange();

		private:
			D3D12API* m_d3d12 = nullptr;
			uint32_t m_descriptorSize = 0;
			ID3D12DescriptorHeap* m_heapResource = nullptr;

			D3D12ResourceView m_staticRange;
			D3D12ResourceView m_dynamicRange[NUM_GPU_BUFFERS];
		};
	}
}