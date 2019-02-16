#include "Renderer.hpp"

Renderer * Renderer::MakeRenderer(RendererBackend backend)
{
	switch (backend)
	{
	case Renderer::RendererBackend::D3D11:
		break;
	case Renderer::RendererBackend::D3D12:
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
