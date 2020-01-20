#include "FullScreenPass.hpp"
#include "D3D12API.hpp"
#include "D3D12Window.hpp"

#include <string>
#include <fstream>

#include <d3d12.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

FullScreenPass::FullScreenPass()
{
}


FullScreenPass::~FullScreenPass()
{
	vs_blob->Release();
	ps_blob->Release();
	m_rootSignature->Release();
	m_pipelineState->Release();

}

bool FullScreenPass::Initialize(D3D12API* renderer)
{
	m_renderer = renderer;
	m_srv_cbv_uav_size = m_renderer->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	if (!InitializeShaders())
		return false;
	if (!InitializeRootSignature())
		return false;
	if (!InitializePSO())
		return false;

	return true;
}

void FullScreenPass::Record(ID3D12GraphicsCommandList3 * list, D3D12Window* window)
{
	UINT backBufferIndex = window->GetCurrentBackBufferIndex();

	list->SetPipelineState(m_pipelineState);
	list->SetGraphicsRootSignature(m_rootSignature);
	list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	list->RSSetViewports(1, window->GetViewport());
	list->RSSetScissorRects(1, window->GetScissorRect());

	ID3D12DescriptorHeap* descHeap = m_renderer->GetDescriptorHeap();
	list->SetDescriptorHeaps(1, &descHeap);

	D3D12_GPU_DESCRIPTOR_HANDLE cdh = descHeap->GetGPUDescriptorHandleForHeapStart();
	cdh.ptr  += (m_renderer->NUM_DESCRIPTORS_IN_HEAP - 3 * NUM_SWAP_BUFFERS + 3 * 0 + 2) * m_srv_cbv_uav_size;
	   //.ptr  += (m_renderer->NUM_DESCRIPTORS_IN_HEAP - 3 * NUM_SWAP_BUFFERS + 3 * 0 + 1) * m_srv_cbv_uav_size;

	list->SetGraphicsRootDescriptorTable(0, cdh);

	list->DrawInstanced(3, 1, 0, 0);
}

bool FullScreenPass::InitializeShaders()
{
	ID3DBlob* blob_err;

	HRESULT hr;

	std::string shaderPath = "../assets/D3D12/VertexShader_FS.hlsl";
	std::ifstream shaderFile(shaderPath);

	std::string shaderText;
	std::string completeShaderText("");
	std::string entry = "main";
	std::string model = "vs_5_1";

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
		&vs_blob,
		&blob_err
	);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)blob_err->GetBufferPointer());
		return false;
	}

	shaderFile.close();
	//PIXEL

	shaderPath = "../assets/D3D12/PixelShader_FS.hlsl";
	shaderFile = std::ifstream(shaderPath);

	entry = "main";
	model = "ps_5_1";

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
		&ps_blob,
		&blob_err
	);
	if (FAILED(hr))
	{
		OutputDebugStringA((char*)blob_err->GetBufferPointer());
		return false;
	}

	return true;
}


bool FullScreenPass::InitializeRootSignature()
{

	HRESULT hr;
	D3D12_DESCRIPTOR_RANGE dr[1] = {};
	D3D12_ROOT_DESCRIPTOR_TABLE rootDescTable = {};

	D3D12_ROOT_SIGNATURE_DESC rootSigDesc = {};
	D3D12_ROOT_PARAMETER rootParams[1] = {};
	ID3DBlob* sBlob;

	dr[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	dr[0].NumDescriptors = 1;
	dr[0].BaseShaderRegister = 0;
	dr[0].RegisterSpace = 0;

	rootDescTable.NumDescriptorRanges = 1;
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

	m_rootSignature->SetName(L"FullScreen Tri Root");

	return true;
}


bool FullScreenPass::InitializePSO()
{
	HRESULT hr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};

	// Specify pipeline stages
	gpsd.pRootSignature = m_rootSignature;
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
	gpsd.VS.pShaderBytecode = vs_blob->GetBufferPointer();
	gpsd.VS.BytecodeLength =  vs_blob->GetBufferSize();

	gpsd.PS.pShaderBytecode = ps_blob->GetBufferPointer();
	gpsd.PS.BytecodeLength =  ps_blob->GetBufferSize();

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
	gpsd.DepthStencilState.DepthEnable = false;
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

	hr = m_renderer->GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_pipelineState));
	if (FAILED(hr))
	{
		return false;
	}

	m_pipelineState->SetName(L"FullScreen tri PSO");



	return true;
}