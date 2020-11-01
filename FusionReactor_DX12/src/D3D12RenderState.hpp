#pragma once

#include "FusionReactor/src/RenderState.hpp"
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		/*
			Specific rendering states goes here like Wireframe, conservative rasterization etc.
		*/
		class D3D12RenderState : public RenderState {
		public:
			D3D12RenderState();
			virtual ~D3D12RenderState();

		};
	}
}