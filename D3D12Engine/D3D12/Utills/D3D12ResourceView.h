#pragma once
#include "../D3D12_FDecl.h"
#include "../DXR/D3D12Utils.h"

class D3D12API;

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

	D3D12Utils::D3D12_DESCRIPTOR_HANDLE AllocateSlot();
	D3D12ResourceView AllocateSlots(uint32_t nSlots = 1);
	/*
		Resets the pointer to the next free slot to point at the descriptor heaps first slot.
		Warning, calling reset will invalidate all view-slots previously allocaded from this Resource View.
	*/
	void Reset();

protected:
	uint32_t m_descriptorSize = 0;
	size_t m_descriptorCount = 0;
	size_t m_descriptorCountMax = 0;
	DESCRIPTOR_HEAP_TYPE m_type = DESCRIPTOR_TYPE_UNKOWN;

	D3D12Utils::D3D12_DESCRIPTOR_HANDLE m_startSlot = { 0 };
	D3D12Utils::D3D12_DESCRIPTOR_HANDLE m_nextSlot = { 0 };

	D3D12ResourceView(DESCRIPTOR_HEAP_TYPE type, D3D12Utils::D3D12_DESCRIPTOR_HANDLE startSlot, uint32_t descriptorSize, uint32_t count);
};

class D3D12DescriptorHeap : public D3D12ResourceView {
public:
	D3D12DescriptorHeap() {};
	~D3D12DescriptorHeap();
	bool Initialize(D3D12API* d3d12, DESCRIPTOR_HEAP_TYPE type, uint32_t num);
	ID3D12DescriptorHeap* GetDescriptorHeap();

private:
	D3D12API* m_d3d12 = nullptr;
	ID3D12DescriptorHeap* m_heapResource = nullptr;
};
