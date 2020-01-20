#include "RenderAPI.hpp"
#include "D3D12/D3D12API.hpp"

RenderAPI::~RenderAPI()
{
}

RenderAPI* RenderAPI::MakeRenderer(RenderBackendAPI backend)
{
	switch (backend)
	{
	case RenderAPI::RenderBackendAPI::D3D11:
		break;
	case RenderAPI::RenderBackendAPI::D3D12:
		return new D3D12API;
		break;
	case RenderAPI::RenderBackendAPI::Vulcan:
		break;
	case RenderAPI::RenderBackendAPI::OpenGL:
		break;
	default:
		break;
	}

	return nullptr;
}

RenderAPI::RenderAPI()
{

}
