#pragma once
#include "D3D12Renderer.h"
#include <vector>
#include <thread>
#include <mutex>

class D3D12Window;
class D3D12Camera;

class D3D12ForwardRenderer : public D3D12Renderer
{
public:
	static constexpr unsigned NUM_COMMAND_LISTS = 1U + 2U;
	static constexpr unsigned NUM_RECORDING_THREADS = NUM_COMMAND_LISTS - 1U;
	static constexpr unsigned MAIN_COMMAND_INDEX = 0U;
	static constexpr unsigned NUM_MATRICES_IN_BUFFER = 10240U * 2U;
	static constexpr unsigned NUM_DESCRIPTORS_PER_SWAP_BUFFER = NUM_MATRICES_IN_BUFFER;
	static constexpr unsigned NUM_DESCRIPTORS_IN_HEAP = NUM_SWAP_BUFFERS * NUM_DESCRIPTORS_PER_SWAP_BUFFER + NUM_SWAP_BUFFERS;

public:
	D3D12ForwardRenderer(D3D12API* d3d12);
	~D3D12ForwardRenderer();

	virtual bool Initialize() override;

	virtual void ClearFrame() override;
	virtual void ClearSubmissions() override;
	virtual void Submit(SubmissionItem item, Camera* c = nullptr, unsigned char layer = 0) override;

	virtual void Frame(Window* window, Camera* c) override;
	virtual void Present(Window* w) override;

	ID3D12DescriptorHeap* GetDescriptorHeap() const;

private:
	struct SortingItem
	{
		SortingItem() {

		}
		SortingItem(const SortingItem& other) {
			sortingIndex = other.sortingIndex;
			item = other.item;
		}
		SortingItem& operator=(const SortingItem& other)
		{
			sortingIndex = other.sortingIndex;
			item = other.item;
			return *this;
		}

		union {
			UINT128 sortingIndex;						//16 Bytes
			struct {
				//By setting meshIndex and techniqueIndex, sortingIndex will be set automatically.
				unsigned short distance;				//2 Bytes
				unsigned short textureIndex;			//2 Bytes
				unsigned short meshIndex;				//2 Bytes
				unsigned short meshTypeDistance;		//2 Bytes, closest element in the mesh
				unsigned short techniqueIndex;			//2 Bytes
				unsigned short techniqueTypeDistance;	//2 Bytes, closest element in the technique
				unsigned char layer;
			};

		};
		//unsigned short techniqueIndex;
		//unsigned short meshIndex;
		SubmissionItem item;
	};

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

private:
	bool InitializeMatrixStructuredBuffer();
	bool InitializeTextureDescriptorHeap();
	bool InitializeCommandInterfaces();

	void SetUpRenderInstructions();
	void ResetCommandListAndAllocator(int backbufferIndex, int index);
	void SetMatrixDataAndTextures(int backBufferIndex);
	void RecordRenderInstructions(D3D12Window* w, D3D12Camera* c, int commandListIndex, int backBufferIndex, size_t firstInstructionIndex, size_t numInstructions);
	void RecordCommands(int threadIndex);
	void SetThreadWork(int threadIndex, D3D12Window* w, D3D12Camera* c, int backBufferIndex, int firstInstructionIndex, int numInstructions);

private:
	std::vector<SortingItem> m_items;

	unsigned short m_closestMeshType_float[100];
	unsigned short m_closestMeshType[100];
	unsigned short m_closestMeshType_lastFrame[100];

	unsigned short m_closestTechniqueType_float[100];
	unsigned short m_closestTechniqueType[100];
	unsigned short m_closestTechniqueType_lastFrame[100];

	ID3D12CommandAllocator* m_commandAllocators[NUM_SWAP_BUFFERS][NUM_COMMAND_LISTS] = { nullptr };
	ID3D12GraphicsCommandList3* m_commandLists[NUM_SWAP_BUFFERS][NUM_COMMAND_LISTS] = { nullptr };

	ID3D12Resource* m_structuredBufferResources[NUM_SWAP_BUFFERS] = { nullptr };
	ID3D12DescriptorHeap* m_descriptorHeap = nullptr;

	int m_numActiveWorkerThreads;
	unsigned __int64 m_frames_recorded[NUM_RECORDING_THREADS + 1] = { 0 };

	std::condition_variable m_cv_main;
	std::condition_variable m_cv_workers;
	std::mutex m_mutex;
	std::thread m_recorderThreads[NUM_RECORDING_THREADS];
	RecordingThreadWork m_threadWork[NUM_RECORDING_THREADS];

	//-1 means new technique. everything else is number of instances to draw.
	std::vector<int> m_renderInstructions;
	std::vector<int> m_instanceOffsets;
	std::vector<int> m_textureOffsets;

	bool m_isRunning = true;

};