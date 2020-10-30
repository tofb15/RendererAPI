#include "D3D12ParticleSystem.hpp"
#include "Renderers/D3D12ForwardRenderer.h"

#include <d3dcompiler.h>
#include <string>
#include <fstream>
#include <comdef.h>
#include <math.h>

#pragma comment(lib, "d3dcompiler.lib")
#include "D3D12API.hpp"
#include "D3D12Camera.hpp"

D3D12ParticleSystem::D3D12ParticleSystem(D3D12API* renderer, short id) : ParticleSystem()
{
	m_renderer = renderer;
	m_id = id;
}

D3D12ParticleSystem::~D3D12ParticleSystem()
{
	//Wait if needed
	if (m_Fence[m_bufferIndex]->GetCompletedValue() < m_FenceValue[m_bufferIndex] - 1)
	{
		m_Fence[m_bufferIndex]->SetEventOnCompletion(m_FenceValue[m_bufferIndex] - 1, m_EventHandle[m_bufferIndex]);
		WaitForSingleObject(m_EventHandle[m_bufferIndex], INFINITE);
	}

	m_cs_blob->Release();
	m_rootSignature_CS->Release();
	m_pipelineState_CS->Release();
	m_UA_Resource[NUM_SWAP_BUFFERS]->Release();

	m_vs_blob->Release();
	m_gs_blob->Release();
	m_ps_blob->Release();
	m_rootSignature_render->Release();
	m_pipelineState_render->Release();

	for (size_t i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_commandList[i]->Release();
		m_commandAllocator[i]->Release();
	}
	m_commandQueue->Release();
}

bool D3D12ParticleSystem::Initialize()
{
	if (!InitializeShaders())
		return false;
	if (!InitializeRootSignature_CS())
		return false;
	if (!InitializePSO_CS())
		return false;
	if (!InitializeRootSignature_Render())
		return false;
	if (!InitializePSO_Render())
		return false;
	if (!InitializeResources())
		return false;
	if (!InitializeCommandInterfaces())
		return false;

#ifdef 	Particle_Single_Fence
	HRESULT hr;
	hr = m_renderer->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
	if (FAILED(hr))
	{
		return false;
	}

	m_FenceValue = 1;
	//Create an event handle to use for GPU synchronization.
	m_EventHandle = CreateEvent(0, false, false, 0);
#else

	HRESULT hr;
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		hr = m_renderer->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence[i]));
		if (FAILED(hr))
		{
			return false;
		}

		m_FenceValue[i] = 1;
		//Create an event handle to use for GPU synchronization.
		m_EventHandle[i] = CreateEvent(0, false, false, 0);
	}
#endif
	return true;
}

void D3D12ParticleSystem::Update(float dt)
{
	m_time += dt;

	m_commandAllocator[m_bufferIndex]->Reset();
	m_commandList[m_bufferIndex]->Reset(m_commandAllocator[m_bufferIndex], m_pipelineState_CS);

	m_commandList[m_bufferIndex]->SetComputeRootSignature(m_rootSignature_CS);
	m_commandList[m_bufferIndex]->SetComputeRoot32BitConstants(1, 1, &m_time, 0);

	ID3D12DescriptorHeap* tempDescriptorHeap = m_renderer->GetDescriptorHeap();
	m_commandList[m_bufferIndex]->SetDescriptorHeaps(1, &tempDescriptorHeap);

	D3D12_GPU_DESCRIPTOR_HANDLE descHndGPU = m_renderer->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
	descHndGPU.ptr += (m_renderer->NUM_DESCRIPTORS_IN_HEAP - NUM_SWAP_BUFFERS + m_bufferIndex) * m_srv_cbv_uav_size;
	m_commandList[m_bufferIndex]->SetComputeRootDescriptorTable(0, descHndGPU);

	m_commandList[m_bufferIndex]->Dispatch(NUM_PARTICLES / 1000, 1, 1);
	m_commandList[m_bufferIndex]->Close();

	ID3D12CommandList* commandList = m_commandList[m_bufferIndex];
	m_commandQueue->ExecuteCommandLists(1, &commandList);

	/*
		THIS SHOULD BE REMOVED
	*/

	static UINT64 nWaits = 0;

#ifdef Particle_Single_Fence
	// Tell last frame to signal when done
	const UINT64 fence = m_FenceValue;
	m_commandQueue->Signal(m_Fence, fence);
	m_FenceValue++;

	//Wait until command queue is done.
	if (m_Fence->GetCompletedValue() < fence)
	{
		nWaits++;
		std::string s = "Compute waits: " + std::to_string(nWaits) + "\n";
		printf(s.c_str());
		m_Fence->SetEventOnCompletion(fence, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
#else
	//Tell last update to signal when done
	const UINT64 fence = m_FenceValue[m_bufferIndex];
	m_commandQueue->Signal(m_Fence[m_bufferIndex], fence);
	m_FenceValue[m_bufferIndex]++;

	m_bufferIndex += 1;
	m_bufferIndex %= NUM_SWAP_BUFFERS;

	//Wait if needed
	if (m_Fence[m_bufferIndex]->GetCompletedValue() < m_FenceValue[m_bufferIndex] - 1)
	{
		nWaits++;
		std::string s = "Compute waits: " + std::to_string(nWaits) + "\n";
		printf(s.c_str());
		m_Fence[m_bufferIndex]->SetEventOnCompletion(m_FenceValue[m_bufferIndex] - 1, m_EventHandle[m_bufferIndex]);
		WaitForSingleObject(m_EventHandle[m_bufferIndex], INFINITE);
	}
#endif
}

void D3D12ParticleSystem::Render(D3D12ForwardRenderer* renderer, ID3D12GraphicsCommandList3 * list, D3D12Camera* camera)
{
#ifndef Particle_Single_Fence
	//Get Last done update
	int searchIndex = (m_bufferIndex - 1 + NUM_SWAP_BUFFERS) % NUM_SWAP_BUFFERS;
	for (size_t i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		if (m_Fence[searchIndex]->GetCompletedValue() == m_FenceValue[searchIndex] - 1)
		{
			break;//Found one that is done!
		}
		searchIndex += (NUM_SWAP_BUFFERS - 1);
		searchIndex %= (NUM_SWAP_BUFFERS);
	}
#endif // Particle_Single_Fence

	DirectX::XMFLOAT4X4 viewPersp = camera->GetViewPerspective();

	list->SetPipelineState(m_pipelineState_render);
	list->SetGraphicsRootSignature(m_rootSignature_render);
	list->IASetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY::D3D10_PRIMITIVE_TOPOLOGY_POINTLIST);

	ID3D12DescriptorHeap* tempDescriptorHeap = renderer->GetDescriptorHeap();
	list->SetDescriptorHeaps(1, &tempDescriptorHeap);

	D3D12_GPU_DESCRIPTOR_HANDLE descHndGPU = renderer->GetDescriptorHeap()->GetGPUDescriptorHandleForHeapStart();
#ifdef Particle_Single_Fence
	descHndGPU.ptr += (m_renderer->NUM_DESCRIPTORS_IN_HEAP - NUM_SWAP_BUFFERS + m_bufferIndex) * m_srv_cbv_uav_size;
#else
	descHndGPU.ptr += (renderer->NUM_DESCRIPTORS_IN_HEAP - NUM_SWAP_BUFFERS + searchIndex) * m_srv_cbv_uav_size;
#endif
	list->SetGraphicsRootDescriptorTable(0, descHndGPU);
	list->SetGraphicsRoot32BitConstants(1, 16, &viewPersp, 0);

	list->DrawInstanced(NUM_PARTICLES, 1, 0, 0);

}

bool D3D12ParticleSystem::CompileShader(ID3DBlob** shaderBlob, const char * name, Shader_Type type)
{
	ID3DBlob* blob_err;

	HRESULT hr;

	std::string shaderPath = "../assets/D3D12/" + std::string(name) + ".hlsl";
	std::ifstream shaderFile(shaderPath);

	std::string shaderText;
	std::string completeShaderText("");
	std::string entry = "main";
	std::string model;
	switch (type)
	{
	case D3D12ParticleSystem::VS:
		model = "vs_5_1";
		break;
	case D3D12ParticleSystem::GS:
		model = "gs_5_1";
		break;
	case D3D12ParticleSystem::PS:
		model = "ps_5_1";
		break;
	case D3D12ParticleSystem::CS:
		model = "cs_5_1";
		break;
	default:
		return false;
	}

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
		shaderBlob,
		&blob_err
	);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)blob_err->GetBufferPointer());
		return false;
	}

	return true;
}

bool D3D12ParticleSystem::InitializeRootSignature_Render()
{
	HRESULT hr;
	D3D12_DESCRIPTOR_RANGE dr[1] = {};
	D3D12_ROOT_DESCRIPTOR_TABLE rootDescTable = {};

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	D3D12_ROOT_PARAMETER rootParams[2] = {};
	ID3DBlob* sBlob;

	dr[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	dr[0].NumDescriptors = 1;
	dr[0].BaseShaderRegister = 0;
	dr[0].RegisterSpace = 0;

	rootDescTable.NumDescriptorRanges = 1;
	rootDescTable.pDescriptorRanges = dr;

	// Root constants
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable = rootDescTable;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[1].Constants.Num32BitValues = 16;
	rootParams[1].Constants.RegisterSpace = 0;
	rootParams[1].Constants.ShaderRegister = 0;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_GEOMETRY;

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
		IID_PPV_ARGS(&m_rootSignature_render));
	if (FAILED(hr))
	{
		return false;
	}

	m_rootSignature_render->SetName(L"Particle Root Render");

	return true;
}

bool D3D12ParticleSystem::InitializePSO_Render()
{
	HRESULT hr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};

	// Specify pipeline stages
	gpsd.pRootSignature = m_rootSignature_render;
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	
	gpsd.VS.pShaderBytecode = m_vs_blob->GetBufferPointer();
	gpsd.VS.BytecodeLength = m_vs_blob->GetBufferSize();

	gpsd.GS.pShaderBytecode = m_gs_blob->GetBufferPointer();
	gpsd.GS.BytecodeLength = m_gs_blob->GetBufferSize();

	gpsd.PS.pShaderBytecode = m_ps_blob->GetBufferPointer();
	gpsd.PS.BytecodeLength = m_ps_blob->GetBufferSize();

	// Specify render target and depthstencil usage
	gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsd.NumRenderTargets = 1;

	gpsd.SampleDesc.Count = 1;
	gpsd.SampleMask = UINT_MAX;

	// Specify rasterizer behaviour
	gpsd.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
	gpsd.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
	//gpsd.RasterizerState.DepthClipEnable = true;

	//dsd.dep
	gpsd.DepthStencilState.DepthEnable = true;
	gpsd.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	gpsd.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	gpsd.DepthStencilState.StencilEnable = false;
	gpsd.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	gpsd.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	const D3D12_DEPTH_STENCILOP_DESC dsopd =
	{
		D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
	};
	gpsd.DepthStencilState.FrontFace = dsopd;
	gpsd.DepthStencilState.BackFace = dsopd;

	gpsd.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	//gpsd.DepthStencilState.DepthEnable = true;


	// Specify blend descriptions
	D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc =
	{
		false, false,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
		D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL
	};

	for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
		gpsd.BlendState.RenderTarget[i] = defaultRTdesc;

	hr = m_renderer->GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_pipelineState_render));
	if (FAILED(hr))
	{
		return false;
	}

	m_pipelineState_render->SetName(L"Particle PSO Render");

	return true;
}

bool D3D12ParticleSystem::InitializeShaders()
{
	if (!CompileShader(&m_cs_blob, "ComputeShader_Particle", CS))
		return false;
	if (!CompileShader(&m_vs_blob, "VertexShader_Particle", VS))
		return false;
	if (!CompileShader(&m_gs_blob, "GeometryShader_Particle", GS))
		return false;
	if (!CompileShader(&m_ps_blob, "PixelShader_Particle", PS))
		return false;

	return true;
}

bool D3D12ParticleSystem::InitializeRootSignature_CS()
{
	HRESULT hr;
	D3D12_DESCRIPTOR_RANGE dr[1] = {};
	D3D12_ROOT_DESCRIPTOR_TABLE rootDescTable = {};

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	D3D12_ROOT_PARAMETER rootParams[2] = {};
	ID3DBlob* sBlob;

	dr[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
	dr[0].NumDescriptors = 1;
	dr[0].BaseShaderRegister = 0;
	dr[0].RegisterSpace = 0;

	rootDescTable.NumDescriptorRanges = 1;
	rootDescTable.pDescriptorRanges = dr;

	// Root constants
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[0].DescriptorTable = rootDescTable;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[1].Constants.Num32BitValues = 1;
	rootParams[1].Constants.RegisterSpace = 0;
	rootParams[1].Constants.ShaderRegister = 0;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

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
		IID_PPV_ARGS(&m_rootSignature_CS));
	if (FAILED(hr))
	{
		return false;
	}

	m_rootSignature_CS->SetName(L"Particle Root CS");

	return true;
}

bool D3D12ParticleSystem::InitializePSO_CS()
{
	HRESULT hr;

	D3D12_COMPUTE_PIPELINE_STATE_DESC gpsd = {};

	// Specify pipeline stages
	gpsd.pRootSignature = m_rootSignature_CS;

	gpsd.CS.pShaderBytecode = m_cs_blob->GetBufferPointer();
	gpsd.CS.BytecodeLength = m_cs_blob->GetBufferSize();

	hr = m_renderer->GetDevice()->CreateComputePipelineState(&gpsd, IID_PPV_ARGS(&m_pipelineState_CS));
	if (FAILED(hr))
	{
		return false;
	}

	m_pipelineState_CS->SetName(L"Particle compute PSO");

	return true;
}

bool D3D12ParticleSystem::InitializeResources()
{
	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	resourceDesc.Width = NUM_PARTICLES * sizeof(Float3) * 2;
	resourceDesc.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;
	
	resourceDesc.Height = 1;
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 1;
	resourceDesc.Format = DXGI_FORMAT_UNKNOWN;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 1; //used when multi-gpu
	heapProperties.VisibleNodeMask = 1; //used when multi-gpu
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

	m_srv_cbv_uav_size = m_renderer->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE cdh2 = m_renderer->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
	cdh2.ptr += (m_renderer->NUM_DESCRIPTORS_IN_HEAP - NUM_SWAP_BUFFERS) * m_srv_cbv_uav_size;

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
			if (FAILED(hr))
			{
				_com_error err2(hr);
				std::cout << err2.ErrorMessage() << std::endl;
			}
			return false;
		}
		std::wstring name = L"Particles UA #" + std::to_wstring(i);
		m_UA_Resource[i]->SetName(name.c_str());

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = DXGI_FORMAT_UNKNOWN;
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.NumElements = NUM_PARTICLES * 2;
		uavDesc.Buffer.StructureByteStride = sizeof(Float3);

		m_renderer->GetDevice()->CreateUnorderedAccessView(m_UA_Resource[i], nullptr, &uavDesc, cdh2);
		std::cout << "UAV created at: " << cdh2.ptr << std::endl;
		cdh2.ptr += m_srv_cbv_uav_size;
	}


	return true;
}

bool D3D12ParticleSystem::InitializeCommandInterfaces()
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
