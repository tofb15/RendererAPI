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

class D3D12TextureLoader
{
public:

	D3D12TextureLoader(D3D12Renderer* renderer);
	~D3D12TextureLoader();

	bool Initialize();
	void DoWork();
	void LoadTextureToGPU(D3D12Texture* texture);
	ID3D12DescriptorHeap* GetDescriptorHeap();
private:
	void WaitForCopy();

	std::vector<D3D12Texture*> mTextures;

	D3D12Renderer* mRenderer;
	ID3D12CommandQueue*	mCommandQueue = nullptr;
	ID3D12CommandAllocator*		mCommandAllocator = nullptr;
	ID3D12GraphicsCommandList3*	mCommandList4 = nullptr;

	ID3D12DescriptorHeap*	mDescriptorHeap = nullptr;

	//Fences for the render targets
	ID3D12Fence1*				mFence = nullptr;
	HANDLE						mEventHandle = nullptr;
	UINT64						mFenceValue = 0;
};