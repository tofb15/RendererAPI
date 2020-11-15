#include "D3D12ShaderPass.h"	
#include <d3d12.h>

namespace FusionReactor {
	namespace FusionReactor_DX12 {

		D3D12ShaderPass::D3D12ShaderPass() {
			m_pDevice = nullptr;
			m_pPipelineState = nullptr;
			m_pRootSignature = nullptr;
		}

		D3D12ShaderPass::~D3D12ShaderPass() {
			if (m_pPipelineState) {
				m_pPipelineState->Release();
			}
			if (m_pRootSignature) {
				m_pRootSignature->Release();
			}
		}

		bool D3D12ShaderPass::Initialize(ID3D12Device* pDevice) {
			m_pPipelineState = nullptr;
			m_pRootSignature = nullptr;
			m_pDevice = pDevice;

			return true;
		}

		D3D12RenderEffect::D3D12RenderEffect() {

		}

		D3D12RenderEffect::~D3D12RenderEffect() {

		}
	}
}

