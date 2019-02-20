#include "D3D12VertexBuffer.hpp"
#include "D3D12Renderer.hpp"
#include <d3d12.h>

D3D12VertexBuffer::D3D12VertexBuffer(D3D12Renderer* renderer)
{
	this->renderer = renderer;
}

D3D12VertexBuffer::~D3D12VertexBuffer()
{
	if (view)
	{
		delete view;
	}
}

bool D3D12VertexBuffer::Initialize(int nElements, int elementSize, void* data)
{
	mNumOfElements = nElements;
	mElementSize = elementSize;

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
	HRESULT hr = renderer->GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&resource)
	);

	if (FAILED(hr))
		return false;
	
	// Transfer data to upload heap
	void* dataBegin = nullptr;
	D3D12_RANGE range = { 0, 0 }; //We do not intend to read this resource on the CPU.
	resource->Map(0, &range, &dataBegin);
	memcpy(dataBegin, data, nElements * elementSize);
	resource->Unmap(0, nullptr);
	
	//Initialize vertex buffer view, used in the render call.
	view = new D3D12_VERTEX_BUFFER_VIEW;
	view->BufferLocation = resource->GetGPUVirtualAddress();
	view->StrideInBytes = elementSize;
	view->SizeInBytes = elementSize * nElements;

	resource->SetName(L"vb heap");
	return true;
}

ID3D12Resource1 * D3D12VertexBuffer::GetResource()
{
	return resource;
}

D3D12_VERTEX_BUFFER_VIEW * D3D12VertexBuffer::GetView()
{
	return view;
}

int D3D12VertexBuffer::GetNumberOfElements() const
{
	return mNumOfElements;
}

int D3D12VertexBuffer::GetElementSize() const
{
	return mElementSize;
}
