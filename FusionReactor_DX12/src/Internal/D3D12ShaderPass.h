#include "../D3D12_FDecl.h"
#include "D3D12ResourceView.h"

namespace FusionReactor {
	namespace FusionReactor_DX12 {

		class D3D12RenderEffect {
		public:
			D3D12RenderEffect();
			~D3D12RenderEffect();

			virtual bool Initialize() = 0;
		};

		class D3D12ShaderPass {
		public:

			D3D12ShaderPass();
			~D3D12ShaderPass();
			bool Initialize(ID3D12Device* pDevice);

		protected:
			ID3D12Device* m_pDevice;
			ID3D12PipelineState* m_pPipelineState;
			ID3D12RootSignature* m_pRootSignature;
			D3D12ResourceView m_descriptorTable;

		};

	}
}