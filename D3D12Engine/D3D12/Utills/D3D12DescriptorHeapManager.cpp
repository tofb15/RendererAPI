#include "stdafx.h"
#include <d3d12.h>

#include "D3D12DescriptorHeapManager.hpp"
#include "../D3D12API.hpp"

D3D12DescriptorHeapManager::D3D12DescriptorHeapManager(D3D12API* d3d12) : m_d3d12(d3d12) {
}

D3D12DescriptorHeapManager::~D3D12DescriptorHeapManager() {
}

bool D3D12DescriptorHeapManager::Initialize(size_t nRTVStatic, size_t nRTVDynamic, size_t nDSVStatic, size_t nDSVDynamic, size_t nCBV_SRV_UAVStatic, size_t nCBV_SRV_UAVDynamic, size_t nSampStatic, size_t nSampDynamic) {
	if (!m_descriptorHeap_RTV.Initialize(m_d3d12, DESCRIPTOR_TYPE_RTV, nRTVStatic, nRTVDynamic)) {
		return false;
	}

	if (!m_descriptorHeap_DSV.Initialize(m_d3d12, DESCRIPTOR_TYPE_DSV, nDSVStatic, nDSVDynamic)) {
		return false;
	}

	if (!m_descriptorHeap_SRV_CBV_UAV.Initialize(m_d3d12, DESCRIPTOR_TYPE_CBV_SRV_UAV, nCBV_SRV_UAVStatic, nCBV_SRV_UAVDynamic)) {
		return false;
	}

	if (!m_descriptorHeap_Samp.Initialize(m_d3d12, DESCRIPTOR_TYPE_SAMPLER, nSampStatic, nSampDynamic)) {
		return false;
	}

	return true;
}

void D3D12DescriptorHeapManager::BeginFrame() {
	m_descriptorHeap_RTV.BeginFrame();
	m_descriptorHeap_DSV.BeginFrame();
	m_descriptorHeap_SRV_CBV_UAV.BeginFrame();
	m_descriptorHeap_Samp.BeginFrame();
}

D3D12DescriptorHeap* D3D12DescriptorHeapManager::GetDescriptorHeap(DESCRIPTOR_HEAP_TYPE type) {
	switch (type) {
	case DESCRIPTOR_TYPE_CBV_SRV_UAV:
		return &m_descriptorHeap_SRV_CBV_UAV;
		break;
	case DESCRIPTOR_TYPE_SAMPLER:
		return &m_descriptorHeap_Samp;
		break;
	case DESCRIPTOR_TYPE_RTV:
		return &m_descriptorHeap_RTV;
		break;
	case DESCRIPTOR_TYPE_DSV:
		return &m_descriptorHeap_DSV;
		break;
	default:
		return nullptr;
		break;
	}
}