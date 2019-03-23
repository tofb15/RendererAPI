#pragma once
#include <vector>
#include <d3d12.h>
#include <thread>
#include <mutex>

class D3D12Texture;
class D3D12Renderer;
struct ID3D12Resource;

#include <vector>

class D3D12TextureLoader
{
public:

	D3D12TextureLoader(D3D12Renderer* renderer);
	~D3D12TextureLoader();

	bool Initialize();

	void DoWork();
	void LoadTextureToGPU(D3D12Texture* texture);
	D3D12_GPU_DESCRIPTOR_HANDLE GetSpecificTextureGPUAdress(D3D12Texture* texture);
	D3D12_CPU_DESCRIPTOR_HANDLE GetSpecificTextureCPUAdress(D3D12Texture* texture);
	ID3D12Resource* GetResource(int index);

	//ID3D12DescriptorHeap* GetDescriptorHeap();
	unsigned GetNumberOfHeaps();
	ID3D12DescriptorHeap** GetAllHeaps();

	void SynchronizeWork();
	void Kill();
private:
	void SignalAndWaitForCopy();
	void WaitForCopy(UINT64 fence);
	bool AddDescriptorHeap();

	std::mutex m_mutex_TextureResources;
	std::condition_variable m_cv_not_empty;	//Notify that there is work to be done.
	std::condition_variable m_cv_empty;		//Notify that there is no work to be done.
	bool m_stop = false;
	bool m_atLeastOneTextureIsLoaded = false;

	//Temp Storage
	std::vector<D3D12Texture*> m_texturesToLoadToGPU;

	//Permanent Storage
	const unsigned MAX_SRVs_PER_DESCRIPTOR_HEAP = 100;
	unsigned m_nrOfTextures = 0;
	std::vector<ID3D12DescriptorHeap*> m_descriptorHeaps;
	std::vector<ID3D12Resource*> m_textureResources;

	unsigned m_CBV_SRV_UAV_DescriptorSize;

	D3D12Renderer*				m_renderer;
	ID3D12CommandQueue*			m_commandQueue = nullptr;
	ID3D12CommandAllocator*		m_commandAllocator = nullptr;
	ID3D12GraphicsCommandList3*	m_commandList = nullptr;

	//Fences for the render targets
	ID3D12Fence1*				m_fence = nullptr;
	HANDLE						m_eventHandle = nullptr;
	UINT64						m_fenceValue = 0;

	int m_queueTimingIndex;
	int m_CPURecordTimingIndex;
};