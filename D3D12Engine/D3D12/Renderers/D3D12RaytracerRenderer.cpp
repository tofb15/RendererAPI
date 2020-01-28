#include "D3D12RaytracerRenderer.h"
#include "..\D3D12Window.hpp"
#include "..\D3D12Texture.hpp"
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

bool D3D12RaytracerRenderer::InitializeOutputTextures(D3D12Window* window)
{
	//If window output has not changed keep the old resources, else create new
	if (window->GetDimensions() == m_outputDim) {
		return true;
	}

	//TODO: A Wait for GPU is Needed here

	m_outputDim = window->GetDimensions();

	HRESULT hr;
	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.Width = m_outputDim.x;
	textureDesc.Height = m_outputDim.y;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
	textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	D3D12_CLEAR_VALUE clearValue = { textureDesc.Format, { 0.00f, 0.00f, 0.00f, 1.0f } };

	for (int i = 0; i < NUM_GPU_BUFFERS; i++)
	{
		if (m_outputTextures[i]) {
			m_outputTextures[i]->Release();
		}
		hr = m_d3d12->GetDevice()->CreateCommittedResource(&D3D12Utils::sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_COMMON, &clearValue, IID_PPV_ARGS(&m_outputTextures[i]));
		if (FAILED(hr)) {
			return false;
		}
	}

	m_dxrBase->SetOutputResources(m_outputTextures, m_outputDim);

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
	InitializeOutputTextures(static_cast<D3D12Window*>(window));

	ResetCommandListAndAllocator(bufferIndex);
	ID3D12GraphicsCommandList4* cmdlist = m_commandLists[bufferIndex];

	D3D12_GPU_VIRTUAL_ADDRESS rtAddr = static_cast<D3D12Window*>(window)->GetCurrentRenderTargetGPUDescriptorHandle().ptr;
	
	D3D12Texture* tex = static_cast<D3D12Texture*>(m_OpaqueItems[0].blueprint->textures[0]);
	m_dxrBase->UpdateSceneData(static_cast<D3D12Camera*>(camera));
	m_dxrBase->UpdateAccelerationStructures(m_OpaqueItems, cmdlist);
	m_dxrBase->Dispatch(cmdlist);

	///Copy final output to window resource
	ID3D12Resource* windowOutput = static_cast<D3D12Window*>(window)->GetCurrentRenderTargetResource();

	D3D12Utils::SetResourceTransitionBarrier(cmdlist, m_outputTextures[bufferIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, 0);
	D3D12Utils::SetResourceTransitionBarrier(cmdlist, windowOutput, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST, 0);
	cmdlist->CopyResource(static_cast<D3D12Window*>(window)->GetCurrentRenderTargetResource(), m_outputTextures[bufferIndex]);
	D3D12Utils::SetResourceTransitionBarrier(cmdlist, windowOutput, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT, 0);
	D3D12Utils::SetResourceTransitionBarrier(cmdlist, m_outputTextures[bufferIndex], D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0);
}

void D3D12RaytracerRenderer::Present(Window* window)
{
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
	ID3D12GraphicsCommandList4* cmdlist = m_commandLists[bufferIndex];
	cmdlist->Close();
	ID3D12CommandList* listsToExecute[1] = { cmdlist };
	static_cast<D3D12Window*>(window)->GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	DXGI_PRESENT_PARAMETERS pp = {};
	static_cast<D3D12Window*>(window)->GetSwapChain()->Present1(0, 0, &pp);
	static_cast<D3D12Window*>(window)->WaitForGPU();

	//Lastly
	m_d3d12->IncGPUBufferIndex();
}

void D3D12RaytracerRenderer::ClearFrame()
{

}
