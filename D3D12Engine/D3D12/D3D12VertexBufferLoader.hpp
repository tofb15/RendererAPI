#ifndef D3D12_VERTEX_BUFFER_LOADER_HPP
#define D3D12_VERTEX_BUFFER_LOADER_HPP
#include <vector>
#include <Windows.h>

class D3D12API;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList3;
struct ID3D12Resource1;
struct ID3D12Heap;
struct ID3D12Fence1;

struct GPUBuffer {
	ID3D12Resource1* resource = nullptr;
	int nElements = 0;
	int elementSize = 0;
};

class D3D12VertexBufferLoader {
public:
	D3D12VertexBufferLoader(D3D12API* d3d12);
	~D3D12VertexBufferLoader();

	bool Initialize();

	GPUBuffer CreateBuffer(int nElements, int elementSize, void* data);

private:
	void WaitForGPU(const unsigned typeIndex);
	bool InitializeCommandInterfaces(const unsigned typeIndex);

	D3D12API* m_d3d12;

	static const unsigned COPY_INDEX = 0U;
	static const unsigned DIRECT_INDEX = 1U;

	ID3D12CommandQueue* m_commandQueues[2U] = { nullptr };
	ID3D12CommandAllocator* m_commandAllocators[2U] = { nullptr };
	ID3D12GraphicsCommandList3* m_commandLists[2U] = { nullptr };

	ID3D12Fence1* m_fence = nullptr;
	HANDLE m_eventHandle = nullptr;
	UINT64 m_fenceValue;
};

#endif