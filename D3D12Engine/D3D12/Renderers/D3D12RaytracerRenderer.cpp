#include "D3D12RaytracerRenderer.h"
#include "..\DXR\Temp\DXRBase.h"

#include <d3d12.h>

D3D12RaytracerRenderer::D3D12RaytracerRenderer(D3D12API* d3d12) : D3D12Renderer(d3d12)
{
	m_dxrBase = new DXRBase(m_d3d12);
}

D3D12RaytracerRenderer::~D3D12RaytracerRenderer()
{
	delete m_dxrBase;
}

bool D3D12RaytracerRenderer::Initialize()
{
	if (m_dxrBase->Initialize()) {
		return false;
	}
	
	return true;
}

void D3D12RaytracerRenderer::Submit(SubmissionItem item, Camera* camera, unsigned char layer)
{

}

void D3D12RaytracerRenderer::ClearSubmissions()
{

}

void D3D12RaytracerRenderer::Frame(Window* window, Camera* camera)
{
	ID3D12GraphicsCommandList4* cmdList;
}

void D3D12RaytracerRenderer::Present(Window* window)
{

}

void D3D12RaytracerRenderer::ClearFrame()
{

}
