#include "D3D12API.hpp"
#include <d3d12.h>
#include <dxgi1_6.h> //Only used for initialization of the device and swap chain.

#include "D3D12Window.hpp"
#include "D3D12Camera.hpp"
#include "D3D12Texture.hpp"
#include "D3D12Technique.hpp"
#include "D3D12Material.hpp"
#include "D3D12Mesh.hpp"
#include "D3D12Terrain.hpp"
#include "D3D12VertexBuffer.hpp"
#include "D3D12VertexBufferLoader.hpp"
#include "D3D12RenderState.hpp"
#include "D3D12ShaderManager.hpp"
#include "D3D12TextureLoader.hpp"

#include "Renderers/D3D12ForwardRenderer.h"
#include "Renderers/D3D12RaytracerRenderer.h"

#include <iostream>
#include <comdef.h>
#include <iostream>

#pragma comment(lib, "d3d12.lib")

#define MULTI_THREADED


/*Release a Interface that will not be used anymore*/
template<class Interface>
inline void SafeRelease(Interface **ppInterfaceToRelease)
{
	if (*ppInterfaceToRelease != NULL)
	{
		(*ppInterfaceToRelease)->Release();

		(*ppInterfaceToRelease) = NULL;
	}
}

D3D12API::D3D12API()
{

}
D3D12API::~D3D12API()
{
	m_textureLoader->Kill();	//Notify the other thread to stop.
	m_thread_texture.join();	//Wait for the other thread to stop.
	if (m_textureLoader)
		delete m_textureLoader;

	if (m_vertexBufferLoader)
		delete m_vertexBufferLoader;

	HRESULT hr = m_device->GetDeviceRemovedReason();
	if (FAILED(hr)) {
		_com_error err2(hr);
		std::cout << "Device Status: " << err2.ErrorMessage() << std::endl;
	}
	

	m_device->Release();

	int i;
}

bool D3D12API::Initialize()
{
	if (!InitializeDirect3DDevice())
	{
		printf("Error: Could not initialize device\n");
		return false;
	}

	m_textureLoader = new D3D12TextureLoader(this);
	if (!m_textureLoader->Initialize())
	{
		printf("Error: Could not initialize texture loader\n");
		return false;
	}

	//Start new Texture loading thread
	m_thread_texture = std::thread(&D3D12TextureLoader::DoWork, &*m_textureLoader);

	m_vertexBufferLoader = new D3D12VertexBufferLoader(this);
	if (!m_vertexBufferLoader->Initialize())
	{
		printf("Error: Could not initialize vertex buffer loader\n");
		return false;
	}

	return true;
}

Camera * D3D12API::MakeCamera()
{
	return new D3D12Camera();
}
Window * D3D12API::MakeWindow()
{
	return new D3D12Window(this);
}
Texture * D3D12API::MakeTexture()
{
	if (++m_texturesCreated == 0)
		return nullptr;

	return new D3D12Texture(this, m_texturesCreated);
}
Mesh * D3D12API::MakeMesh()
{
	if (++m_meshesCreated == 0)
		return nullptr;

	return new D3D12Mesh(this, m_meshesCreated);
}
Terrain * D3D12API::MakeTerrain()
{
	return new D3D12Terrain(this);
}
Material * D3D12API::MakeMaterial()
{
	return new D3D12Material;
}
RenderState * D3D12API::MakeRenderState()
{
	return new D3D12RenderState;
}
Technique * D3D12API::MakeTechnique(RenderState* rs, ShaderProgram* sp, ShaderManager* sm)
{
	if (++m_techniquesCreated == 0)
		return nullptr;

	D3D12Technique* tech = new D3D12Technique(this, m_techniquesCreated);
	if (!tech->Initialize(static_cast<D3D12RenderState*>(rs), sp, static_cast<D3D12ShaderManager*>(sm)))
	{
		delete tech;
		return nullptr;
	}
	//Next Frame Frame
	//int* closestTechnique_temp = new int[m_techniquesCreated];
	//for (size_t i = 0; i < m_techniquesCreated - 1; i++)
	//{
	//	closestTechnique_temp[i] = m_closestTechnique[i];
	//}
	//closestTechnique_temp[m_techniquesCreated - 1] = 0;
	//delete m_closestTechnique;
	//m_closestTechnique = closestTechnique_temp;

	////Last Frame
	//closestTechnique_temp = new int[m_techniquesCreated];
	//for (size_t i = 0; i < m_techniquesCreated - 1; i++)
	//{
	//	closestTechnique_temp[i] = m_closestTechnique_lastFrame[i];
	//}
	//closestTechnique_temp[m_techniquesCreated - 1] = 0;
	//delete m_closestTechnique_lastFrame;
	//m_closestTechnique_lastFrame = closestTechnique_temp;

	return tech;
}
ShaderManager * D3D12API::MakeShaderManager()
{
	D3D12ShaderManager* sm = new D3D12ShaderManager(this);
	return sm;
}
D3D12VertexBuffer * D3D12API::MakeVertexBuffer()
{
	return new D3D12VertexBuffer(this);
}



ID3D12Device5 * D3D12API::GetDevice() const
{
	return m_device;
}

D3D12TextureLoader * D3D12API::GetTextureLoader() const
{
	return m_textureLoader;
}

D3D12VertexBufferLoader * D3D12API::GetVertexBufferLoader() const
{
	return m_vertexBufferLoader;
}

USHORT D3D12API::GetNrMeshesCreated() const
{
	return m_meshesCreated;
}

USHORT D3D12API::GetNrTechniquesCreated() const
{
	return m_techniquesCreated;
}

USHORT D3D12API::GetNrTexturesCreated() const
{
	return m_texturesCreated;
}

UINT D3D12API::GetViewSize()
{
	return m_cbv_srv_uav_size;
}

bool D3D12API::InitializeDirect3DDevice()
{
#ifndef DEBUG
	//Enable the D3D12 debug layer.
	ID3D12Debug* debugController = nullptr;

	HMODULE mD3D12 = GetModuleHandle("D3D12.dll");
	PFN_D3D12_GET_DEBUG_INTERFACE f = (PFN_D3D12_GET_DEBUG_INTERFACE)GetProcAddress(mD3D12, "D3D12GetDebugInterface");
	if (SUCCEEDED(f(IID_PPV_ARGS(&debugController))))
	{
		debugController->EnableDebugLayer();
	}
	debugController->Release();
#endif


	//dxgi1_6 is only needed for the initialization process using the adapter.
	IDXGIFactory6*	factory = nullptr;
	IDXGIAdapter1*	adapter = nullptr;
	//First a factory is created to iterate through the adapters available.
	CreateDXGIFactory(IID_PPV_ARGS(&factory));
	for (UINT adapterIndex = 0;; ++adapterIndex)
	{
		adapter = nullptr;
		if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &adapter))
		{
			return false;	//No more adapters to enumerate.
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device5), nullptr)))
		{
			break;
		}

		SafeRelease(&adapter);
	}
	if (adapter)
	{
		HRESULT hr = S_OK;
		//Create the actual device.
		if (!SUCCEEDED(hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&m_device))))
		{
			return false;
		}

		SafeRelease(&adapter);
	}
	else
	{
		return false;
		////Create warp device if no adapter was found.
		//factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
		//D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice5));
	}

	// Retrieve hardware specific descriptor size
	m_cbv_srv_uav_size = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	SafeRelease(&factory);
	return true;
}

Renderer* D3D12API::MakeRenderer(const RendererType rendererType)
{
	D3D12Renderer* renderer = nullptr;

	switch (rendererType)
	{
	case RendererType::Forward:
		renderer = new D3D12ForwardRenderer(this);
		break;
	case RendererType::Raytracing:
		renderer = new D3D12RaytracerRenderer(this);
		break;
	default:
		break;
	}

	if (renderer) {
		if (!renderer->Initialize()) {
			delete renderer;
			renderer = nullptr;
		}
	}


	return renderer;
}
