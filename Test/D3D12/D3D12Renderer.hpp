#pragma once

#include "../Renderer.hpp"
#include "GlobalSettings.hpp"
#include <thread>
#include <mutex>

struct ID3D12Device4;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList3;
struct ID3D12DescriptorHeap;
struct ID3D12RootSignature;
struct ID3D12Resource;

class D3D12VertexBuffer;
class D3D12TextureLoader;
class D3D12Window;
class D3D12Camera;

/*
	Documentation goes here ^^
*/
class D3D12Renderer : public Renderer {
public:
	
	D3D12Renderer();
	virtual ~D3D12Renderer();

	// Inherited via Renderer
	virtual bool Initialize() override;
	
	virtual Camera * MakeCamera() override;
	virtual Window * MakeWindow() override;
	virtual Texture * MakeTexture() override;
	virtual Mesh * MakeMesh() override;
	virtual Material * MakeMaterial() override;
	virtual RenderState * MakeRenderState() override;
	virtual Technique * MakeTechnique(RenderState* rs, ShaderProgram* sp, ShaderManager* sm) override;
	virtual ShaderManager * MakeShaderManager() override;
	virtual D3D12VertexBuffer * MakeVertexBuffer();

	virtual void ClearFrame() override;
	virtual void ClearSubmissions() override;
	virtual void Submit(SubmissionItem item) override;
	virtual void Frame(Window * window, Camera * c) override;
	virtual void Present(Window * w) override;

	ID3D12Device4* GetDevice() const;
	ID3D12RootSignature* GetRootSignature() const;
	ID3D12GraphicsCommandList3* GetCommandList() const;
	D3D12TextureLoader* GetTextureLoader() const;

private:
	struct SortingItem
	{
		unsigned int sortingIndex;
		unsigned short techniqueIndex;
		unsigned short meshIndex;
		SubmissionItem item;
	};

	bool InitializeDirect3DDevice();
	bool InitializeCommandInterfaces();
	bool InitializeRootSignature();
	bool InitializeMatrixStructuredBuffer();
	bool InitializeTextureDescriptorHeap();

	void SetUpRenderInstructions();
	void ResetCommandListAndAllocator(int index);
	void MapMatrixData(int backBufferIndex);
	void RecordRenderInstructions(D3D12Window* w, D3D12Camera* c, int commandListIndex, int backBufferIndex, int firstInstructionIndex, int numInstructions);

	void RecordCommands(int threadIndex);
	void SetThreadWork(int threadIndex, D3D12Window* w, D3D12Camera* c, int backBufferIndex, int firstInstructionIndex, int numInstructions);

	static const unsigned NUM_MATRICES_IN_BUFFER = 10240U;
	static const unsigned NUM_DESCRIPTORS_IN_HEAP = 100000U;
	static const unsigned NUM_COMMAND_LISTS = 1U + 2U;
	static const unsigned NUM_RECORDING_THREADS = NUM_COMMAND_LISTS - 1U;
	static const unsigned MAIN_COMMAND_INDEX = 0U;

	unsigned short m_meshesCreated = 0;
	unsigned short m_techniquesCreated = 0;

	unsigned int m_cbv_srv_uav_size;

	std::vector<SortingItem> m_items;
	D3D12TextureLoader* m_textureLoader;
	std::thread m_thread_texture;

	
	//-1 means new technique. everything else is number of instances to draw.
	std::vector<int> m_renderInstructions;
	std::vector<int> m_instanceOffsets;

	// Default resources
	ID3D12Device4*				m_device								= nullptr;
	ID3D12RootSignature*		m_rootSignature							= nullptr;
	ID3D12CommandAllocator*		m_commandAllocators[NUM_COMMAND_LISTS]	= { nullptr };
	ID3D12GraphicsCommandList3*	m_commandLists[NUM_COMMAND_LISTS]		= { nullptr };

	// Structured buffer for matrices
	ID3D12Resource*				m_structuredBufferResources[NUM_SWAP_BUFFERS] = { nullptr };
	
	// Descriptor heap for texture descriptors
	ID3D12DescriptorHeap*		m_descriptorHeap[NUM_SWAP_BUFFERS] = { nullptr };


	// Multithreaded recording resources
	struct RecordingThreadWork
	{
		D3D12Window* w;
		D3D12Camera* c;
		int commandListIndex;
		int backBufferIndex;
		int firstInstructionIndex;
		int numInstructions;
	};

	int m_numActiveWorkerThreads;
	unsigned __int64 m_frames_recorded[NUM_RECORDING_THREADS + 1] = {0};

	bool m_isRunning = true;
	std::condition_variable m_cv_main;
	std::condition_variable m_cv_workers;
	std::mutex m_mutex;
	std::thread m_recorderThreads[NUM_RECORDING_THREADS];
	RecordingThreadWork m_threadWork[NUM_RECORDING_THREADS];
};