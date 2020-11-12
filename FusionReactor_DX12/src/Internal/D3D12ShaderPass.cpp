#include "D3D12ShaderPass.h"	
#include <d3d12.h>

FusionReactor::FusionReactor_DX12::D3D12ShaderPass::D3D12ShaderPass(ID3D12Device* pDevice) {
	m_pipelineState = nullptr;
	m_rootSignature = nullptr;
	m_pDevice = pDevice;
}

FusionReactor::FusionReactor_DX12::D3D12ShaderPass::~D3D12ShaderPass() {
	if (m_pipelineState) {
		m_pipelineState->Release();
	}
	if (m_rootSignature) {
		m_rootSignature->Release();
	}
}
