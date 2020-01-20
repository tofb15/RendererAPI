#include "D3D12ForwardRenderer.h"
#include "..\D3D12Camera.hpp"
#include "..\D3D12ParticleSystem.hpp"
#include "..\D3D12Window.hpp"
#include "..\D3D12Technique.hpp"
#include "..\D3D12Mesh.hpp"
#include "..\D3D12API.hpp"
#include "..\D3D12Texture.hpp"
#include "..\D3D12VertexBuffer.hpp"

#include <algorithm>
#include <functional>
#include <comdef.h>
#include <iostream>

#define MULTI_THREADED
//Public
D3D12ForwardRenderer::D3D12ForwardRenderer(D3D12API* d3d12) : D3D12Renderer(d3d12)
{}

D3D12ForwardRenderer::~D3D12ForwardRenderer()
{
	m_isRunning = false;

#ifdef MULTI_THREADED
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv_workers.notify_all();
	}
	for (int i = 0; i < NUM_RECORDING_THREADS; i++)
	{
		m_recorderThreads[i].join();
	}
#endif

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		for (int j = 0; j < NUM_COMMAND_LISTS; j++)
		{
			m_commandLists[i][j]->Release();
			m_commandAllocators[i][j]->Release();
		}
	}


	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_structuredBufferResources[i]->Release();
	}

	m_descriptorHeap->Release();
	m_rootSignature->Release();
}

bool D3D12ForwardRenderer::Initialize()
{
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
	if (!InitializeCommandInterfaces())
	{
		printf("Error: Could not initialize command interfaces\n");
		return false;
	}


	m_numActiveWorkerThreads = 0;
#ifdef MULTI_THREADED
	for (int i = 0; i < NUM_RECORDING_THREADS; i++)
	{
		m_recorderThreads[i] = std::thread(&D3D12ForwardRenderer::RecordCommands, this, i);
		std::cout << std::hex << m_recorderThreads[i].get_id() << std::dec << std::endl;
	}
#endif

	return true;
}

void D3D12ForwardRenderer::ClearFrame()
{}

void D3D12ForwardRenderer::ClearSubmissions()
{
	m_items.clear();

	for (size_t i = 0; i < 100; i++)
	{
		m_closestMeshType_lastFrame[i] = m_closestMeshType[i];
		m_closestMeshType[i] = 0;
		m_closestMeshType_float[i] = 10000.0f;

		m_closestTechniqueType_lastFrame[i] = m_closestTechniqueType[i];
		m_closestTechniqueType[i] = 0;
		m_closestTechniqueType_float[i] = 10000.0f;
	}
}

void D3D12ForwardRenderer::Submit(SubmissionItem item, Camera* c, unsigned char layer)
{
	D3D12Camera* camera = static_cast<D3D12Camera*>(c);

	Sphere sphere;
	sphere.center = item.transform.pos;
	sphere.radius = max(item.transform.scale.x, max(item.transform.scale.y, item.transform.scale.z));

	if (!camera->GetFrustum().CheckAgainstFrustum(sphere))
		return;

	unsigned short techIndex = static_cast<D3D12Technique*>(item.blueprint->technique)->GetID();
	unsigned short meshIndex = static_cast<D3D12Mesh*>(item.blueprint->mesh)->GetID();

	SortingItem s;
	s.item = item;
	s.sortingIndex = 0U;

	//int i2 = sizeof(INT64);
	//int i = sizeof(s.sortingIndex);
	int meshTechindex = (techIndex - 1) * m_d3d12->GetNrMeshesCreated() + (meshIndex - 1);

	if (c != nullptr) {
		Float3 f = item.transform.pos - c->GetPosition();

		unsigned int dist = f.length() * 65;
		s.distance = UINT_MAX - min(dist, UINT_MAX);

		if (dist < m_closestMeshType_float[meshTechindex]) {
			m_closestMeshType_float[meshTechindex] = dist;
			m_closestMeshType[meshTechindex] = USHRT_MAX - (unsigned short)(dist);
		}

		if (dist < m_closestTechniqueType_float[techIndex - 1]) {
			m_closestTechniqueType_float[techIndex - 1] = dist;
			m_closestTechniqueType[techIndex - 1] = USHRT_MAX - (unsigned short)(dist);
		}
	}

	std::vector<Texture*>& textureList = s.item.blueprint->textures;

	s.distance = 0;
	if (textureList.size() > 0)
		s.textureIndex = s.item.blueprint->textures[0]->GetIndex();
	else
		s.textureIndex = 0;
	s.meshIndex = meshIndex;
	s.meshTypeDistance = m_closestMeshType_lastFrame[meshTechindex];
	s.techniqueIndex = techIndex;
	s.techniqueTypeDistance = m_closestTechniqueType_lastFrame[techIndex - 1];
	s.layer = UCHAR_MAX - layer;

	//s.sortingIndex = 0U;
	//s.meshIndex = meshIndex;
	//s.techniqueIndex = techIndex;

	m_items.push_back(s);
}

void D3D12ForwardRenderer::Frame(Window* w, Camera* c)
{
	D3D12Window* window = static_cast<D3D12Window*>(w);
	D3D12Camera* cam = static_cast<D3D12Camera*>(c);
	ID3D12GraphicsCommandList3* mainCommandList = m_commandLists[window->GetCurrentBackBufferIndex()][MAIN_COMMAND_INDEX];
	UINT backBufferIndex = window->GetCurrentBackBufferIndex();

	// Sort the objects to be rendered
	std::sort(m_items.begin(), m_items.end(),
		[](const SortingItem& a, const SortingItem& b) -> const bool
		{
			return a.sortingIndex > b.sortingIndex;
		});

	// Fill out the render information vectors
	SetUpRenderInstructions();


#ifdef MULTI_THREADED
	// How many instructions will each thread record
	// If any instructions would remain, each thread records an additional one
	unsigned numInstrPerThread = static_cast<unsigned>(m_renderInstructions.size()) / NUM_RECORDING_THREADS;
	numInstrPerThread += (m_renderInstructions.size() % NUM_RECORDING_THREADS ? 1U : 0U);

	//unsigned numInstrPerThread = (m_renderInstructions.size() / 2) / NUM_RECORDING_THREADS;
	//numInstrPerThread += ((m_renderInstructions.size() / 2) % NUM_RECORDING_THREADS ? 1U : 0U);

	ResetCommandListAndAllocator(backBufferIndex, MAIN_COMMAND_INDEX);

	for (int i = 0; i < NUM_RECORDING_THREADS; i++)
	{
		ResetCommandListAndAllocator(backBufferIndex, i + 1);

		SetThreadWork(
			i,
			window,
			cam,
			backBufferIndex,
			i * numInstrPerThread,
			numInstrPerThread
		);
	}

	{
		// Set the number of active worker threads
		std::unique_lock<std::mutex> lock(m_mutex);
		m_numActiveWorkerThreads = NUM_RECORDING_THREADS;
	}

	m_frames_recorded[0]++;

	{
		// Wake up the worker threads
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv_workers.notify_all();
	}

	// Map matrix data to GPU
	SetMatrixDataAndTextures(backBufferIndex);

	// Create a resource barrier transition from present to render target
	D3D12_RESOURCE_BARRIER barrierDesc = {};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = window->GetCurrentRenderTargetResource();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	// Main command list commands
	mainCommandList->ResourceBarrier(1, &barrierDesc);
	window->ClearRenderTarget(mainCommandList);

	// Wait until each worker thread has finished their recording
	{
		std::unique_lock<std::mutex> lock(m_mutex);
		while (m_numActiveWorkerThreads != 0) {
			//std::unique_lock<std::mutex> lock2(m_mutex_cv_main);
			m_cv_main.wait_for(lock, std::chrono::seconds(2), [this]() { return m_numActiveWorkerThreads == 0; });
		}
	}

#else
	for (int i = 0; i < NUM_RECORDING_THREADS; i++)
	{
		ResetCommandListAndAllocator(i);
	}
	// Map matrix data to GPU
	MapMatrixData(backBufferIndex);

	// Create a resource barrier transition from present to render target
	D3D12_RESOURCE_BARRIER barrierDesc = {};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = window->GetCurrentRenderTargetResource();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

	// Main command list commands
	mainCommandList->ResourceBarrier(1, &barrierDesc);
	window->ClearRenderTarget(mainCommandList);
	RecordRenderInstructions(window, cam, MAIN_COMMAND_INDEX, backBufferIndex, 0, m_renderInstructions.size());
#endif

}
void D3D12ForwardRenderer::Present(Window* w)
{
	D3D12Window* window = static_cast<D3D12Window*>(w);
	UINT backBufferIndex = window->GetCurrentBackBufferIndex();
	DXGI_PRESENT_PARAMETERS pp = {};
	//ID3D12CommandList* listsToExecute[1];
	ID3D12CommandList* listsToExecute[NUM_COMMAND_LISTS];

	D3D12_RESOURCE_BARRIER barrierDesc = {};
	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = window->GetCurrentRenderTargetResource();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;


	//m_commandLists[backBufferIndex][MAIN_COMMAND_INDEX]->ResourceBarrier(1, &barrierDesc);
	//m_commandLists[backBufferIndex][MAIN_COMMAND_INDEX]->Close();
	//listsToExecute[0] = m_commandLists[backBufferIndex][MAIN_COMMAND_INDEX];
	//window->GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	//m_fullScreenPass->Record(m_commandLists[backBufferIndex][NUM_COMMAND_LISTS - 1], window);
	// Set the barrier in the last command list
	m_commandLists[backBufferIndex][NUM_COMMAND_LISTS - 1]->ResourceBarrier(1, &barrierDesc);

	// Close the command lists
	for (int i = 0; i < NUM_COMMAND_LISTS; i++)
	{
		m_commandLists[backBufferIndex][i]->Close();
	}

	// Set the command lists
	for (int i = 0; i < NUM_COMMAND_LISTS; i++)
	{
		listsToExecute[i] = m_commandLists[backBufferIndex][i];
	}

	window->GetCommandQueue()->ExecuteCommandLists(ARRAYSIZE(listsToExecute), listsToExecute);

	//Wait for REnder before FXAA.

	//// Tell last frame to signal when done
	//const UINT64 fence_fxaa = m_FenceValue_fxaa;
	//window->GetCommandQueue()->Signal(m_Fence_fxaa, fence_fxaa);
	//m_FenceValue_fxaa++;

	////Wait until command queue is done.
	//if (m_Fence_fxaa->GetCompletedValue() < fence_fxaa)
	//{
	//	m_Fence_fxaa->SetEventOnCompletion(fence_fxaa, m_EventHandle_fxaa);
	//	WaitForSingleObject(m_EventHandle_fxaa, INFINITE);
	//}

	//Apply FXAA
	//m_FXAAPass->ApplyFXAA(window);


	//Present the frame.
	window->GetSwapChain()->Present1(0, 0, &pp);
	window->WaitForGPU();
}

ID3D12RootSignature* D3D12ForwardRenderer::GetRootSignature() const
{
	return m_rootSignature;
}

ID3D12DescriptorHeap* D3D12ForwardRenderer::GetDescriptorHeap() const
{
	return m_descriptorHeap;
}

//Private

bool D3D12ForwardRenderer::InitializeRootSignature()
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
	rootParams[0].Constants.Num32BitValues = 16 + 8;		// 1 * float4x4 + 2 * int
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

	rootSigDesc.Flags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;
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

	hr = m_d3d12->GetDevice()->CreateRootSignature(
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
bool D3D12ForwardRenderer::InitializeMatrixStructuredBuffer()
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
		hr = m_d3d12->GetDevice()->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&m_structuredBufferResources[i]));
		if (FAILED(hr))
			return false;
	}

	return true;
}
bool D3D12ForwardRenderer::InitializeTextureDescriptorHeap()
{
	HRESULT hr;
	D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};

	heapDescriptorDesc.NumDescriptors = NUM_DESCRIPTORS_IN_HEAP;
	heapDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	hr = m_d3d12->GetDevice()->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&m_descriptorHeap));
	if (FAILED(hr))
		return false;

	m_descriptorHeap->SetName(L"Big Shared Heap");
	/*for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		hr = m_device->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&m_descriptorHeap[i]));
		if (FAILED(hr))
			return false;
	}*/

	return true;
}

bool D3D12ForwardRenderer::InitializeCommandInterfaces()
{
	HRESULT hr;

	for (size_t backbufferIndex = 0; backbufferIndex < NUM_SWAP_BUFFERS; backbufferIndex++)
	{
		for (int i = 0; i < NUM_COMMAND_LISTS; i++)
		{
			hr = m_d3d12->GetDevice()->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&m_commandAllocators[backbufferIndex][i]));
			if (FAILED(hr))
			{
				return false;
			}

			//Create command list.
			hr = m_d3d12->GetDevice()->CreateCommandList(
				0,
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				m_commandAllocators[backbufferIndex][i],
				nullptr,
				IID_PPV_ARGS(&m_commandLists[backbufferIndex][i]));
			if (FAILED(hr))
			{
				return false;
			}

			//Command lists are created in the recording state. Since there is nothing to
			//record right now and the main loop expects it to be closed, we close it.
			m_commandLists[backbufferIndex][i]->Close();
		}
	}


	return true;
}

void D3D12ForwardRenderer::SetUpRenderInstructions()
{
	// Clear render instructions from previous frame
	m_renderInstructions.clear();
	m_instanceOffsets.clear();
	m_textureOffsets.clear();

	if (m_items.size() == 0)
		return;

	unsigned short meshID_last = m_items[0].meshIndex;
	unsigned short techID_last = m_items[0].techniqueIndex;
	unsigned short meshID_curr;
	unsigned short techID_curr;
	unsigned int nrOfInstances = 0;
	unsigned int nrOfTextures = 0;

	// Start with setting new technique
	m_renderInstructions.push_back(-1);

	// Setting a new technique does not increase the total instance count
	m_instanceOffsets.push_back(0);
	m_textureOffsets.push_back(0);

	// The first draw call has no instance offset
	m_instanceOffsets.push_back(0);
	m_textureOffsets.push_back(0);

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
			// Add up the total amount of textures in this draw call
			m_textureOffsets.push_back(m_textureOffsets.back() + nrOfTextures);

			// Add a technique swap instruction
			m_renderInstructions.push_back(-1);

			// Setting a new technique does not increase the total instance count
			m_instanceOffsets.push_back(m_instanceOffsets.back());
			// Setting a new technique does not increase the total texture count
			m_textureOffsets.push_back(m_textureOffsets.back());

			nrOfInstances = 0;
			nrOfTextures = 0;

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
			// Add up the total amount of textures in this draw call
			m_textureOffsets.push_back(m_textureOffsets.back() + nrOfTextures);

			nrOfInstances = 0;
			nrOfTextures = 0;
		}
		nrOfInstances++;
		nrOfTextures += m_items[i].item.blueprint->textures.size();
	}

	//Start with setting new technique
	m_renderInstructions.push_back(nrOfInstances);
}
void D3D12ForwardRenderer::ResetCommandListAndAllocator(int backbufferIndex, int index)
{
	HRESULT hr;
	hr = m_commandAllocators[backbufferIndex][index]->Reset();
	if (!SUCCEEDED(hr))
		printf("Error: Command allocator %d failed to reset\n", index);

	hr = m_commandLists[backbufferIndex][index]->Reset(m_commandAllocators[backbufferIndex][index], nullptr);
	if (!SUCCEEDED(hr))
		printf("Error: Command list %d failed to reset\n", index);
}
void D3D12ForwardRenderer::SetMatrixDataAndTextures(int backBufferIndex)
{
	// Heap information
	//D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_descriptorHeap[backBufferIndex]->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
	cdh.ptr += backBufferIndex * NUM_DESCRIPTORS_PER_SWAP_BUFFER * m_d3d12->GetViewSize();

	HRESULT hr;

	// Begin mapping matrix data
	D3D12_RANGE readRange = { 0, 0 };
	void* data = nullptr;

	hr = m_structuredBufferResources[backBufferIndex]->Map(0, &readRange, &data);
	if (FAILED(hr)) {

		_com_error err(hr);
		std::cout << err.ErrorMessage() << std::endl;

		hr = m_d3d12->GetDevice()->GetDeviceRemovedReason();
		_com_error err2(hr);
		std::cout << err2.ErrorMessage() << std::endl;

		return;
	}

	// Map matrix data to GPU
	// Copy descriptors to textures
	DirectX::XMMATRIX mat;
	size_t nItems = m_items.size();
	for (size_t item = 0; item < nItems; item++)
	{
		// Construct and copy matrix data
		Float3 pos = m_items[item].item.transform.pos;
		Float3 rot = m_items[item].item.transform.rotation;
		Float3 scal = m_items[item].item.transform.scale;

		mat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
		mat.r[3] = { pos.x, pos.y, pos.z, 1.0f };
		mat.r[0].m128_f32[0] *= scal.x;
		mat.r[1].m128_f32[1] *= scal.y;
		mat.r[2].m128_f32[2] *= scal.z;

		memcpy(static_cast<char*>(data) + sizeof(mat) * item, &mat, sizeof(mat));


		// Retrieve this mesh's textures
		std::vector<Texture*>& textures = m_items[item].item.blueprint->textures;
		size_t nTextures = textures.size();

		// Create a shader resource view for each texture
		for (size_t textureIndex = 0; textureIndex < nTextures; textureIndex++)
		{
			D3D12Texture* texture = static_cast<D3D12Texture*>(textures[textureIndex]);
			int textureIndexOnGPU = texture->IsLoaded() ? texture->GetTextureIndex() : 0;

			m_d3d12->GetDevice()->CopyDescriptorsSimple(1, cdh, m_d3d12->GetTextureLoader()->GetSpecificTextureCPUAdress(texture), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			// Move the pointer forward in memory
			cdh.ptr += m_d3d12->GetViewSize();
		}
	}

	// Finish mapping and write the matrix data
	D3D12_RANGE writeRange = { 0, sizeof(mat) * nItems };
	m_structuredBufferResources[backBufferIndex]->Unmap(0, &writeRange);
}
void D3D12ForwardRenderer::RecordRenderInstructions(D3D12Window* w, D3D12Camera* c, int commandListIndex, int backBufferIndex, size_t firstInstructionIndex, size_t numInstructions)
{
	if (firstInstructionIndex >= m_renderInstructions.size())
	{
		return;
	}

	ID3D12CommandAllocator* commandAllocator = m_commandAllocators[backBufferIndex][commandListIndex];
	ID3D12GraphicsCommandList3* commandList = m_commandLists[backBufferIndex][commandListIndex];

	DirectX::XMFLOAT4X4 viewPersp = c->GetViewPerspective();

	// Set everything which is necessary to record draw commands
	commandList->SetGraphicsRootSignature(m_rootSignature);
	//commandList->SetDescriptorHeaps(1, &m_descriptorHeap[backBufferIndex]);
	commandList->SetDescriptorHeaps(1, &m_descriptorHeap);
	commandList->SetGraphicsRootShaderResourceView(2, m_structuredBufferResources[backBufferIndex]->GetGPUVirtualAddress());
	commandList->SetGraphicsRoot32BitConstants(0, 16, &viewPersp, 0);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->RSSetViewports(1, w->GetViewport());
	commandList->RSSetScissorRects(1, w->GetScissorRect());
	w->SetRenderTarget(commandList);

	// Set a pipeline state unless the first instruction is to do so
	if (m_renderInstructions[firstInstructionIndex] != -1)
	{
		int instanceOffset = m_instanceOffsets[firstInstructionIndex];
		ID3D12PipelineState* pls = static_cast<D3D12Technique*>(m_items[instanceOffset].item.blueprint->technique)->GetPipelineState();
		commandList->SetPipelineState(pls);
	}

	// Determine the index of next list (m_renderInstructions.size() for the last list)
	size_t nextListFirstIndex = firstInstructionIndex + numInstructions;
	nextListFirstIndex = min(nextListFirstIndex, m_renderInstructions.size());

	for (size_t instructionIndex = firstInstructionIndex; instructionIndex < nextListFirstIndex; instructionIndex++)
	{
		int instruction = m_renderInstructions[instructionIndex];
		int instanceOffset = m_instanceOffsets[instructionIndex];
		int textureOffset = m_textureOffsets[instructionIndex];

		if (instruction == -1)
		{
			// Retrieve and enable a new technique from the first object using it
			ID3D12PipelineState* pls = static_cast<D3D12Technique*>(m_items[instanceOffset].item.blueprint->technique)->GetPipelineState();
			commandList->SetPipelineState(pls);
		}
		else
		{
			// Retrieve and enable a new mesh from the first object using it

			// Set instance offset
			commandList->SetGraphicsRoot32BitConstants(0, 1, &instanceOffset, 16);

			//D3D12_GPU_DESCRIPTOR_HANDLE handle = m_descriptorHeap[backBufferIndex]->GetGPUDescriptorHandleForHeapStart();
			//handle.ptr += instanceOffset * m_cbv_srv_uav_size;
			D3D12_GPU_DESCRIPTOR_HANDLE handle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
			handle.ptr += (backBufferIndex * NUM_DESCRIPTORS_PER_SWAP_BUFFER + textureOffset) * m_d3d12->GetViewSize();
			commandList->SetGraphicsRootDescriptorTable(1, handle);

			//Set Vertex Buffers
			std::vector<D3D12VertexBuffer*>& buffers = *static_cast<D3D12Mesh*>(m_items[instanceOffset].item.blueprint->mesh)->GetVertexBuffers();
			unsigned numBuffers = static_cast<unsigned>(buffers.size());
			for (unsigned j = 0; j < numBuffers; j++)
			{
				commandList->IASetVertexBuffers(j, 1, buffers[j]->GetView());
			}

			commandList->DrawInstanced(buffers[0]->GetNumberOfElements(), instruction, 0, 0);
		}
	}
}

void D3D12ForwardRenderer::RecordCommands(int threadIndex)
{
	{
		// Initial wait until the first work has been requested
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv_workers.wait(lock, [this]() { return m_numActiveWorkerThreads != 0; });
	}

	// A pointer to this thread's work
	RecordingThreadWork* work = &m_threadWork[threadIndex];

	while (m_isRunning)
	{
		// Record the commands for this list/thread
		RecordRenderInstructions(work->w, work->c, threadIndex + 1, work->backBufferIndex, work->firstInstructionIndex, work->numInstructions);

		{
			// Decrement the counter of the number of active threads
			std::unique_lock<std::mutex> lock(m_mutex);
			m_numActiveWorkerThreads--;
		}
		//
		m_frames_recorded[threadIndex + 1]++;
		{
			// Notify main thread of this one finishing its work
			// Main thread will go back to sleep if there are more than one active thread remaining
			std::unique_lock<std::mutex> lock(m_mutex);
			m_cv_main.notify_one();
		}
		{
			// Wait until main thread wakes this thread up
			// If this thread should wake up by itself, put it back to sleep unless main has reset the number of active threads
			std::unique_lock<std::mutex> lock(m_mutex);
			m_cv_workers.wait(lock, [this, threadIndex]() { return (m_frames_recorded[threadIndex + 1] < m_frames_recorded[0]) || !m_isRunning; });
		}
	}
}

void D3D12ForwardRenderer::SetThreadWork(int threadIndex, D3D12Window* w, D3D12Camera* c, int backBufferIndex, int firstInstructionIndex, int numInstructions)
{
	m_threadWork[threadIndex].w = w;
	m_threadWork[threadIndex].c = c;
	m_threadWork[threadIndex].backBufferIndex = backBufferIndex;
	m_threadWork[threadIndex].firstInstructionIndex = firstInstructionIndex;
	m_threadWork[threadIndex].numInstructions = numInstructions;
}