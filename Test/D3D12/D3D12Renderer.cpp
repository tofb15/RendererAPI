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
#include "D3D12TextureLoader.hpp"

#include <algorithm>
#include <functional>

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
	mTextureLoader->Kill();	//Notify the other thread to stop.
	thread_texture.join();	//Wait for the other thread to stop.
	if (mTextureLoader)
		delete mTextureLoader;
}

#include <iostream>

bool D3D12Renderer::Initialize()
{
	InitializeDirect3DDevice();

	InitializeCommandInterfaces();

	InitializeRootSignature();

	InitializeBigConstantBuffer();

	mTextureLoader = new D3D12TextureLoader(this);
	mTextureLoader->Initialize();

	//Start new Texture loading thread
	std::cout << thread_texture.get_id() << std::endl;

	thread_texture = std::thread(&D3D12TextureLoader::DoWork, &*mTextureLoader);

	std::cout << thread_texture.get_id() << std::endl;

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
	return new D3D12Texture(this);
}

Mesh * D3D12Renderer::MakeMesh()
{
	if (++m_meshesCreated == 0)
		return nullptr;

	return new D3D12Mesh(this, m_meshesCreated);
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
	if (++m_techniquesCreated == 0)
		return nullptr;

	D3D12Technique* tech = new D3D12Technique(this, m_techniquesCreated);
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
	unsigned short techIndex = static_cast<D3D12Technique*>(item.blueprint->technique)->GetID();
	unsigned short meshIndex = static_cast<D3D12Mesh*>(item.blueprint->mesh)->GetID();

	unsigned int sortIndex = techIndex << (sizeof(unsigned short) * 8);//
	sortIndex += meshIndex;

	unsigned short test = sortIndex;

	items.push_back({sortIndex, techIndex, meshIndex, item });//pushing a SortingItem struct
}

void D3D12Renderer::ClearSubmissions()
{
	items.clear();
}

void D3D12Renderer::Frame(Window* w, Camera* c)
{

	int breakpoint = 0;

	std::sort(items.begin(), items.end(),
		[](const SortingItem & a, const SortingItem & b) -> bool
	{
		return a.sortingIndex > b.sortingIndex;
	});

	D3D12Window* window = static_cast<D3D12Window*>(w);
	
	UINT backBufferIndex = window->GetCurrentBackBufferIndex();

	mCommandAllocator->Reset();
	mCommandList4->Reset(mCommandAllocator, nullptr);

	//Set constant buffer descriptor heap
	int nTextureDescriptorHeaps = mTextureLoader->GetNumberOfHeaps();
	ID3D12DescriptorHeap** textureHeaps = mTextureLoader->GetAllHeaps();

	int nDescriptorHeapsTotal = nTextureDescriptorHeaps;

	ID3D12DescriptorHeap** heapsToUse = new ID3D12DescriptorHeap*[nDescriptorHeapsTotal];

	for (size_t i = 0; i < nTextureDescriptorHeaps; i++)
	{
		heapsToUse[i] = textureHeaps[i];
	}

	//ID3D12DescriptorHeap* descriptorHeaps[] = { test/*mTextureLoader->GetDescriptorHeap()*/ };
	mCommandList4->SetDescriptorHeaps(nDescriptorHeapsTotal, heapsToUse );

	//Set root signature
	mCommandList4->SetGraphicsRootSignature(mRootSignature);
	mCommandList4->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	D3D12_RANGE readRange = { 0, 0 };
	void* data = nullptr;
	m_constantBufferResource[backBufferIndex]->Map(0, &readRange, &data);

	static float time = 0;
	time += 0.001f;

	DirectX::XMFLOAT4X4 mat = {};

	std::vector<int> renderInstructions; //-1 means new technique. everything else is number of instances to draw.

	unsigned short meshID_last = 0;
	unsigned short techID_last = 0;
	unsigned short meshID_curr;
	unsigned short techID_curr;
	unsigned int nOfInstances;

	renderInstructions.push_back(-1);//Start with setting new technique

	for (int i = 0; i < items.size(); i++)
	{
		techID_curr = items[i].techniqueIndex;
		meshID_curr = items[i].meshIndex;
		nOfInstances++;

		if (techID_curr != techID_last) {
			techID_last = techID_curr;
			renderInstructions.push_back(nOfInstances);
			renderInstructions.push_back(-1);
			nOfInstances = 0;

			if (meshID_curr != meshID_last) {
				meshID_last = meshID_curr;
			}
		}
		else if (meshID_curr != meshID_last) {
			meshID_last = meshID_curr;
			renderInstructions.push_back(nOfInstances);
			nOfInstances = 0;
		}

		Float3 pos = items[i].item.transform.pos;
		Float3 scal = items[i].item.transform.scale;
		DirectX::XMMATRIX posMat = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
		DirectX::XMMATRIX scalMat = DirectX::XMMatrixScaling(scal.x, scal.y, scal.z);

		DirectX::XMMATRIX mat = DirectX::XMMatrixTranspose(posMat/* * scalMat*/);
		//DirectX::XMMATRIX mat = DirectX::XMMatrixTranspose(DirectX::XMMatrixTranslation(5 * sin(time), 5 * sin(time), 0));
		memcpy(static_cast<char*>(data) + sizeof(DirectX::XMMATRIX) * i, &mat, sizeof(DirectX::XMMATRIX));
	}

	D3D12_RANGE writeRange = { 0, sizeof(DirectX::XMMATRIX) * items.size() };
	m_constantBufferResource[backBufferIndex]->Unmap(0, &writeRange);

	mCommandList4->SetGraphicsRootShaderResourceView(2, m_constantBufferResource[backBufferIndex]->GetGPUVirtualAddress());

	//Set root descriptor table to index 0 in previously set root signature
	//mCommandList4->SetGraphicsRootDescriptorTable(0, mDescriptorHeap[backBufferIndex]->GetGPUDescriptorHandleForHeapStart());
	//mCommandList4->SetGraphicsRootDescriptorTable(2, mSamplerHeap->GetGPUDescriptorHandleForHeapStart());

		//Set necessary states.
	mCommandList4->RSSetViewports(1, window->GetViewport());
	mCommandList4->RSSetScissorRects(1, window->GetScissorRect());
	//mCommandList4->SetGraphicsRootConstantBufferView(2, );
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
	window->SetRenderTarget(mCommandList4);

	D3D12Camera* cam = static_cast<D3D12Camera*>(c);
	DirectX::XMFLOAT4X4 viewPersp = cam->GetViewPerspective();
	mCommandList4->SetGraphicsRoot32BitConstants(0, 16, &viewPersp, 0);

	// Offset between instanced draws
	//mCommandList4->SetGraphicsRoot32BitConstants(0, 1, &i, 16);

	int itemIndex = 0;
	for (size_t instIndex = 0; instIndex < renderInstructions.size(); instIndex++)
	{
		int instruction = renderInstructions[instIndex];
		if (instruction == -1) {
			items[itemIndex].item.blueprint->technique->Enable();
			continue;
		}
	}

	for (size_t i = 0; i < newTechniqueStart.size(); i++)
	{
		items[newTechniqueStart[i]].item.blueprint->technique->Enable();

		D3D12_GPU_DESCRIPTOR_HANDLE handle = mTextureLoader->GetSpecificTextureGPUAdress(static_cast<D3D12Texture*>(items[i].item.blueprint->textures[0]));
		mCommandList4->SetGraphicsRootDescriptorTable(1, handle);

		std::vector<D3D12VertexBuffer*>& buffers = *static_cast<D3D12Mesh*>(items[i].item.blueprint->mesh)->GetVertexBuffers();

		for (size_t j = 0; j < buffers.size(); j++)
		{
			mCommandList4->IASetVertexBuffers(j, 1, buffers[j]->GetView());
		}

		mCommandList4->DrawInstanced(buffers[0]->GetNumberOfElements(), items.size(), 0, 0);
		//mCommandList4->s
	}

	delete[] heapsToUse;
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
	window->WaitForGPU();

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

D3D12TextureLoader * D3D12Renderer::GetTextureLoader() const
{
	return mTextureLoader;
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

	D3D12_STATIC_SAMPLER_DESC samp[1] = {};
	samp[0].RegisterSpace = 0;
	samp[0].ShaderRegister = 0;
	samp[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	samp[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samp[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samp[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samp[0].MinLOD = 0;
	samp[0].MaxLOD = 1;
	samp[0].MipLODBias = 0;
	samp[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;


	D3D12_DESCRIPTOR_RANGE dr[1] = {};
	dr[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	dr[0].NumDescriptors = -1; //No bounds (bindless)
	dr[0].BaseShaderRegister = 0;
	dr[0].RegisterSpace = 0;


	D3D12_ROOT_DESCRIPTOR_TABLE rdt = {};
	rdt.NumDescriptorRanges = 1;
	rdt.pDescriptorRanges = dr;

	//create root parameter
	D3D12_ROOT_PARAMETER rootParam[3];
	rootParam[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParam[0].Constants.Num32BitValues = 16 + 4;		// 1 * float4x4 + 1 * int
	rootParam[0].Constants.RegisterSpace = 0;
	rootParam[0].Constants.ShaderRegister = 0;
	rootParam[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	rootParam[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParam[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParam[1].DescriptorTable = rdt;

	rootParam[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParam[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParam[2].Descriptor.ShaderRegister = 1;
	rootParam[2].Descriptor.RegisterSpace = 0;

	D3D12_ROOT_SIGNATURE_DESC rsDesc;
	rsDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rsDesc.NumParameters = sizeof(rootParam) / sizeof(D3D12_ROOT_PARAMETER);
	rsDesc.pParameters = rootParam;
	rsDesc.NumStaticSamplers = 1;
	rsDesc.pStaticSamplers = samp;

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

bool D3D12Renderer::InitializeBigConstantBuffer()
{
	D3D12_HEAP_PROPERTIES hp = {};
	hp.Type = D3D12_HEAP_TYPE_UPLOAD;
	hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;

	D3D12_RESOURCE_DESC rd = { };
	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = 10240 * 16 * 4;
	rd.Alignment = D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

	rd.Height = 1;
	rd.DepthOrArraySize = 1;
	rd.MipLevels = 1;
	rd.Format = DXGI_FORMAT_UNKNOWN;
	rd.SampleDesc.Count = 1;
	rd.SampleDesc.Quality = 0;
	rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
	rd.Flags = D3D12_RESOURCE_FLAG_NONE;

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		HRESULT hr = mDevice5->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_constantBufferResource[i]));
		if (FAILED(hr))
		{
			return false;
		}
	}

	return true;
}


ShaderManager * D3D12Renderer::MakeShaderManager()
{
	D3D12ShaderManager* sm = new D3D12ShaderManager(this);

	return sm;
}
