#include "ExampleEffect.h"
#include "../../D3D12Texture.hpp"

namespace FusionReactor {
	namespace FusionReactor_DX12 {
		ExampleEffect::ExampleEffect() : D3D12RenderEffect() {
			m_pApi = nullptr;
		}

		ExampleEffect::~ExampleEffect() {

		}

		bool ExampleEffect::Initialize(D3D12API* pApi) {

			m_pApi = pApi;

			if (!InitializePass1()) {
				return false;
			}

			return true;
		}

		bool ExampleEffect::InitializePass1() {

			m_pExampleTexture = static_cast<D3D12Texture*>(m_pApi->MakeTexture());
			m_pExampleTexture->InitEmpty(10, 10, DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS);

			return true;
		}
	}
}