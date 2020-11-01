#include "stdafx.h"

#include "D3D12Renderer.h"
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		D3D12Renderer::D3D12Renderer(D3D12API* d3d12) : Renderer(), m_d3d12(d3d12) {

		}

		D3D12Renderer::~D3D12Renderer() {
		}
	}
}