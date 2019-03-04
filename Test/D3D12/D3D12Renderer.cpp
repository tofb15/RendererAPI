#include "D3D12Renderer.hpp"
#include <d3d12.h>
#include <dxgi1_6.h> //Only used for initialization of the device and swap chain.

#include "D3D12Window.hpp"
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
	m_textureLoader->Kill();	//Notify the other thread to stop.
	m_thread_texture.join();	//Wait for the other thread to stop.
	if (m_textureLoader)
		delete m_textureLoader;
}

bool D3D12Renderer::Initialize()
{
	if (!InitializeDirect3DDevice())
	{
		printf("Error: Could not initialize device\n");
		return false;
	}
	if (!InitializeCommandInterfaces())
	{
		printf("Error: Could not initialize command interfaces\n");
		return false;
	}
	if (!InitializeRootSignature())
	{
		printf("Error: Could not initialize root signature\n");
		return false;
	}
	if (!InitializeMatrixStructuredBuffer())
	{
		printf("Error: Could not initialize structured buffer\n");
		return false;
	}
	if (!InitializeTextureDescriptorHeap())
	{
		printf("Error: Could not initialize descriptor heap\n");
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
ShaderManager * D3D12Renderer::MakeShaderManager()
{
	D3D12ShaderManager* sm = new D3D12ShaderManager(this);
	return sm;
}
D3D12VertexBuffer * D3D12Renderer::MakeVertexBuffer()
{
	return new D3D12VertexBuffer(this);
}

void D3D12Renderer::ClearFrame()
{
}
void D3D12Renderer::ClearSubmissions()
{
	m_items.clear();
}
void D3D12Renderer::Submit(SubmissionItem item, Camera* c)
{

	unsigned short techIndex = static_cast<D3D12Technique*>(item.blueprint->technique)->GetID();
	unsigned short meshIndex = static_cast<D3D12Mesh*>(item.blueprint->mesh)->GetID();

	SortingItem s;
	s.item = item;
	s.sortingIndex = 0;

	//int i2 = sizeof(INT64);
	//int i = sizeof(s.sortingIndex);

	Float3 f = c->GetPosition() - item.transform.pos;

	unsigned int dist = f.length() * 65;
	s.distance = UINT_MAX - dist;
	//s.distance = 0;
	s.meshIndex = meshIndex;
	s.techniqueIndex = techIndex;

	m_items.push_back(s);
}
void D3D12Renderer::Frame(Window* w, Camera* c)
{
	HRESULT hr;
	D3D12Window* window = static_cast<D3D12Window*>(w);
	UINT backBufferIndex;

	std::sort(m_items.begin(), m_items.end(),
		[](const SortingItem & a, const SortingItem & b) -> bool
	{
		return a.sortingIndex > b.sortingIndex;
	});
	
	backBufferIndex = window->GetCurrentBackBufferIndex();

	hr = m_commandAllocator->Reset();
	if (!SUCCEEDED(hr)) printf("Error: Command allocator reset\n");

	hr = m_commandList->Reset(m_commandAllocator, nullptr);
	if (!SUCCEEDED(hr)) printf("Error: Command list reset\n");

	// Heap information
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_descriptorHeap[backBufferIndex]->GetCPUDescriptorHandleForHeapStart();

	// Description (maybe fix another solution)
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;

	m_commandList->SetDescriptorHeaps(1, &m_descriptorHeap[backBufferIndex]);

	//Set root signature
	m_commandList->SetGraphicsRootSignature(m_rootSignature);
	m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Fill out the render information vectors
	SetUpRenderInstructions();

	// Begin mapping matrix data
	D3D12_RANGE readRange = { 0, 0 };
	void* data = nullptr;
	m_structuredBufferResources[backBufferIndex]->Map(0, &readRange, &data);

	// Create shader resource views for each texture on each object
	// Copy matrix data to GPU
	DirectX::XMMATRIX mat;
	size_t nItems = m_items.size();
	for (int item = 0; item < nItems; item++)
	{
		// Retrieve this mesh's textures
		std::vector<Texture*>& textures = m_items[item].item.blueprint->textures;
		size_t nTextures = textures.size();

		// Create a shader resource view for each texture
		for (size_t textureIndex = 0; textureIndex < nTextures; textureIndex++)
		{
			D3D12Texture* texture = static_cast<D3D12Texture*>(textures[textureIndex]);
			int textureIndexOnGPU = texture->IsLoaded() ? texture->GetTextureIndex() : 0;

			ID3D12Resource* textureResource = m_textureLoader->GetResource(textureIndexOnGPU);
			//m_device->CreateShaderResourceView(textureResource, &srvDesc, cdh);

			m_device->CopyDescriptorsSimple(1, cdh, m_textureLoader->GetSpecificTextureCPUAdress(texture), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			// Move the pointer forward in memory
			cdh.ptr += m_cbv_srv_uav_size;
		}
		
		Float3 pos = m_items[item].item.transform.pos;
		Float3 scal = m_items[item].item.transform.scale;

		mat = DirectX::XMMatrixIdentity();

		mat.r[3] = { pos.x, pos.y, pos.z, 1.0f };

		mat.r[0].m128_f32[0] *= scal.x;
		mat.r[1].m128_f32[1] *= scal.y;
		mat.r[2].m128_f32[2] *= scal.z;

		memcpy(static_cast<char*>(data) + sizeof(mat) * item, &mat, sizeof(mat));
	}
	
	// Finish mapping and write the matrix data
	D3D12_RANGE writeRange = { 0, sizeof(mat) * nItems };
	m_structuredBufferResources[backBufferIndex]->Unmap(0, &writeRange);

	// Set structured buffer
	m_commandList->SetGraphicsRootShaderResourceView(2, m_structuredBufferResources[backBufferIndex]->GetGPUVirtualAddress());

	// Set viewport and scissor rectangle
	m_commandList->RSSetViewports(1, window->GetViewport());
	m_commandList->RSSetScissorRects(1, window->GetScissorRect());

	//Indicate that the back buffer will be used as render target
#pragma region Barrier Swap Target
	D3D12_RESOURCE_BARRIER barrierDesc = {};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = window->GetCurrentRenderTargetResource();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	m_commandList->ResourceBarrier(1, &barrierDesc);
#pragma endregion 

	window->ClearRenderTarget(m_commandList);
	window->SetRenderTarget(m_commandList);

	D3D12Camera* cam = static_cast<D3D12Camera*>(c);
	DirectX::XMFLOAT4X4 viewPersp = cam->GetViewPerspective();
	m_commandList->SetGraphicsRoot32BitConstants(0, 16, &viewPersp, 0);

	for (size_t instIndex = 0; instIndex < m_renderInstructions.size(); instIndex++)
	{
		RecordRenderInstructions(backBufferIndex, instIndex, instIndex);
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

	m_commandList->ResourceBarrier(1, &barrierDesc);
#pragma endregion
	m_commandList->Close();

	//Execute the command list.
	ID3D12CommandList* listsToExecute[] = { m_commandList };
	window->GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	//Present the frame.
	DXGI_PRESENT_PARAMETERS pp = {};
	window->GetSwapChain()->Present1(0, 0, &pp);
	window->WaitForGPU();
}

void D3D12Renderer::SetUpRenderInstructions()
{
	unsigned short meshID_last = m_items[0].meshIndex;
	unsigned short techID_last = m_items[0].techniqueIndex;
	unsigned short meshID_curr;
	unsigned short techID_curr;
	unsigned int nrOfInstances = 0;

	// Clear render instructions from previous frame
	m_renderInstructions.clear();
	m_instanceOffsets.clear();

	// Start with setting new technique
	m_renderInstructions.push_back(-1);

	// Setting a new technique does not increase the total instance count
	m_instanceOffsets.push_back(0);

	// The first draw call has no instance offset
	m_instanceOffsets.push_back(0);

	// Create render instructions
	size_t nItems = m_items.size();
	for (int i = 0; i < nItems; i++)
	{
		//Check Mesh and Tech
		techID_curr = m_items[i].techniqueIndex;
		meshID_curr = m_items[i].meshIndex;

		if (techID_curr != techID_last)
		{
			// This is not the last technique

			techID_last = techID_curr;

			// Add an instanced draw call instruction
			m_renderInstructions.push_back(nrOfInstances);

			// Add up the total amount of instances in this draw call
			m_instanceOffsets.push_back(m_instanceOffsets.back() + nrOfInstances);

			// Add a technique swap instruction
			m_renderInstructions.push_back(-1);

			// Setting a new technique does not increase the total instance count
			m_instanceOffsets.push_back(m_instanceOffsets.back());

			nrOfInstances = 0;

			if (meshID_curr != meshID_last)
			{
				meshID_last = meshID_curr;
			}
		}
		else if (meshID_curr != meshID_last)
		{
			// This is not the last mesh

			meshID_last = meshID_curr;

			// Add an instanced draw call instruction
			m_renderInstructions.push_back(nrOfInstances);

			// Add up the total amount of instances in this draw call
			m_instanceOffsets.push_back(m_instanceOffsets.back() + nrOfInstances);

			nrOfInstances = 0;
		}
		nrOfInstances++;
	}

	//Start with setting new technique
	m_renderInstructions.push_back(nrOfInstances);
}
void D3D12Renderer::RecordRenderInstructions(int backBufferIndex, int firstInstructionIdx, int lastInstructionIdx)
{
	for (int instructionIndex = firstInstructionIdx; instructionIndex < lastInstructionIdx + 1; instructionIndex++)
	{
		// Retrieve instruction
		int instruction = m_renderInstructions[instructionIndex];

		// Retrieve the offset in objects for this instruction
		int instanceOffset = m_instanceOffsets[instructionIndex];
		
		if (instruction == -1)
		{
			// Retrieve and enable a new technique from the first object using it
			m_items[instanceOffset].item.blueprint->technique->Enable();
		}
		else
		{
			// Retrieve and enable a new mesh from the first object using it

			// Set instance offset
			m_commandList->SetGraphicsRoot32BitConstants(0, 1, &instanceOffset, 16);

			D3D12_GPU_DESCRIPTOR_HANDLE handle = m_descriptorHeap[backBufferIndex]->GetGPUDescriptorHandleForHeapStart();
			handle.ptr += instanceOffset * m_cbv_srv_uav_size;
			m_commandList->SetGraphicsRootDescriptorTable(1, handle);

			//Set Vertex Buffers
			std::vector<D3D12VertexBuffer*>& buffers = *static_cast<D3D12Mesh*>(m_items[instanceOffset].item.blueprint->mesh)->GetVertexBuffers();
			for (size_t j = 0; j < buffers.size(); j++)
			{
				m_commandList->IASetVertexBuffers(j, 1, buffers[j]->GetView());
			}
			//Draw
			m_commandList->DrawInstanced(buffers[0]->GetNumberOfElements(), instruction, 0, 0);
		}
	}
}

ID3D12Device4 * D3D12Renderer::GetDevice() const
{
	return m_device;
}
ID3D12RootSignature * D3D12Renderer::GetRootSignature() const
{
	return m_rootSignature;
}
ID3D12GraphicsCommandList3 * D3D12Renderer::GetCommandList() const
{
	return m_commandList;
}
D3D12TextureLoader * D3D12Renderer::GetTextureLoader() const
{
	return m_textureLoader;
}

bool D3D12Renderer::InitializeDirect3DDevice()
{
#ifdef _DEBUG
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
bool D3D12Renderer::InitializeCommandInterfaces()
{
	HRESULT hr;

	hr = m_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_commandAllocator));
	if (FAILED(hr))
	{
		return false;
	}

	//Create command list.
	hr = m_device->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_DIRECT,
		m_commandAllocator,
		nullptr,
		IID_PPV_ARGS(&m_commandList));
	if (FAILED(hr))
	{
		return false;
	}

	//Command lists are created in the recording state. Since there is nothing to
	//record right now and the main loop expects it to be closed, we close it.
	m_commandList->Close();
	return true;
}
bool D3D12Renderer::InitializeRootSignature()
{
	HRESULT hr;
	ID3DBlob* sBlob;
	D3D12_STATIC_SAMPLER_DESC samplerDesc[1] = {};
	D3D12_DESCRIPTOR_RANGE descRange[1] = {};
	D3D12_ROOT_DESCRIPTOR_TABLE rootDescTable = {};
	D3D12_ROOT_PARAMETER rootParams[3];
	D3D12_ROOT_SIGNATURE_DESC rootSigDesc;
	
	samplerDesc[0].RegisterSpace = 0;
	samplerDesc[0].ShaderRegister = 0;
	samplerDesc[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	samplerDesc[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc[0].MinLOD = 0;
	samplerDesc[0].MaxLOD = 1;
	samplerDesc[0].MipLODBias = 0;
	samplerDesc[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;

	// Bindless texture descriptor range
	descRange[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	descRange[0].NumDescriptors = -1; // No bounds (bindless)
	descRange[0].BaseShaderRegister = 0;
	descRange[0].RegisterSpace = 0;

	rootDescTable.NumDescriptorRanges = 1;
	rootDescTable.pDescriptorRanges = descRange;

	// Root constants
	rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
	rootParams[0].Constants.Num32BitValues = 16 + 4;		// 1 * float4x4 + 1 * int
	rootParams[0].Constants.RegisterSpace = 0;
	rootParams[0].Constants.ShaderRegister = 0;
	rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	// Texture descriptor table
	rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
	rootParams[1].DescriptorTable = rootDescTable;

	// Structured buffer view
	rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
	rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
	rootParams[2].Descriptor.ShaderRegister = 1;
	rootParams[2].Descriptor.RegisterSpace = 0;

	rootSigDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
	rootSigDesc.NumParameters = sizeof(rootParams) / sizeof(D3D12_ROOT_PARAMETER);
	rootSigDesc.pParameters = rootParams;
	rootSigDesc.NumStaticSamplers = 1;
	rootSigDesc.pStaticSamplers = samplerDesc;

	hr = D3D12SerializeRootSignature(
		&rootSigDesc,
		D3D_ROOT_SIGNATURE_VERSION_1,
		&sBlob,
		nullptr);
	if (FAILED(hr))
	{
		return false;
	}

	hr = m_device->CreateRootSignature(
		0,
		sBlob->GetBufferPointer(),
		sBlob->GetBufferSize(),
		IID_PPV_ARGS(&m_rootSignature));
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}
bool D3D12Renderer::InitializeMatrixStructuredBuffer()
{
	HRESULT hr;
	D3D12_HEAP_PROPERTIES hp = {};
	D3D12_RESOURCE_DESC rd = { };

	hp.Type = D3D12_HEAP_TYPE_UPLOAD;
	hp.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	hp.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
	hp.CreationNodeMask = 1;
	hp.VisibleNodeMask = 1;

	rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	rd.Width = NUM_MATRICES_IN_BUFFER * sizeof(DirectX::XMMATRIX);
	//rd.Width = 10240 * 16 * 4;
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
		hr = m_device->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_structuredBufferResources[i]));
		if (FAILED(hr))
			return false;
	}

	return true;
}
bool D3D12Renderer::InitializeTextureDescriptorHeap()
{
	HRESULT hr;
	D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};

	heapDescriptorDesc.NumDescriptors = NUM_DESCRIPTORS_IN_HEAP;
	heapDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		hr = m_device->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&m_descriptorHeap[i]));
		if (FAILED(hr))
			return false;
	}

	return true;
}