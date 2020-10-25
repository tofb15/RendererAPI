#include "../D3D12_FDecl.h"
#include "D3D12ResourceView.h"	

class D3D12API;

class D3D12DescriptorHeapManager {
public:
	D3D12DescriptorHeapManager(D3D12API* d3d12);
	~D3D12DescriptorHeapManager();
	bool Initialize(size_t nRTVStatic, size_t nRTVDynamic, size_t nDSVStatic, size_t nDSVDynamic, size_t nCBV_SRV_UAVStatic, size_t nCBV_SRV_UAVDynamic, size_t nSampStatic, size_t nSampDynamic);
	void BeginFrame();

	D3D12DescriptorHeap* GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE type);

private:
	D3D12API* m_d3d12 = nullptr;

	D3D12DescriptorHeap m_descriptorHeap_RTV;
	D3D12DescriptorHeap m_descriptorHeap_DSV;
	D3D12DescriptorHeap m_descriptorHeap_SRV_CBV_UAV;
	D3D12DescriptorHeap m_descriptorHeap_Samp;
};