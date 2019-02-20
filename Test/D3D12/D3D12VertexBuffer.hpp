#pragma once

struct ID3D12Resource1;
struct D3D12_VERTEX_BUFFER_VIEW;
class D3D12Renderer;

class D3D12VertexBuffer
{
public:
	D3D12VertexBuffer(D3D12Renderer* renderer);
	virtual ~D3D12VertexBuffer();

	bool Initialize(int nElements, int elementSize, void* data);

	ID3D12Resource1* GetResource();
	D3D12_VERTEX_BUFFER_VIEW* GetView();
	int GetNumberOfElements() const;
	int GetElementSize() const;

private:
	int mNumOfElements;
	int mElementSize;

	D3D12Renderer* renderer;
	ID3D12Resource1* resource;
	D3D12_VERTEX_BUFFER_VIEW* view;
};
