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
#include "D3D12VertexBuffer.hpp"
#include "D3D12RenderState.hpp"
#include "D3D12ShaderManager.hpp"






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

D3D12Renderer::~D3D12Renderer()
{
}

bool D3D12Renderer::Initialize()
{
	InitializeDirect3DDevice();

	InitializeCommandInterfaces();

	InitializeRootSignature();

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
	return new D3D12Mesh(this);
}

Material * D3D12Renderer::MakeMaterial()
{
	return new D3D12Material;
}

RenderState * D3D12Renderer::MakeRenderState()
{
	return new D3D12RenderState;
}

Technique * D3D12Renderer::MakeTechnique(RenderState* rs, ShaderProgram* sp, ShaderManager* sm)
{
	D3D12Technique* tech = new D3D12Technique(this);
	if (!tech->Initialize(static_cast<D3D12RenderState*>(rs), sp, static_cast<D3D12ShaderManager*>(sm)))
	{
		delete tech;
		return nullptr;
	}
	return tech;
}

D3D12VertexBuffer * D3D12Renderer::MakeVertexBuffer()
{
	return new D3D12VertexBuffer(this);
}

void D3D12Renderer::Submit(SubmissionItem item)
{
	items.push_back(item);
}

void D3D12Renderer::ClearSubmissions()
{
	items.clear();
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
	mCommandList4->SetGraphicsRootSignature(mRootSignature);
	mCommandList4->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

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

	for (size_t i = 0; i < items.size(); i++)
	{
		items[i].blueprint->technique->Enable();

		std::vector<D3D12VertexBuffer*> buffers = *static_cast<D3D12Mesh*>(items[i].blueprint->mesh)->GetVertexBuffers();

		mCommandList4->IASetVertexBuffers(0, 1, buffers[0]->GetView());
		mCommandList4->DrawInstanced(3, 1, 0, 0);
		//mCommandList4->s
	}

}

void D3D12Renderer::Present(Window * w)
{
	D3D12Window* window = static_cast<D3D12Window*>(w);

#pragma region Barrier Swap Target
	D3D12_RESOURCE_BARRIER barrierDesc = {};

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = window->GetCurrentRenderTargetResource();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

	mCommandList4->ResourceBarrier(1, &barrierDesc);
#pragma endregion
	mCommandList4->Close();

	//Execute the command list.
	ID3D12CommandList* listsToExecute[] = { mCommandList4 };
	window->GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	//Present the frame.
	DXGI_PRESENT_PARAMETERS pp = {};
	window->GetSwapChain()->Present1(0, 0, &pp);

	WaitForGpu();

}

void D3D12Renderer::ClearFrame()
{
}

ID3D12Device4 * D3D12Renderer::GetDevice() const
{
	return mDevice5;
}

ID3D12RootSignature * D3D12Renderer::GetRootSignature() const
{
	return mRootSignature;
}

ID3D12GraphicsCommandList3 * D3D12Renderer::GetCommandList() const
{
	return mCommandList4;
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

bool D3D12Renderer::InitializeRootSignature()
{
	HRESULT hr;

	//create root parameter
	D3D12_ROOT_PARAMETER rootParam[1];
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam[0].Constants.Num32BitValues = 8;		// 2 * float4
	rootParam[0].Constants.RegisterSpace = 0;
	rootParam[0].Constants.ShaderRegister = 0;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;


	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = 1;
	rsDesc.pParameters = rootParam;
	rsDesc.NumStaticSamplers = 0;
	rsDesc.pStaticSamplers = nullptr;

	ID3DBlob* sBlob;
	hr = D3D12SerializeRootSignature(
		&rsDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&sBlob,
		nullptr);
	if (FAILED(hr))
	{
		return false;
	}

	hr = mDevice5->CreateRootSignature(
		0,
		sBlob->GetBufferPointer(),
		sBlob->GetBufferSize(),
		IID_PPV_ARGS(&mRootSignature));
	if (FAILED(hr))
	{
		return false;
	}

	//for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	//{
	//	D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};
	//	heapDescriptorDesc.NumDescriptors = 1;		// Make room for (NOT one cb) and one srv
	//	heapDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//	heapDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	//	hr = mDevice5->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&mDescriptorHeap[i]));
	//	if (FAILED(hr))
	//		return false;
	//}

	return true;
}

ShaderManager * D3D12Renderer::MakeShaderManager()
{
	D3D12ShaderManager* sm = new D3D12ShaderManager(this);

	return sm;
}
