#pragma once
#include "D3D12_FDecl.h"

namespace FusionReactor {
	namespace FusionReactor_DX12 {
		class D3D12API;

		class D3D12VertexBuffer {
		public:
			D3D12VertexBuffer(D3D12API* renderer);
			D3D12VertexBuffer(const D3D12VertexBuffer& buffer);

			virtual ~D3D12VertexBuffer();
			bool Initialize(int nElements, int elementSize, void* data);

			ID3D12Resource* GetResource();
			D3D12_VERTEX_BUFFER_VIEW* GetView();
			int GetNumberOfElements() const;
			int GetElementSize() const;

		private:
			int m_NumOfElements = 0;
			int m_ElementSize = 0;

			D3D12API* m_renderer = nullptr;
			ID3D12Resource* m_resource = nullptr;
			D3D12_VERTEX_BUFFER_VIEW* m_view = nullptr;
		};
	}
}