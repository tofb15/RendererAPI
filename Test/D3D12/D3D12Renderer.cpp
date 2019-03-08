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
#include <iostream>
#include <comdef.h>

#include <algorithm>
#include <functional>

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

D3D12Renderer::D3D12Renderer()
{
}
D3D12Renderer::~D3D12Renderer()
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

	m_textureLoader->Kill();	//Notify the other thread to stop.
	m_thread_texture.join();	//Wait for the other thread to stop.
	if (m_textureLoader)
		delete m_textureLoader;

	// These are safe to release as long as each window has been deleted first,
	// since the windows wait for their queues to finish all their work

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		for (int j = 0; j < NUM_COMMAND_LISTS; j++)
		{
			m_commandLists[i][j]->Release();
			m_commandAllocators[i][j]->Release();
		}
		m_descriptorHeap[i]->Release();
		m_structuredBufferResources[i]->Release();
	}
	m_rootSignature->Release();
	m_device->Release();
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

	m_numActiveWorkerThreads = 0;
#ifdef MULTI_THREADED
	for (int i = 0; i < NUM_RECORDING_THREADS; i++)
	{
		m_recorderThreads[i] = std::thread(&D3D12Renderer::RecordCommands, this, i);
		std::cout << std::hex << m_recorderThreads[i].get_id() << std::dec << std::endl;
	}
#endif

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
	if (++m_texturesCreated == 0)
		return nullptr;

	return new D3D12Texture(this, m_texturesCreated);
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

	int max = m_techniquesCreated * m_meshesCreated;
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

void D3D12Renderer::Submit(SubmissionItem item, Camera* c)
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

	Float3 f = item.transform.pos - c->GetPosition();

	unsigned int dist = f.length() * 65;
	s.distance = UINT_MAX - min(dist, UINT_MAX);
	int meshTechindex = (techIndex-1) * m_meshesCreated + (meshIndex-1);

	if (dist < m_closestMeshType_float[meshTechindex]) {
		m_closestMeshType_float[meshTechindex] = dist;
		m_closestMeshType[meshTechindex] = USHRT_MAX - (unsigned short)(dist);
	}

	if (dist < m_closestTechniqueType_float[techIndex-1]) {
		m_closestTechniqueType_float[techIndex-1] = dist;
		m_closestTechniqueType[techIndex-1] = USHRT_MAX - (unsigned short)(dist);
	}

	s.distance = 0;
	s.textureIndex = s.item.blueprint->textures[0]->GetIndex();
	s.meshIndex = meshIndex;
	s.meshTypeDistance = m_closestMeshType_lastFrame[meshTechindex];
	s.techniqueIndex = techIndex;
	s.techniqueTypeDistance = m_closestTechniqueType_lastFrame[techIndex-1];

	//s.sortingIndex = 0U;
	//s.meshIndex = meshIndex;
	//s.techniqueIndex = techIndex;

	m_items.push_back(s);
}
void D3D12Renderer::Frame(Window* w, Camera* c)
{
	D3D12Window* window = static_cast<D3D12Window*>(w);
	D3D12Camera* cam = static_cast<D3D12Camera*>(c);
	ID3D12GraphicsCommandList3* mainCommandList = m_commandLists[window->GetCurrentBackBufferIndex()][MAIN_COMMAND_INDEX];

	// Sort the objects to be rendered
	std::sort(m_items.begin(), m_items.end(),
		[](const SortingItem & a, const SortingItem & b) -> const bool
	{
		return a.sortingIndex > b.sortingIndex;
	});
//<<<<<<< HEAD
	
	//UINT backBufferIndex = window->GetCurrentBackBufferIndex();

	//HRESULT hr = m_commandAllocator->Reset();
	//if (!SUCCEEDED(hr)) printf("Error: Command allocator reset\n");

	//hr = m_commandList->Reset(m_commandAllocator, nullptr);
	//if (!SUCCEEDED(hr)) printf("Error: Command list reset\n");

	//// Heap information
	//D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_descriptorHeap[backBufferIndex]->GetCPUDescriptorHandleForHeapStart();

	//// Description (maybe fix another solution)
	//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	//srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	//srvDesc.Texture2D.MipLevels = 1;

	//m_commandList->SetDescriptorHeaps(1, &m_descriptorHeap[backBufferIndex]);

	////Set root signature
	//m_commandList->SetGraphicsRootSignature(m_rootSignature);
	//m_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
//=======
//
	UINT backBufferIndex = window->GetCurrentBackBufferIndex();
//>>>>>>> MultithreadedCommandRecording

	// Fill out the render information vectors
	SetUpRenderInstructions();

//<<<<<<< HEAD
//	// Begin mapping matrix data
//	D3D12_RANGE readRange = { 0, 0 };
//	void* data = nullptr;
//
//	hr = m_structuredBufferResources[backBufferIndex]->Map(0, &readRange, &data);
//	int i = 0;
//	if (FAILED(hr)) {
//		_com_error err(hr);
//		std::cout << err.ErrorMessage() << std::endl;
//	}
//
//	// Create shader resource views for each texture on each object
//	// Copy matrix data to GPU
//	DirectX::XMMATRIX mat;
//	size_t nItems = m_items.size();
//	for (int item = 0; item < nItems; item++)
//	{
//		// Retrieve this mesh's textures
//		std::vector<Texture*>& textures = m_items[item].item.blueprint->textures;
//		size_t nTextures = textures.size();
//=======
//>>>>>>> MultithreadedCommandRecording


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
		ResetCommandListAndAllocator(backBufferIndex, i+1);
		
//<<<<<<< HEAD
//		Float3 pos = m_items[item].item.transform.pos;
//		Float3 rot = m_items[item].item.transform.rotation;
//		Float3 scal = m_items[item].item.transform.scale;
//=======
		// Set the data each thread will process
		SetThreadWork(
			i,
			window,
			cam,
			backBufferIndex,
			i * numInstrPerThread,
			numInstrPerThread
		);
	}
//>>>>>>> MultithreadedCommandRecording

	{
		// Set the number of active worker threads
		std::unique_lock<std::mutex> lock(m_mutex);
		m_numActiveWorkerThreads = NUM_RECORDING_THREADS;
	}

	m_frames_recorded[0]++;

//<<<<<<< HEAD
//		mat.r[0].m128_f32[0] *= scal.x;
//		mat.r[1].m128_f32[1] *= scal.y;
//		mat.r[2].m128_f32[2] *= scal.z;
//
//		mat = DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z), mat);
//
//		memcpy(static_cast<char*>(data) + sizeof(mat) * item, &mat, sizeof(mat));
//=======
	{
		// Wake up the worker threads
		std::unique_lock<std::mutex> lock(m_mutex);
		m_cv_workers.notify_all();
//>>>>>>> MultithreadedCommandRecording
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
void D3D12Renderer::Present(Window * w)
{
	D3D12Window* window = static_cast<D3D12Window*>(w);
	UINT backBufferIndex = window->GetCurrentBackBufferIndex();
	D3D12_RESOURCE_BARRIER barrierDesc = {};
	DXGI_PRESENT_PARAMETERS pp = {};
	ID3D12CommandList* listsToExecute[NUM_COMMAND_LISTS];

	barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrierDesc.Transition.pResource = window->GetCurrentRenderTargetResource();
	barrierDesc.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	barrierDesc.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	barrierDesc.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

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

	//Present the frame.
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
void D3D12Renderer::ResetCommandListAndAllocator(int backbufferIndex, int index)
{
	HRESULT hr;
	hr = m_commandAllocators[backbufferIndex][index]->Reset();
	if (!SUCCEEDED(hr))
		printf("Error: Command allocator %d failed to reset\n", index);

	hr = m_commandLists[backbufferIndex][index]->Reset(m_commandAllocators[backbufferIndex][index], nullptr);
	if (!SUCCEEDED(hr))
		printf("Error: Command list %d failed to reset\n", index);
}
void D3D12Renderer::MapMatrixData(int backBufferIndex)
{
	// Heap information
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_descriptorHeap[backBufferIndex]->GetCPUDescriptorHandleForHeapStart();
	HRESULT hr;

	// Begin mapping matrix data
	D3D12_RANGE readRange = { 0, 0 };
	void* data = nullptr;

	//static int count = 0;

	//if (count++ < 10) {
	//	
	//}
	//else {
	//	return;
	//}

	hr = m_structuredBufferResources[backBufferIndex]->Map(0, &readRange, &data);
	if (FAILED(hr)) {
		
		_com_error err(hr);
		std::cout << err.ErrorMessage() << std::endl;

		hr = m_device->GetDeviceRemovedReason();
		_com_error err2(hr);
		std::cout << err2.ErrorMessage() << std::endl;

		return;
	}
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
		Float3 rot = m_items[item].item.transform.rotation;
		Float3 scal = m_items[item].item.transform.scale;

		mat = DirectX::XMMatrixIdentity();

		mat.r[3] = { pos.x, pos.y, pos.z, 1.0f };

		mat.r[0].m128_f32[0] *= scal.x;
		mat.r[1].m128_f32[1] *= scal.y;
		mat.r[2].m128_f32[2] *= scal.z;

		mat = DirectX::XMMatrixMultiply(DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z), mat);

		memcpy(static_cast<char*>(data) + sizeof(mat) * item, &mat, sizeof(mat));
	}

	// Finish mapping and write the matrix data
	D3D12_RANGE writeRange = { 0, sizeof(mat) * nItems };
	m_structuredBufferResources[backBufferIndex]->Unmap(0, &writeRange);
}
void D3D12Renderer::RecordRenderInstructions(D3D12Window* w, D3D12Camera* c, int commandListIndex, int backBufferIndex, size_t firstInstructionIndex, size_t numInstructions)
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
	commandList->SetDescriptorHeaps(1, &m_descriptorHeap[backBufferIndex]);
	commandList->SetGraphicsRootShaderResourceView(2, m_structuredBufferResources[backBufferIndex]->GetGPUVirtualAddress());
	commandList->SetGraphicsRoot32BitConstants(0, 16, &viewPersp, 0);
	commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	commandList->RSSetViewports(1, w->GetViewport());
	commandList->RSSetScissorRects(1, w->GetScissorRect());
	w->SetRenderTarget(commandList);

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
		// Retrieve instruction
		int instruction;
		{
			//std::unique_lock<std::mutex> lock(m_mutex_instr);
			instruction = m_renderInstructions[instructionIndex];
		}

		// Retrieve the offset in objects for this instruction
		int instanceOffset;
		{
			//std::unique_lock<std::mutex> lock(m_mutex_offset);
			instanceOffset = m_instanceOffsets[instructionIndex];
		}
		
		if (instruction == -1)
		{
			// Retrieve and enable a new technique from the first object using it
			{
				//std::unique_lock<std::mutex> lock(m_mutex_item);
				ID3D12PipelineState* pls = static_cast<D3D12Technique*>(m_items[instanceOffset].item.blueprint->technique)->GetPipelineState();
				commandList->SetPipelineState(pls);
			}
		}
		else
		{
			// Retrieve and enable a new mesh from the first object using it

			// Set instance offset
			commandList->SetGraphicsRoot32BitConstants(0, 1, &instanceOffset, 16);

			D3D12_GPU_DESCRIPTOR_HANDLE handle = m_descriptorHeap[backBufferIndex]->GetGPUDescriptorHandleForHeapStart();
			handle.ptr += instanceOffset * m_cbv_srv_uav_size;
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

void D3D12Renderer::RecordCommands(int threadIndex)
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
			m_cv_workers.wait(lock, [this, threadIndex]() { return (m_frames_recorded[threadIndex + 1] < m_frames_recorded[0]/*m_numActiveWorkerThreads == NUM_RECORDING_THREADS*/) || !m_isRunning; });
		}
	}
}

void D3D12Renderer::SetThreadWork(int threadIndex, D3D12Window * w, D3D12Camera * c, int backBufferIndex, int firstInstructionIndex, int numInstructions)
{
	m_threadWork[threadIndex].w = w;
	m_threadWork[threadIndex].c = c;
	m_threadWork[threadIndex].backBufferIndex = backBufferIndex;
	m_threadWork[threadIndex].firstInstructionIndex = firstInstructionIndex;
	m_threadWork[threadIndex].numInstructions = numInstructions;
}

ID3D12Device4 * D3D12Renderer::GetDevice() const
{
	return m_device;
}
ID3D12RootSignature * D3D12Renderer::GetRootSignature() const
{
	return m_rootSignature;
}
D3D12TextureLoader * D3D12Renderer::GetTextureLoader() const
{
	return m_textureLoader;
}

bool D3D12Renderer::InitializeDirect3DDevice()
{
#ifndef _DEBUG
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

	for (size_t backbufferIndex = 0; backbufferIndex < NUM_SWAP_BUFFERS; backbufferIndex++)
	{
		for (int i = 0; i < NUM_COMMAND_LISTS; i++)
		{
			hr = m_device->CreateCommandAllocator(
				D3D12_COMMAND_LIST_TYPE_DIRECT,
				IID_PPV_ARGS(&m_commandAllocators[backbufferIndex][i]));
			if (FAILED(hr))
			{
				return false;
			}

			//Create command list.
			hr = m_device->CreateCommandList(
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