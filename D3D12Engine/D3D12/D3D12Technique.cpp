#include "D3D12Technique.hpp"
#include "D3D12Material.hpp"
#include "D3D12RenderState.hpp"
#include "D3D12ShaderManager.hpp"
#include "D3D12Renderer.hpp"
#include <d3d12.h>
#include <comdef.h>

D3D12Technique::D3D12Technique(D3D12Renderer * renderer, unsigned short id) : m_renderer(renderer), m_id(id)
{

}

bool D3D12Technique::Initialize(D3D12RenderState * rs, ShaderProgram * sp, D3D12ShaderManager* sm)
{
	HRESULT hr;
	D3D12_INPUT_ELEMENT_DESC* ied;

	D3D12_INPUT_LAYOUT_DESC inputLayoutDesc = {};
	D3D12_GRAPHICS_PIPELINE_STATE_DESC gpsd = {};
	bool hasPos = false, hasNorm = false, hasUV = false, hasNMap = false;
	int n_inputs = 0;

	std::string defines = sm->GetVertexDefines(sp->VS.handle);
	if (/*defines.find("POSITION") != defines.npos*/true)
	{
		if (!hasPos)
			n_inputs++;

		hasPos = true;
	}
	if (defines.find("NORMAL") != defines.npos)
	{
		if (!hasNorm)
			n_inputs++;

		hasNorm = true;
	}
	if (defines.find("TEXTCOORD") != defines.npos)
	{
		if (!hasUV)
			n_inputs++;

		hasUV = true;
	}
	if (defines.find("NMAP") != defines.npos)
	{
		if (!hasNMap)
			n_inputs+=2;

		hasNMap = true;
	}

	ied = new D3D12_INPUT_ELEMENT_DESC[n_inputs];
	int idx = 0;
	if (hasPos)
	{
		ied[idx++] = { "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	}
	if (hasNorm)
	{
		ied[idx++] = { "NORM", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	}
	if (hasUV)
	{
		ied[idx++] = { "UV", 0, DXGI_FORMAT_R32G32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	}
	if (hasNMap)
	{
		ied[idx++] = { "TAN", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
		ied[idx++] = { "BI", 0, DXGI_FORMAT_R32G32B32_FLOAT, 3, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 };
	}
	inputLayoutDesc.pInputElementDescs = ied;
	//inputLayoutDesc.NumElements = ARRAYSIZE(ied);
	inputLayoutDesc.NumElements = n_inputs;

	// Specify pipeline stages
	gpsd.pRootSignature = m_renderer->GetRootSignature();
	gpsd.InputLayout = inputLayoutDesc;
	gpsd.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	gpsd.VS.pShaderBytecode = sm->GetShaderBlob(sp->VS)->GetBufferPointer();
	gpsd.VS.BytecodeLength = sm->GetShaderBlob(sp->VS)->GetBufferSize();
	if (sp->GS.type != ShaderType::UNKNOWN) {
		gpsd.GS.pShaderBytecode = sm->GetShaderBlob(sp->GS)->GetBufferPointer();
		gpsd.GS.BytecodeLength = sm->GetShaderBlob(sp->GS)->GetBufferSize();
	}
	gpsd.PS.pShaderBytecode = sm->GetShaderBlob(sp->FS)->GetBufferPointer();
	gpsd.PS.BytecodeLength = sm->GetShaderBlob(sp->FS)->GetBufferSize();

	// Specify render target and depthstencil usage
	gpsd.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	gpsd.NumRenderTargets = 1;

	gpsd.SampleDesc.Count = 1;
	gpsd.SampleMask = UINT_MAX;
	
	// Specify rasterizer behaviour
	gpsd.RasterizerState.FillMode = (rs->GetWireframe() ? D3D12_FILL_MODE_WIREFRAME : D3D12_FILL_MODE_SOLID);
	gpsd.RasterizerState.CullMode = static_cast<D3D12_CULL_MODE>(rs->GetFaceCulling());
	//gpsd.RasterizerState.DepthClipEnable = true;

	//dsd.dep
	gpsd.DepthStencilState.DepthEnable = rs->GetIsUsingDepthBuffer();
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
		_com_error err2(hr);
		std::cout << err2.ErrorMessage() << std::endl;
		return false;
	}

	std::wstring name = L"Technique #" + std::wstring(std::to_wstring(2));
	m_pipelineState->SetName(LPCWSTR(name.c_str()));

	delete[] ied;

	return true;
}

D3D12Technique::~D3D12Technique()
{
	if (m_pipelineState)
	{
		m_pipelineState->Release();
	}
}

bool D3D12Technique::Enable()
{
	//mRenderer->GetCommandList()->SetPipelineState(mPipelineState);

	return true;
}

unsigned short D3D12Technique::GetID() const
{
	return m_id;
}

ID3D12PipelineState * D3D12Technique::GetPipelineState()
{
	return m_pipelineState;
}
