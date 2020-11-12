#include "../D3D12_FDecl.h"
#include "D3D12ResourceView.h"

namespace FusionReactor {
	namespace FusionReactor_DX12 {

		class D3D12RenderEffect {
		public:
			D3D12RenderEffect();
			~D3D12RenderEffect();
		};

		class D3D12ShaderPass {
		public:
			D3D12ShaderPass(ID3D12Device* pDevice);
			~D3D12ShaderPass();

		private:
			ID3D12PipelineState* m_pipelineState;
			ID3D12RootSignature* m_rootSignature;
			D3D12ResourceView m_descriptorTable;

			ID3D12Device* m_pDevice;
		};

	}
}