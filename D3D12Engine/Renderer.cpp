#include "Renderer.hpp"
#include "D3D12/D3D12Renderer.hpp"

Renderer::~Renderer()
{
}

Renderer * Renderer::MakeRenderer(RendererBackend backend)
{
	switch (backend)
	{
	case Renderer::RendererBackend::D3D11:
		break;
	case Renderer::RendererBackend::D3D12:
		return new D3D12Renderer;
		break;
	case Renderer::RendererBackend::Vulcan:
		break;
	case Renderer::RendererBackend::OpenGL:
		break;
	default:
		break;
	}

	return nullptr;
}

Renderer::Renderer()
{
}
