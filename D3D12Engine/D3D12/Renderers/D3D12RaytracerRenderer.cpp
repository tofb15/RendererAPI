#include "stdafx.h"

#include "D3D12RaytracerRenderer.h"
#include "..\D3D12Window.hpp"
#include "..\D3D12Texture.hpp"
#include "..\D3D12API.hpp"
#include "..\DXR\DXRBase.h"
#include "..\D3D12ShaderManager.hpp"

#include "../../External/IMGUI/imgui.h"
#include "../../External/IMGUI/imgui_impl_win32.h"
#include "../../External/IMGUI/imgui_impl_dx12.h"
#include "../External/LodePNG/lodepng.h"

#include "../External/D3DX12/d3dx12.h"

#include <d3d12.h>
#include <comdef.h>
#include <iostream>
#include <Filesystem>

D3D12RaytracerRenderer::D3D12RaytracerRenderer(D3D12API* d3d12) : D3D12Renderer(d3d12) {
	m_dxrBase = MY_NEW DXRBase(m_d3d12);
}

D3D12RaytracerRenderer::~D3D12RaytracerRenderer() {
	if (m_dxrBase) {
		delete m_dxrBase;
	}

	for (auto& e : m_commandLists) {
		if (e) {
			e->Release();
		}
	}
	for (auto& e : m_commandAllocators) {
		if (e) {
			e->Release();
		}
	}
	for (auto& e : m_outputTextures) {
		if (e) {
			e->Release();
		}
	}
}

bool D3D12RaytracerRenderer::Initialize() {
	if (!m_dxrBase->Initialize()) {
		return false;
	}

	if (!InitializeCommandInterfaces()) {
		return false;
	}

	return true;
}

bool D3D12RaytracerRenderer::InitializeCommandInterfaces() {
	HRESULT hr;

	for (size_t i = 0; i < NUM_GPU_BUFFERS; i++) {
		hr = m_d3d12->GetDevice()->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			IID_PPV_ARGS(&m_commandAllocators[i]));
		if (FAILED(hr)) {
			return false;
		}

		//Create command list.
		hr = m_d3d12->GetDevice()->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_DIRECT,
			m_commandAllocators[i],
			nullptr,
			IID_PPV_ARGS(&m_commandLists[i]));
		if (FAILED(hr)) {
			return false;
		}

		//Command lists are created in the recording state. Since there is nothing to
		//record right now and the main loop expects it to be closed, we close it.
		m_commandLists[i]->Close();
	}

	return true;
}

bool D3D12RaytracerRenderer::InitializeOutputTextures(D3D12Window* window) {
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

	for (int i = 0; i < NUM_GPU_BUFFERS; i++) {
		if (m_outputTextures[i]) {
			m_outputTextures[i]->Release();
		}
		hr = m_d3d12->GetDevice()->CreateCommittedResource(&D3D12Utils::sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &clearValue, IID_PPV_ARGS(&m_outputTextures[i]));
		m_outputTextures[i]->SetName((L"D3D12RaytracerRenderer Output Texture #" + std::to_wstring(i)).c_str());
		if (FAILED(hr)) {
			return false;
		}
	}

	m_dxrBase->SetOutputResources(m_outputTextures, m_outputDim);

	return true;
}

void D3D12RaytracerRenderer::ResetCommandListAndAllocator(int index) {
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

void D3D12RaytracerRenderer::Submit(const SubmissionItem& item, Camera* camera, unsigned char layer) {
	m_renderItems.emplace_back(item);
}

void D3D12RaytracerRenderer::ClearSubmissions() {
	m_renderItems.clear();
}

void D3D12RaytracerRenderer::Frame(Window* window, Camera* camera) {
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
	InitializeOutputTextures(static_cast<D3D12Window*>(window));

	ResetCommandListAndAllocator(bufferIndex);
	if (m_renderItems.size() == 0) {
		return;
	}
	ID3D12GraphicsCommandList4* cmdlist = m_commandLists[bufferIndex];

	//D3D12_GPU_VIRTUAL_ADDRESS rtAddr = static_cast<D3D12Window*>(window)->GetCurrentRenderTargetGPUDescriptorHandle().ptr;	
	//D3D12Texture* tex = static_cast<D3D12Texture*>(m_OpaqueItems[0].blueprint->textures[0]);

	D3D12ShaderManager* sm = m_d3d12->GetShaderManager_D3D12();
	m_dxrBase->UpdateSceneData(static_cast<D3D12Camera*>(camera), m_lights);
	m_dxrBase->UpdateAccelerationStructures(m_renderItems, cmdlist);
	m_dxrBase->UpdateDescriptorHeap(cmdlist);
	m_dxrBase->UpdateShaderTable(sm);
	m_dxrBase->Dispatch(cmdlist, sm);

	///Copy final output to window resource
	ID3D12Resource* windowOutput = static_cast<D3D12Window*>(window)->GetCurrentRenderTargetResource();

	D3D12Utils::SetResourceTransitionBarrier(cmdlist, m_outputTextures[bufferIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, 0);
	D3D12Utils::SetResourceTransitionBarrier(cmdlist, windowOutput, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST, 0);
	cmdlist->CopyResource(static_cast<D3D12Window*>(window)->GetCurrentRenderTargetResource(), m_outputTextures[bufferIndex]);
	D3D12Utils::SetResourceTransitionBarrier(cmdlist, windowOutput, D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT, 0);
	D3D12Utils::SetResourceTransitionBarrier(cmdlist, m_outputTextures[bufferIndex], D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0);
}

void D3D12RaytracerRenderer::Present(Window* window, GUI* gui) {
	D3D12Window* w = static_cast<D3D12Window*>(window);
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
	ID3D12GraphicsCommandList4* cmdlist = m_commandLists[bufferIndex];

	/////////////////
	//Render GUI
	if (gui) {
		ID3D12Resource* windowOutput = w->GetCurrentRenderTargetResource();
		D3D12Utils::SetResourceTransitionBarrier(cmdlist, windowOutput, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET, 0);
		w->SetRenderTarget(cmdlist);
		ID3D12DescriptorHeap* guiDescHeap = w->GetGUIDescriptorHeap();
		cmdlist->SetDescriptorHeaps(1, &guiDescHeap);

		// Start the Dear ImGui frame
		ImGui_ImplDX12_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();

		gui->RenderGUI();

		ImGui::Render();
		ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), cmdlist);
		D3D12Utils::SetResourceTransitionBarrier(cmdlist, windowOutput, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT, 0);
	}
	/////////////////

	cmdlist->Close();
	ID3D12CommandList* listsToExecute[1] = { cmdlist };
	m_d3d12->GetDirectCommandQueue()->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	DXGI_PRESENT_PARAMETERS pp = {};

	HRESULT hr = static_cast<D3D12Window*>(window)->GetSwapChain()->Present1(0, 0, &pp);
	if (FAILED(hr)) {
		hr = m_d3d12->GetDevice()->GetDeviceRemovedReason();
		_com_error err2(hr);
		std::cout << "Device Status: " << err2.ErrorMessage() << std::endl;
		//TODO: Log Error
		//Return false
	}

	//Lastly
	m_d3d12->IncGPUBufferIndex();
}

void D3D12RaytracerRenderer::ClearFrame() {

}

void D3D12RaytracerRenderer::SetLightSources(const std::vector<LightSource>& lights) {
	m_lights = lights;
}

void D3D12RaytracerRenderer::SetSetting(const std::string& setting, float value) {
	if (setting == "anyhit") {
		m_dxrBase->SetAllowAnyHitShader(value > 0);
	}
}

void D3D12RaytracerRenderer::SetSetting(const std::string& setting, void* value) {

}

float D3D12RaytracerRenderer::GetSetting(const std::string& setting) {
	if (setting == "anyhit") {
		return 0;
	}
}

bool D3D12RaytracerRenderer::SaveLastFrame(const std::string& file) {
	//Wait for the current frame to finnish rendering.
	m_d3d12->WaitForGPU_ALL();

	//Get the previus buffer Index
	uint bufferIndex = m_d3d12->GetGPUBufferIndex();
	bufferIndex = (bufferIndex + NUM_GPU_BUFFERS - 1) % NUM_GPU_BUFFERS;

	ID3D12Resource* readBackRes;
	m_commandAllocators[bufferIndex]->Reset();
	ID3D12GraphicsCommandList4* cmdlist = m_commandLists[bufferIndex];
	cmdlist->Reset(m_commandAllocators[bufferIndex], nullptr);

	const UINT64 bufferSize = GetRequiredIntermediateSize(m_outputTextures[bufferIndex], 0, 1);

	D3D12_HEAP_PROPERTIES prop;
	prop.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	prop.CreationNodeMask = 1;
	prop.VisibleNodeMask = 1;
	prop.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	prop.Type = D3D12_HEAP_TYPE_READBACK;

	D3D12_RESOURCE_DESC resDesc;
	resDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resDesc.Format = DXGI_FORMAT_UNKNOWN;
	resDesc.Alignment = 0;
	resDesc.Width = bufferSize;
	resDesc.Height = 1;
	resDesc.DepthOrArraySize = 1;
	resDesc.MipLevels = 1;
	resDesc.SampleDesc.Count = 1;
	resDesc.SampleDesc.Quality = 0;
	resDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
	resDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	HRESULT hr;
	hr = m_d3d12->GetDevice()->CreateCommittedResource(&prop, D3D12_HEAP_FLAG_NONE, &resDesc, D3D12_RESOURCE_STATE_COPY_DEST, nullptr, IID_PPV_ARGS(&readBackRes));

	D3D12_TEXTURE_COPY_LOCATION copyLocDest;
	D3D12_TEXTURE_COPY_LOCATION copyLocSource;

	copyLocDest.PlacedFootprint.Footprint.Depth = 1;
	copyLocDest.PlacedFootprint.Footprint.Width = m_outputDim.x;
	copyLocDest.PlacedFootprint.Footprint.Height = m_outputDim.y;
	copyLocDest.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	copyLocDest.PlacedFootprint.Footprint.RowPitch = m_outputDim.x * 4;
	copyLocDest.PlacedFootprint.Offset = 0;
	copyLocDest.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	copyLocDest.pResource = readBackRes;

	copyLocSource.pResource = m_outputTextures[bufferIndex];
	copyLocSource.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
	copyLocSource.SubresourceIndex = 0;


	D3D12Utils::SetResourceTransitionBarrier(cmdlist, m_outputTextures[bufferIndex], D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_COPY_SOURCE, 0);
	cmdlist->CopyTextureRegion(&copyLocDest, 0, 0, 0, &copyLocSource, nullptr);
	D3D12Utils::SetResourceTransitionBarrier(cmdlist, m_outputTextures[bufferIndex], D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, 0);

	hr = cmdlist->Close();
	if (FAILED(hr)) {
		int i = 0;
	}
	ID3D12CommandList* listsToExecute[1] = { cmdlist };
	m_d3d12->GetDirectCommandQueue()->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	m_d3d12->SignalFence();
	m_d3d12->WaitForGPU_ALL();

	std::vector<unsigned char> imageData;
	imageData.resize(bufferSize);

	void* mappedData;
	readBackRes->Map(0, nullptr, &mappedData);
	memcpy(imageData.data(), mappedData, bufferSize);
	readBackRes->Unmap(0, nullptr);

	std::filesystem::create_directories(std::filesystem::path(file).parent_path());
	unsigned error = lodepng::encode(file, imageData, m_outputDim.x, m_outputDim.y);
	//unsigned error = lodepng::encode(file, imageData, m_outputDim.x, m_outputDim.y);

	readBackRes->Release();

	return true;
}

#ifdef PERFORMANCE_TESTING
double* D3D12RaytracerRenderer::GetGPU_Timers(int& nValues, int& firstValue) {
	return m_dxrBase->GetGPU_Timers(nValues, firstValue);
}
#endif // PERFORMANCE_TESTING

