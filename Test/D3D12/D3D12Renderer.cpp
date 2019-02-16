#include "D3D12Renderer.hpp"

D3D12Renderer::D3D12Renderer()
{
}

Camera * D3D12Renderer::MakeCamera()
{
	return nullptr;
}

Window * D3D12Renderer::MakeWindow()
{
	return nullptr;
}

Texture * D3D12Renderer::MakeTexture()
{
	return nullptr;
}

Mesh * D3D12Renderer::MakeMesh()
{
	return nullptr;
}

Material * D3D12Renderer::MakeMaterial()
{
	return nullptr;
}

RenderState * D3D12Renderer::MakeRenderState()
{
	return nullptr;
}

Technique * D3D12Renderer::MakeTechnique(Material *, RenderState *)
{
	return nullptr;
}

void D3D12Renderer::Submit(SubmissionItem item)
{
}

void D3D12Renderer::ClearSubmissions()
{
}

void D3D12Renderer::Frame()
{
}

void D3D12Renderer::Present()
{
}

void D3D12Renderer::ClearFrame()
{
}
