#include "D3D12RaytracerRenderer.h"
#include "..\D3D12Window.hpp"
#include "..\D3D12API.hpp"
#include "..\DXR\Temp\DXRBase.h"

#include <d3d12.h>

D3D12RaytracerRenderer::D3D12RaytracerRenderer(D3D12API* d3d12) : D3D12Renderer(d3d12)
{
	m_dxrBase = new DXRBase(m_d3d12);
}

D3D12RaytracerRenderer::~D3D12RaytracerRenderer()
{
	if (m_dxrBase) {
		delete m_dxrBase;
	}

}

bool D3D12RaytracerRenderer::Initialize()
{
	if (!m_dxrBase->Initialize()) {
		return false;
	}

	if (!InitializeCommandInterfaces()) {
		return false;
	}

	return true;
}

bool D3D12RaytracerRenderer::InitializeCommandInterfaces()
{
	HRESULT hr;

	for (size_t i = 0; i < NUM_GPU_BUFFERS; i++)
	{
		hr = m_d3d12->GetDevice()->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&m_commandAllocators[i]));
		if (FAILED(hr))
		{
			return false;
		}

		//Create command list.
		hr = m_d3d12->GetDevice()->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_commandAllocators[i],
			nullptr,
			IID_PPV_ARGS(&m_commandLists[i]));
		if (FAILED(hr))
		{
			return false;
		}

		//Command lists are created in the recording state. Since there is nothing to
		//record right now and the main loop expects it to be closed, we close it.
		m_commandLists[i]->Close();
	}

	return true;
}

void D3D12RaytracerRenderer::ResetCommandListAndAllocator(int index)
{
	HRESULT hr;
	hr = m_commandAllocators[index]->Reset();
	if (!SUCCEEDED(hr)) {
		printf("Error: Command allocator %d failed to reset\n", index);
	}

	hr = m_commandLists[index]->Reset(m_commandAllocators[index], nullptr);
	if (!SUCCEEDED(hr)) {
		printf("Error: Command list %d failed to reset\n", index);
	}
}

void D3D12RaytracerRenderer::Submit(SubmissionItem item, Camera* camera, unsigned char layer)
{
	m_OpaqueItems.emplace_back(item);
}

void D3D12RaytracerRenderer::ClearSubmissions()
{
	m_OpaqueItems.clear();
}

void D3D12RaytracerRenderer::Frame(Window* window, Camera* camera)
{
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
	ResetCommandListAndAllocator(bufferIndex);
	ID3D12GraphicsCommandList4* cmdlist = m_commandLists[bufferIndex];


	m_dxrBase->UpdateAccelerationStructures(m_OpaqueItems, cmdlist);


}

void D3D12RaytracerRenderer::Present(Window* window)
{
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
	ID3D12GraphicsCommandList4* cmdlist = m_commandLists[bufferIndex];
	cmdlist->Close();

	//Lastly
	m_d3d12->IncGPUBufferIndex();
}

void D3D12RaytracerRenderer::ClearFrame()
{

}
