#include "stdafx.h"

#include "RenderAPI.hpp"
#include "D3D12/D3D12API.hpp"

RenderAPI::~RenderAPI()
{
}

RenderAPI* RenderAPI::MakeAPI(RenderBackendAPI backend)
{
	switch (backend)
	{
	case RenderAPI::RenderBackendAPI::D3D11:
		break;
	case RenderAPI::RenderBackendAPI::D3D12:
		return MY_NEW D3D12API;
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
