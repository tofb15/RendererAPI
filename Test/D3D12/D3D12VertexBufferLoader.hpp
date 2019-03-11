#ifndef D3D12_VERTEX_BUFFER_LOADER_HPP
#define D3D12_VERTEX_BUFFER_LOADER_HPP
#include <vector>

class D3D12Renderer;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList3;
struct ID3D12Resource;
struct ID3D12Heap;

class D3D12VertexBufferLoader
{
public:
	D3D12VertexBufferLoader(D3D12Renderer* renderer);
	~D3D12VertexBufferLoader();

	bool Initialize();

	int LoadToGPU(int nElements, int elementSize, void* data);

private:
	struct GPUBuffer
	{
		ID3D12Resource1* resource;
		ID3D12Heap* heap;
	};

	std::vector<GPUBuffer> m_vertexBuffers;

	D3D12Renderer* m_renderer;
	ID3D12CommandQueue* m_commandQueue;
	ID3D12CommandAllocator* m_commandAllocator;
	ID3D12GraphicsCommandList3* m_commandList;
};

#endif