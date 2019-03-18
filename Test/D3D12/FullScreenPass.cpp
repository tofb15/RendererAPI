#include "FullScreenPass.hpp"
#include <string>
#include <fstream>

#include <d3dcompiler.h>
#pragma comment(lib, "d3dcompiler.lib")

FullScreenPass::FullScreenPass()
{
}


FullScreenPass::~FullScreenPass()
{
}

bool FullScreenPass::Initialize()
{
	return false;
}

void FullScreenPass::Record(ID3D12GraphicsCommandList3 * list)
{
}

bool FullScreenPass::InitializeShaders()
{
	ID3DBlob* blob_err;

	HRESULT hr;

	std::string shaderPath = "../assets/D3D12/PixelShader_FS.hlsl";
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

	shaderPath = "../assets/D3D12/VertexShader_FS.hlsl";
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

bool FullScreenPass::InitializePSO()
{
	HRESULT hr;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};

	//// Specify pipeline stages
	//gpsd.pRootSignature = m_renderer->GetRootSignature();
	//gpsd.InputLayout = inputLayoutDesc;
	//gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	//gpsd.VS.pShaderBytecode = sm->GetShaderBlob(sp->VS)->GetBufferPointer();
	//gpsd.VS.BytecodeLength = sm->GetShaderBlob(sp->VS)->GetBufferSize();
	//if (sp->GS.type != ShaderType::UNKNOWN) {
	//	gpsd.GS.pShaderBytecode = sm->GetShaderBlob(sp->GS)->GetBufferPointer();
	//	gpsd.GS.BytecodeLength = sm->GetShaderBlob(sp->GS)->GetBufferSize();
	//}
	//gpsd.PS.pShaderBytecode = sm->GetShaderBlob(sp->FS)->GetBufferPointer();
	//gpsd.PS.BytecodeLength = sm->GetShaderBlob(sp->FS)->GetBufferSize();

	//// Specify render target and depthstencil usage
	//gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	//gpsd.NumRenderTargets = 1;

	//gpsd.SampleDesc.Count = 1;
	//gpsd.SampleMask = UINT_MAX;

	//// Specify rasterizer behaviour
	//gpsd.RasterizerState.FillMode = (rs->GetWireframe() ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID);
	//gpsd.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(rs->GetFaceCulling());
	////gpsd.RasterizerState.DepthClipEnable = true;

	////dsd.dep
	//gpsd.DepthStencilState.DepthEnable = rs->GetIsUsingDepthBuffer();
	//gpsd.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	//gpsd.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
	//gpsd.DepthStencilState.StencilEnable = false;
	//gpsd.DepthStencilState.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
	//gpsd.DepthStencilState.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
	//const D3D12_DEPTH_STENCILOP_DESC dsopd =
	//{
	//	D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS
	//};
	//gpsd.DepthStencilState.FrontFace = dsopd;
	//gpsd.DepthStencilState.BackFace = dsopd;

	//gpsd.DSVFormat = DXGI_FORMAT_D32_FLOAT;

	////gpsd.DepthStencilState.DepthEnable = true;


	//// Specify blend descriptions
	//D3D12_RENDER_TARGET_BLEND_DESC defaultRTdesc =
	//{
	//	false, false,
	//	D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
	//	D3D12_BLEND_ONE, D3D12_BLEND_ZERO, D3D12_BLEND_OP_ADD,
	//	D3D12_LOGIC_OP_NOOP, D3D12_COLOR_WRITE_ENABLE_ALL
	//};

	//for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; i++)
	//	gpsd.BlendState.RenderTarget[i] = defaultRTdesc;

	//hr = m_renderer->GetDevice()->CreateGraphicsPipelineState(&gpsd, IID_PPV_ARGS(&m_pipelineState));
	//if (FAILED(hr))
	//{
	//	return false;
	//}

	//delete[] ied;

	return true;
}

bool FullScreenPass::InitializeRootSignature()
{
	HRESULT hr;
	D3D12_ROOT_PARAMETER rootParams[1];

	// Root constants
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[0].Constants.Num32BitValues = 16 + 4;		// 1 * float4x4 + 1 * int
	rootParams[0].Constants.RegisterSpace = 0;
	rootParams[0].Constants.ShaderRegister = 0;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootSigDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
	rootSigDesc.NumParameters = sizeof(rootParams) / sizeof(D3D12_ROOT_PARAMETER);
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = samplerDesc;

	hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&sBlob,
		nullptr);
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_device->CreateRootSignature(
		0,
		sBlob->GetBufferPointer(),
		sBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature));
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}