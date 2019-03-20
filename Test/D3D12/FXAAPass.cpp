#include "FXAAPass.hpp"
#include "D3D12Renderer.hpp"
#include <string>
#include <fstream>
#include <comdef.h>

#include "D3D12Window.hpp"
#include <d3d12.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")


FXAAPass::FXAAPass()
{
}


FXAAPass::~FXAAPass()
{
}

bool FXAAPass::Initialize(D3D12Renderer * renderer)
{
	m_renderer = renderer;

	if (!InitializeShaders())
		return false;
	if (!InitializeRootSignature())
		return false;
	if (!InitializePSO())
		return false;
	if (!InitializeResources())
		return false;
	if (!InitializeCommandInterfaces())
		return false;

	HRESULT hr;
	hr = m_renderer->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
	if (FAILED(hr))
	{
		return false;
	}

	m_FenceValue = 1;
	//Create an event handle to use for GPU synchronization.
	m_EventHandle = CreateEvent(0, false, false, 0);

	return true;
}

void FXAAPass::ApplyFXAA(D3D12Window * window)
{
	UINT backbufferIndex = window->GetCurrentBackBufferIndex();

	m_commandAllocator[backbufferIndex]->Reset();
	m_commandList[backbufferIndex]->Reset(m_commandAllocator[backbufferIndex], m_pipelineState);

	D3D12_RESOURCE_BARRIER barrierDesc = {};
	//barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//barrierDesc.Transition.pResource = window->GetCurrentRenderTargetResource();
	//barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	//barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	//barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;

	//m_commandList[backbufferIndex]->ResourceBarrier(1,&barrierDesc);

	m_commandList[backbufferIndex]->SetComputeRootSignature(m_rootSignature);
	ID3D12DescriptorHeap* tempDescriptorHeap = m_renderer->GetDescriptorHeap();
	m_commandList[backbufferIndex]->SetDescriptorHeaps(1, &tempDescriptorHeap);

	D3D12_GPU_DESCRIPTOR_HANDLE descHndGPU = m_renderer->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	descHndGPU.ptr += (m_renderer->NUM_DESCRIPTORS_IN_HEAP - 3 * NUM_SWAP_BUFFERS + 3 * backbufferIndex + 1) * m_srv_cbv_uav_size;

	m_commandList[backbufferIndex]->SetComputeRootDescriptorTable(0, descHndGPU);

	m_commandList[backbufferIndex]->Dispatch(1,1,1);

	//barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	//barrierDesc.Transition.pResource = window->GetCurrentRenderTargetResource();
	//barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	//barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
	//barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	//m_commandList[backbufferIndex]->ResourceBarrier(1, &barrierDesc);

	m_commandList[backbufferIndex]->Close();

	ID3D12CommandList* commandList = m_commandList[backbufferIndex];
	m_commandQueue->ExecuteCommandLists(1, &commandList);

	// Tell last frame to signal when done
	const UINT64 fence = m_FenceValue;
	m_commandQueue->Signal(m_Fence, fence);
	m_FenceValue++;

	//Wait until command queue is done.
	if (m_Fence->GetCompletedValue() < fence)
	{
		m_Fence->SetEventOnCompletion(fence, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}

}

bool FXAAPass::InitializeShaders()
{
	ID3DBlob* blob_err;

	HRESULT hr;

	std::string shaderPath = "../assets/D3D12/ComputeShader_FXAA.hlsl";
	std::ifstream shaderFile(shaderPath);

	std::string shaderText;
	std::string completeShaderText("");
	std::string entry = "main";
	std::string model = "cs_5_1";

	// open the file and read it to a string "shaderText"
	if (shaderFile.is_open())
	{
		shaderText = std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
		shaderFile.close();
	}
	else
	{
		return false;
	}

	// Create the complete shader text (defines + file data)
	completeShaderText = shaderText;

	hr = D3DCompile(
		completeShaderText.c_str(),
		completeShaderText.size(),
		nullptr,
		nullptr,
		nullptr,
		entry.c_str(),
		model.c_str(),
		0U,
		0U,
		&m_cs_blob,
		&blob_err
	);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)blob_err->GetBufferPointer());
		return false;
	}

	shaderFile.close();

	return true;
}

bool FXAAPass::InitializeRootSignature()
{
	HRESULT hr;
	D3D12_DESCRIPTOR_RANGE dr[2] = {};
	D3D12_ROOT_DESCRIPTOR_TABLE rootDescTable = {};

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	D3D12_ROOT_PARAMETER rootParams[1] = {};
	ID3DBlob* sBlob;

	/*
		UA UAV
	*/
	dr[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	dr[0].NumDescriptors = 1;
	dr[0].BaseShaderRegister = 0;
	dr[0].RegisterSpace = 0;

	dr[1].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	dr[1].NumDescriptors = 1;
	dr[1].BaseShaderRegister = 0;
	dr[1].RegisterSpace = 0;

	rootDescTable.NumDescriptorRanges = 2;
	rootDescTable.pDescriptorRanges = dr;

	// Root constants
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable = rootDescTable;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootSigDesc.NumParameters = sizeof(rootParams) / sizeof(D3D12_ROOT_PARAMETER);
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 0;
	rootSigDesc.pStaticSamplers = nullptr;

	hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&sBlob,
		nullptr);
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_renderer->GetDevice()->CreateRootSignature(
		0,
		sBlob->GetBufferPointer(),
		sBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature));
	if (FAILED(hr))
	{
		return false;
	}

	m_rootSignature->SetName(L"FXAA Root");

	return true;
}

bool FXAAPass::InitializePSO()
{
	HRESULT hr;

	D3D12_COMPUTE_PIPELINE_STATE_DESC gpsd = {};

	// Specify pipeline stages
	gpsd.pRootSignature = m_rootSignature;

	gpsd.CS.pShaderBytecode = m_cs_blob->GetBufferPointer();
	gpsd.CS.BytecodeLength = m_cs_blob->GetBufferSize();

	hr = m_renderer->GetDevice()->CreateComputePipelineState(&gpsd, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(hr))
	{
		return false;
	}

	m_pipelineState->SetName(L"FXAA compute PSO");

	return true;
}

bool FXAAPass::InitializeResources()
{
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	resourceDesc.Alignment = 0;
	resourceDesc.Width = 640;
	resourceDesc.Height = 640;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	HRESULT hr;
	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 1; //used when multi-gpu
	heapProperties.VisibleNodeMask = 1; //used when multi-gpu
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	m_srv_cbv_uav_size = m_renderer->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE cdh2 = m_renderer->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	cdh2.ptr += (m_renderer->NUM_DESCRIPTORS_IN_HEAP - 3 * NUM_SWAP_BUFFERS) * m_srv_cbv_uav_size;
	
	for (size_t i = 0; i < NUM_SWAP_BUFFERS; i++)
	{

		HRESULT hr = m_renderer->GetDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&m_UA_Resource[i])
		);
		if (FAILED(hr))
		{
			hr = m_renderer->GetDevice()->GetDeviceRemovedReason();
			_com_error err2(hr);
			std::cout << err2.ErrorMessage() << std::endl;

			return false;
		}
		std::wstring name = L"FXAA UA #" + std::to_wstring(i);
		m_UA_Resource[i]->SetName(name.c_str());

		////// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = resourceDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = resourceDesc.Format;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Texture2D.MipSlice = 0;
		uavDesc.Texture2D.PlaneSlice = 0;

		m_renderer->GetDevice()->CreateShaderResourceView(m_UA_Resource[i], &srvDesc, cdh2);
		std::cout << "FXAA SRV: " << cdh2.ptr << std::endl;

		cdh2.ptr += m_srv_cbv_uav_size;
		m_renderer->GetDevice()->CreateUnorderedAccessView(m_UA_Resource[i], nullptr, &uavDesc, cdh2);
		std::cout << "FXAA UAV: " << cdh2.ptr << std::endl;

		cdh2.ptr += 2 * m_srv_cbv_uav_size;
		//Rendertarget
	}


	return true;
}

bool FXAAPass::InitializeCommandInterfaces()
{
	HRESULT hr;

	// CommandList & Allocator 
	for (size_t backbufferIndex = 0; backbufferIndex < NUM_SWAP_BUFFERS; backbufferIndex++)
	{

		hr = m_renderer->GetDevice()->CreateCommandAllocator(
			D3D12_COMMAND_LIST_TYPE_COMPUTE,
			IID_PPV_ARGS(&m_commandAllocator[backbufferIndex]));
		if (FAILED(hr))
		{
			return false;
		}

		//Create command list.
		hr = m_renderer->GetDevice()->CreateCommandList(
			0,
			D3D12_COMMAND_LIST_TYPE_COMPUTE,
			m_commandAllocator[backbufferIndex],
			nullptr,
			IID_PPV_ARGS(&m_commandList[backbufferIndex]));
		if (FAILED(hr))
		{
			return false;
		}

		//Command lists are created in the recording state. Since there is nothing to
		//record right now and the main loop expects it to be closed, we close it.
		m_commandList[backbufferIndex]->Close();

	}


	// CommandQueue

	
	//Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	cqd.Type = D3D12_COMMAND_LIST_TYPE_COMPUTE;
	hr = m_renderer->GetDevice()->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_commandQueue));
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}
