#pragma once
#include "FusionReactor/src/Renderer.hpp"
#include "..\D3D12_FDecl.h"
#include "..\D3D12API.hpp"
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		class D3D12Renderer : public Renderer {
		public:
			D3D12Renderer(D3D12API* d3d12);
			virtual ~D3D12Renderer();
		protected:
			D3D12API* m_d3d12;

		private:

		};
	}
}