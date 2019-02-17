#include "D3D12Renderer.hpp"
#include <d3d12.h>
#include <dxgi1_6.h> //Only used for initialization of the device and swap chain.
//#include <d3dcompiler.h>
//#include <DirectXMath.h>

#include "D3D12Window.hpp"

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

D3D12Renderer::D3D12Renderer()
{
}

bool D3D12Renderer::Initialize()
{
	return false;
}

Camera * D3D12Renderer::MakeCamera()
{
	return nullptr;
}

Window * D3D12Renderer::MakeWindow()
{
	return new D3D12Window(this);
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

void D3D12Renderer::Frame(Window* w)
{
	D3D12Window* window = static_cast<D3D12Window*>(w);
	//UINT backBufferIndex = window->GetCurrentBackBufferIndex();


}

void D3D12Renderer::Present()
{
}

void D3D12Renderer::ClearFrame()
{
}

ID3D12Device4 * D3D12Renderer::GetDevice() const
{
	return mDevice5;
}

bool D3D12Renderer::InitializeDirect3DDevice(HWND wndHandle)
{

	//IDXGIFactory6* factory = nullptr;
	//IDXGIAdapter1* adapter = nullptr;

	////First a factory is created to iterate through the adapters available
	//CreateDXGIFactory(IID_PPV_ARGS(&factory));
	//for (UINT adapterIndex = 0;; adapterIndex++)
	//{
	//	adapter = nullptr;
	//	if (DXGI_ERROR_NOT_FOUND == factory->EnumAdapters1(adapterIndex, &adapter)) {
	//		return false;
	//	}

	//	// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
	//	//Since the last parameter is nullptr the device will not be created yet.
	//	if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device4), nullptr))) {
	//		break;
	//	}

	//	SafeRelease(&adapter);
	//}

	//if (adapter)
	//{
	//	HRESULT hr = S_OK;
	//	//Create the actual device.
	//	if (!SUCCEEDED(hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&mDevice5))))
	//	{
	//		return false;
	//	}

	//	SafeRelease(&adapter);
	//}
	//else
	//{
	//	//If the current computer do not support D3D_FEATURE_LEVEL_12_1 another version could be created here. for exemple 12.0 or 11.0
	//	return false;
	//	////Create warp device if no adapter was found.
	//	//factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter));
	//	//D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&mDevice5));
	//}

	//SafeRelease(&factory);

	return true;
}

bool D3D12Renderer::InitializeCommandInterfacesAndSwapChain(HWND wndHandle)
{
	HRESULT hr;

//#pragma region CommandQueue, List and Allocator
//	//Describe and create the command queue.
//	D3D12_COMMAND_QUEUE_DESC cqd = {};
//	hr = mDevice5->CreateCommandQueue(&cqd, IID_PPV_ARGS(&mCommandQueue));
//	if (FAILED(hr))
//	{
//		return false;
//	}
//
//	//Create command allocator. The command allocator object corresponds
//	//to the underlying allocations in which GPU commands are stored.
//	hr = mDevice5->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));
//	if (FAILED(hr)) //Returns E_OUTOFMEMORY if the GPU has not enough memmory else S_OK
//	{
//		return false;
//	}
//
//	//Create command list.
//	hr = mDevice5->CreateCommandList(
//		0,
//		D3D12_COMMAND_LIST_TYPE_DIRECT,
//		mCommandAllocator,
//		nullptr,
//		IID_PPV_ARGS(&mCommandList4));
//	if (FAILED(hr)) //Returns E_OUTOFMEMORY if the GPU has not enough memmory else S_OK
//	{
//		return false;
//	}
//
//	//Command lists are created in the recording state. Since there is nothing to
//	//record right now and the main loop expects it to be closed, we close it.
//	mCommandList4->Close();
//
//	IDXGIFactory5*	factory = nullptr;
//	hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));
//	if (FAILED(hr))
//	{
//		return false;
//	}
//#pragma endregion
//
//#pragma region SwapChain
//	//Create swap chain.
//	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
//	scDesc.Width = 0;
//	scDesc.Height = 0;
//	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	scDesc.Stereo = FALSE;
//	scDesc.SampleDesc.Count = 1;
//	scDesc.SampleDesc.Quality = 0;
//	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
//	scDesc.BufferCount = NUM_SWAP_BUFFERS;
//	scDesc.Scaling = DXGI_SCALING_NONE;
//	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
//	scDesc.Flags = 0;
//	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
//
//	IDXGISwapChain1* swapChain1 = nullptr;
//
//	hr = factory->CreateSwapChainForHwnd(
//		mCommandQueue,
//		wndHandle,
//		&scDesc,
//		nullptr,
//		nullptr,
//		&swapChain1);
//	if (SUCCEEDED(hr))
//	{
//		hr = swapChain1->QueryInterface(IID_PPV_ARGS(&mSwapChain4));
//		if (SUCCEEDED(hr))
//		{
//			mSwapChain4->Release();
//		}
//		else
//		{
//			return false;
//		}
//	}
//	else
//	{
//		return false;
//	}
//
//	SafeRelease(&factory);
//
//#pragma endregion

	return true;
}

bool D3D12Renderer::InitializeFenceAndEventHandle()
{
	return false;
}

bool D3D12Renderer::InitializeRenderTargets()
{
	return false;
}

void D3D12Renderer::InitializeViewportAndScissorRect(unsigned int width, unsigned int height)
{
}
