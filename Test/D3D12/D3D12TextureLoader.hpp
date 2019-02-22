#pragma once
#include <vector>
#include <d3d12.h>

class D3D12Texture;
class D3D12Renderer;
//struct ID3D12CommandQueue;
//struct ID3D12CommandAllocator;
//struct ID3D12GraphicsCommandList3;
//struct D3D12_SUBRESOURCE_DATA;

//struct ID3D12Resource;

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

	//ID3D12DescriptorHeap* GetDescriptorHeap();
	unsigned GetNumberOfHeaps();
	ID3D12DescriptorHeap** GetAllHeaps();

private:
	void WaitForCopy();
	bool AddDescriptorHeap();

	//Temp Storage
	std::vector<D3D12Texture*> mTexturesToLoadToGPU;

	//Permanent Storage
	const unsigned MAX_SRVs_PER_DESCRIPTOR_HEAP = 10;
	unsigned mNrOfSRVs = 0;
	std::vector<ID3D12DescriptorHeap*> m_DescriptorHeaps;
	std::vector<ID3D12Resource*> m_TextureResources;

	unsigned mCBV_SRV_UAV_DescriptorSize;

	D3D12Renderer*				mRenderer;
	ID3D12CommandQueue*			mCommandQueue = nullptr;
	ID3D12CommandAllocator*		mCommandAllocator = nullptr;
	ID3D12GraphicsCommandList3*	mCommandList4 = nullptr;

	//ID3D12DescriptorHeap*	mDescriptorHeap = nullptr;

	//Fences for the render targets
	ID3D12Fence1*				mFence = nullptr;
	HANDLE						mEventHandle = nullptr;
	UINT64						mFenceValue = 0;
};