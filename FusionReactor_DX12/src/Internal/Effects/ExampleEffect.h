#include "../D3D12ShaderPass.h"

namespace FusionReactor {
	namespace FusionReactor_DX12 {
		class D3D12API;
		class D3D12Texture;

		class ExampleEffect : public D3D12RenderEffect {
		public:
			ExampleEffect();
			~ExampleEffect();
			bool Initialize(D3D12API* d3d12);

		private:
			bool InitializePass1();

		private:
			D3D12API* m_pApi;
			D3D12ShaderPass m_pass1;
			D3D12Texture* m_pExampleTexture;
		};

	}
}