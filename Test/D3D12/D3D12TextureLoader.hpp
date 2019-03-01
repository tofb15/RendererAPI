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
	std::mutex m_mutex_TextureResources;
	std::condition_variable m_cv_not_empty;	//Notify that there is work to be done.
	std::condition_variable m_cv_empty;		//Notify that there is no work to be done.
	bool stop = false;
	bool atLeastOneTextureIsLoaded = false;

	void WaitForCopy();
	bool AddDescriptorHeap();

	//Temp Storage
	std::vector<D3D12Texture*> mTexturesToLoadToGPU;

	//Permanent Storage
	const unsigned MAX_SRVs_PER_DESCRIPTOR_HEAP = 100;
	unsigned m_nrOfTextures = 0;
	std::vector<ID3D12DescriptorHeap*> m_DescriptorHeaps;
	std::vector<ID3D12Resource*> m_TextureResources;

	unsigned mCBV_SRV_UAV_DescriptorSize;

	D3D12Renderer*				m_Renderer;
	ID3D12CommandQueue*			m_CommandQueue = nullptr;
	ID3D12CommandAllocator*		m_CommandAllocator = nullptr;
	ID3D12GraphicsCommandList3*	m_CommandList4 = nullptr;

	//ID3D12DescriptorHeap*	mDescriptorHeap = nullptr;

	//Fences for the render targets
	ID3D12Fence1*				m_Fence = nullptr;
	HANDLE						m_EventHandle = nullptr;
	UINT64						m_FenceValue = 0;
};