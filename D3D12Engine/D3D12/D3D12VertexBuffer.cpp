#include "D3D12VertexBuffer.hpp"
#include "D3D12API.hpp"
#include "D3D12VertexBufferLoader.hpp"
#include <d3d12.h>

D3D12VertexBuffer::D3D12VertexBuffer(D3D12API* renderer)
{
	m_renderer = renderer;
}

D3D12VertexBuffer::~D3D12VertexBuffer()
{
	if (m_view)
	{
		delete m_view;
	}
	if (m_resource)
	{
		m_resource->Release();
	}
}

bool D3D12VertexBuffer::Initialize(int nElements, int elementSize, void* data)
{
	m_NumOfElements = nElements;
	m_ElementSize = elementSize;

#ifdef OLD_VERSION
	//Note: using upload heaps to transfer static data like vert buffers is not 
	//recommended. Every time the GPU needs it, the upload heap will be marshalled 
	//over. Please read up on Default Heap usage. An upload heap is used here for 
	//code simplicity and because there are very few vertices to actually transfer.
	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_UPLOAD;		// This "should" be changed
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC rd = {};
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = nElements * elementSize;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	//Creates both a resource and an implicit heap, such that the heap is big enough
	//to contain the entire resource and the resource is mapped to the heap. 
	HRESULT hr = m_renderer->GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&m_resource)
	);

	if (FAILED(hr))
		return false;
	
	// Transfer data to upload heap
	void* dataBegin = nullptr;
	D3D12_RANGE range = { 0, 0 }; //We do not intend to read this resource on the CPU.
	m_resource->Map(0, &range, &dataBegin);
	memcpy(dataBegin, data, nElements * elementSize);
	m_resource->Unmap(0, nullptr);

	//Initialize vertex buffer view, used in the render call.
	m_view = new D3D12_VERTEX_BUFFER_VIEW;
	m_view->BufferLocation = m_resource->GetGPUVirtualAddress();
	m_view->StrideInBytes = elementSize;
	m_view->SizeInBytes = elementSize * nElements;

#else
	GPUBuffer gpuBuffer = m_renderer->GetVertexBufferLoader()->CreateBuffer(nElements, elementSize, data);

	//Initialize vertex buffer view, used in the render call.
	m_view = new D3D12_VERTEX_BUFFER_VIEW;
	m_view->BufferLocation = gpuBuffer.resource->GetGPUVirtualAddress();
	m_view->StrideInBytes = elementSize;
	m_view->SizeInBytes = elementSize * nElements;
	m_resource = gpuBuffer.resource;

#endif

	m_resource->SetName(L"vb heap");
	return true;
}

ID3D12Resource1 * D3D12VertexBuffer::GetResource()
{
	return m_resource;
}

D3D12_VERTEX_BUFFER_VIEW * D3D12VertexBuffer::GetView()
{
	return m_view;
}

int D3D12VertexBuffer::GetNumberOfElements() const
{
	return m_NumOfElements;
}

int D3D12VertexBuffer::GetElementSize() const
{
	return m_ElementSize;
}
