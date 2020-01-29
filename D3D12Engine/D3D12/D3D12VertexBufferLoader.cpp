#include "stdafx.h"

#include "D3D12VertexBufferLoader.hpp"
#include "D3D12API.hpp"
#include <d3d12.h>

D3D12VertexBufferLoader::D3D12VertexBufferLoader(D3D12API * renderer)
{
	m_renderer = renderer;
}

D3D12VertexBufferLoader::~D3D12VertexBufferLoader()
{
	for (int i = 0; i < 2; i++)
	{
		m_commandQueues[i]->Release();
		m_commandLists[i]->Release();
		m_commandAllocators[i]->Release();
	}
	m_fence->Release();
}

bool D3D12VertexBufferLoader::Initialize()
{
	ID3D12Device5* device = m_renderer->GetDevice();
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	HRESULT hr;
	
	if (!InitializeCommandInterfaces(COPY_INDEX))
	{
		return false;
	}
	if (!InitializeCommandInterfaces(DIRECT_INDEX))
	{
		return false;
	}

	hr = device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	if (FAILED(hr))
	{
		return false;
	}

	m_fenceValue = 1;
	m_eventHandle = CreateEvent(0, false, false, 0);

	return true;
}

bool D3D12VertexBufferLoader::InitializeCommandInterfaces(const unsigned typeIndex)
{
	ID3D12Device5* device = m_renderer->GetDevice();
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	HRESULT hr;

	if (typeIndex == COPY_INDEX)
		cqd.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	else if (typeIndex == DIRECT_INDEX)
		cqd.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

	hr = device->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_commandQueues[typeIndex]));
	if (FAILED(hr))
	{
		return false;
	}

	hr = device->CreateCommandAllocator(cqd.Type, IID_PPV_ARGS(&m_commandAllocators[typeIndex]));
	if (FAILED(hr))
	{
		return false;
	}

	hr = device->CreateCommandList(0, cqd.Type, m_commandAllocators[typeIndex], nullptr, IID_PPV_ARGS(&m_commandLists[typeIndex]));
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_commandLists[typeIndex]->Close();
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

GPUBuffer D3D12VertexBufferLoader::CreateBuffer(int nElements, int elementSize, void * data)
{
	GPUBuffer gpuBuffer;
	HRESULT hr;
	D3D12_HEAP_PROPERTIES hp = {};
	D3D12_RESOURCE_DESC rd = {};
	D3D12_RESOURCE_BARRIER rb = {};

	ID3D12Resource1* defaultResource;
	ID3D12Resource1* uploadResource;

	//
	// Create default heap and resource
	//

	hp.Type = D3D12_HEAP_TYPE_DEFAULT;
	hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;	// D3D12_CPU_PAGE_PROPERTY_NOT_AVAILABLE did not work on default heap
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask = 0;
	hp.VisibleNodeMask = 0;

	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Alignment = 0;
	rd.Width = nElements * elementSize;
	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.SampleDesc.Count = 1;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	hr = m_renderer->GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(&defaultResource)
	);
	if (FAILED(hr))
	{
		return gpuBuffer;
	}

	//
	// Create upload heap and resource
	//

	hp.Type = D3D12_HEAP_TYPE_UPLOAD;

	hr = m_renderer->GetDevice()->CreateCommittedResource(
		&hp,
		D3D12_HEAP_FLAG_NONE,
		&rd,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(&uploadResource)
	);
	if (FAILED(hr))
	{
		return gpuBuffer;
	}

	//
	// Copy data to upload resource
	//
	void* mappedData = nullptr;
	D3D12_RANGE readRange = { 0, 0 };
	uploadResource->Map(0, &readRange, &mappedData);
	memcpy(mappedData, data, nElements * elementSize);

	D3D12_RANGE writeRange = { 0, static_cast<size_t>(nElements * elementSize) };

	uploadResource->Unmap(0, &writeRange);


	//
	// Begin copy command list recording
	//
	m_commandAllocators[COPY_INDEX]->Reset();
	m_commandLists[COPY_INDEX]->Reset(m_commandAllocators[COPY_INDEX], nullptr);

	rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
	rb.Transition.pResource = defaultResource;
	rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	rb.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
	m_commandLists[COPY_INDEX]->ResourceBarrier(1, &rb);

	// Copy from upload to default
	m_commandLists[COPY_INDEX]->CopyResource(defaultResource, uploadResource);

	rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	rb.Transition.StateAfter = D3D12_RESOURCE_STATE_COMMON;		// Required, since this resource will be used in another list/queue type later
	m_commandLists[COPY_INDEX]->ResourceBarrier(1, &rb);

	m_commandLists[COPY_INDEX]->Close();

	// Execute copying
	ID3D12CommandList* copyCommandLists[] = { m_commandLists[COPY_INDEX] };
	m_commandQueues[COPY_INDEX]->ExecuteCommandLists(1, copyCommandLists);

	WaitForGPU(COPY_INDEX);

	// Release the resource once the data has been copied
	uploadResource->Release();

	//
	// Begin direct command list recording
	//
	m_commandAllocators[DIRECT_INDEX]->Reset();
	m_commandLists[DIRECT_INDEX]->Reset(m_commandAllocators[DIRECT_INDEX], nullptr);

	// The only reason this queue and list exists is to transition to this state (and it did NOTHING... (in our test case))
	rb.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	rb.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	m_commandLists[DIRECT_INDEX]->ResourceBarrier(1, &rb);

	m_commandLists[DIRECT_INDEX]->Close();

	// Execute transition
	ID3D12CommandList* directCommandLists[] = { m_commandLists[DIRECT_INDEX] };
	m_commandQueues[DIRECT_INDEX]->ExecuteCommandLists(1, directCommandLists);
	
	WaitForGPU(DIRECT_INDEX);

	gpuBuffer.resource = defaultResource;
	gpuBuffer.nElements = nElements;
	gpuBuffer.elementSize = elementSize;

	return gpuBuffer;
}

void D3D12VertexBufferLoader::WaitForGPU(const unsigned typeIndex)
{
	const UINT64 fence = m_fenceValue;
	m_commandQueues[typeIndex]->Signal(m_fence, fence);
	m_fenceValue++;

	if (m_fence->GetCompletedValue() < fence)
	{
		m_fence->SetEventOnCompletion(fence, m_eventHandle);
		WaitForSingleObject(m_eventHandle, INFINITE);
	}
}