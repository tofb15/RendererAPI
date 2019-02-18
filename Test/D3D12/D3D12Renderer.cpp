#include "D3D12Renderer.hpp"
#include <d3d12.h>
#include <dxgi1_6.h> //Only used for initialization of the device and swap chain.
//#include <d3dcompiler.h>
//#include <DirectXMath.h>

#include "D3D12Camera.hpp"
#include "D3D12Texture.hpp"
#include "D3D12Technique.hpp"
#include "D3D12Material.hpp"
#include "D3D12Mesh.hpp"
#include "D3D12RenderState.hpp"






#include "D3D12Window.hpp"
#pragma comment(lib, "d3d12.lib")

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
	InitializeDirect3DDevice();

	InitializeCommandInterfaces();

	

	return true;
}

Camera * D3D12Renderer::MakeCamera()
{
	return new D3D12Camera();
}

Window * D3D12Renderer::MakeWindow()
{
	return new D3D12Window(this);
}

Texture * D3D12Renderer::MakeTexture()
{
	return new D3D12Texture;
}

Mesh * D3D12Renderer::MakeMesh()
{
	return new D3D12Mesh;
}

Material * D3D12Renderer::MakeMaterial()
{
	return new D3D12Material;
}

RenderState * D3D12Renderer::MakeRenderState()
{
	return new D3D12RenderState;
}

Technique * D3D12Renderer::MakeTechnique(Material * m, RenderState * r)
{
	return new D3D12Technique(static_cast<D3D12Material*>(m), static_cast<D3D12RenderState*>(r));
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
	
	UINT backBufferIndex = window->GetCurrentBackBufferIndex();

	mCommandAllocator->Reset();
	mCommandList4->Reset(mCommandAllocator, nullptr);

	//Set constant buffer descriptor heap
	//ID3D12DescriptorHeap* descriptorHeaps[] = { mDescriptorHeap[backBufferIndex], mSamplerHeap };
	//mCommandList4->SetDescriptorHeaps(ARRAYSIZE(descriptorHeaps), descriptorHeaps);

	//Set root signature
	//mCommandList4->SetGraphicsRootSignature(mRootSignature);

	//Set root descriptor table to index 0 in previously set root signature
	//mCommandList4->SetGraphicsRootDescriptorTable(0, mDescriptorHeap[backBufferIndex]->GetGPUDescriptorHandleForHeapStart());
	//mCommandList4->SetGraphicsRootDescriptorTable(2, mSamplerHeap->GetGPUDescriptorHandleForHeapStart());

	//Set necessary states.
	mCommandList4->RSSetViewports(1, window->GetViewport());
	mCommandList4->RSSetScissorRects(1, window->GetScissorRect());

	////Indicate that the back buffer will be used as render target.
#pragma region Barrier Swap Target
	D3D12_RESOURCE_BARRIER barrierDesc = {};

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = window->GetCurrentRenderTargetResource();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	mCommandList4->ResourceBarrier(1, &barrierDesc);
#pragma endregion

	window->ClearRenderTarget(mCommandList4);


#pragma region Barrier Swap Target
	barrierDesc = {};

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = window->GetCurrentRenderTargetResource();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	mCommandList4->ResourceBarrier(1, &barrierDesc);
#pragma endregion
	mCommandList4->Close();
}

void D3D12Renderer::Present(Window * w)
{
	D3D12Window* window = static_cast<D3D12Window*>(w);

	//Execute the command list.
	ID3D12CommandList* listsToExecute[] = { mCommandList4 };
	window->GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	//Present the frame.
	DXGI_PRESENT_PARAMETERS pp = {};
	window->GetSwapChain()->Present1(0, 0, &pp);
}

void D3D12Renderer::ClearFrame()
{
}

ID3D12Device4 * D3D12Renderer::GetDevice() const
{
	return mDevice5;
}

bool D3D12Renderer::InitializeDirect3DDevice()
{
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
		if (SUCCEEDED(D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, __uuidof(ID3D12Device4), nullptr)))
		{
			break;
		}

		SafeRelease(&adapter);
	}
	if (adapter)
	{
		HRESULT hr = S_OK;
		//Create the actual device.
		if (!SUCCEEDED(hr = D3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_12_1, IID_PPV_ARGS(&mDevice5))))
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

	SafeRelease(&factory);
	return true;
}

bool D3D12Renderer::InitializeCommandInterfaces()
{
	HRESULT hr;

	hr = mDevice5->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&mCommandAllocator));
	if (FAILED(hr))
	{
		return false;
	}

	//Create command list.
	hr = mDevice5->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		mCommandAllocator,
		nullptr,
		IID_PPV_ARGS(&mCommandList4));
	if (FAILED(hr))
	{
		return false;
	}

	//Command lists are created in the recording state. Since there is nothing to
	//record right now and the main loop expects it to be closed, we close it.
	mCommandList4->Close();

	return true;
}

bool D3D12Renderer::InitializeFenceAndEventHandle()
{
	return false;
}
