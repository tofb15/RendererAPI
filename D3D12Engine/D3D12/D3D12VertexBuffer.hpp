#pragma once

struct ID3D12Resource1;
struct D3D12_VERTEX_BUFFER_VIEW;
class D3D12API;

class D3D12VertexBuffer
{
public:
	D3D12VertexBuffer(D3D12API* renderer);
	D3D12VertexBuffer(const D3D12VertexBuffer& buffer);

	virtual ~D3D12VertexBuffer();
	bool Initialize(int nElements, int elementSize, void* data);

	ID3D12Resource1* GetResource();
	D3D12_VERTEX_BUFFER_VIEW* GetView();
	int GetNumberOfElements() const;
	int GetElementSize() const;

private:
	int m_NumOfElements;
	int m_ElementSize;

	D3D12API* m_renderer = nullptr;
	ID3D12Resource1* m_resource = nullptr;
	D3D12_VERTEX_BUFFER_VIEW* m_view = nullptr;
};
