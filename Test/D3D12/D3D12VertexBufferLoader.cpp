#include "D3D12VertexBufferLoader.hpp"
#include "D3D12Renderer.hpp"
#include <d3d12.h>

D3D12VertexBufferLoader::D3D12VertexBufferLoader(D3D12Renderer * renderer)
{
	m_renderer = renderer;
}

D3D12VertexBufferLoader::~D3D12VertexBufferLoader()
{
}

bool D3D12VertexBufferLoader::Initialize()
{
	ID3D12Device4* device = m_renderer->GetDevice();
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	HRESULT hr;
	
	cqd.Type = D3D12_COMMAND_LIST_TYPE_COPY;

	hr = device->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_commandQueue));
	if (FAILED(hr))
	{
		return false;
	}

	hr = device->CreateCommandAllocator(cqd.Type, IID_PPV_ARGS(&m_commandAllocator));
	if (FAILED(hr))
	{
		return false;
	}

	hr = device->CreateCommandList(0, cqd.Type, m_commandAllocator, nullptr, IID_PPV_ARGS(&m_commandList));
	if (FAILED(hr))
	{
		return false;
	}
	
	hr = m_commandList->Close();
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

int D3D12VertexBufferLoader::LoadToGPU(int nElements, int elementSize, void* data)
{
	HRESULT hr;
	m_commandAllocator->Reset();
	m_commandList->Reset(m_commandAllocator, nullptr);

	//
	// Create default heap and resource
	//

	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
	hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask = 0;
	hp.VisibleNodeMask = 0;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	rd.Width = nElements * elementSize;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ID3D12Resource1* defaultResource;
	hr = m_renderer->GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER,
		nullptr,
		IID_PPV_ARGS(&defaultResource)
	);
	if (FAILED(hr))
	{
		return false;
	}

	//
	// Create upload heap and resource
	//

	hp.Type = D3D12_HEAP_TYPE_UPLOAD;

	ID3D12Resource1* uploadResource;


	return true;
}
