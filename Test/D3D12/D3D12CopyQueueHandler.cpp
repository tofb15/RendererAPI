#include "D3D12CopyQueueHandler.h"
#include "External/D3DX12/d3dx12.h"
#include "D3D12Renderer.hpp"
#include "D3D12Texture.hpp"

D3D12CopyQueueHandler::D3D12CopyQueueHandler(D3D12Renderer * renderer) : mRenderer(renderer)
{

}

D3D12CopyQueueHandler::~D3D12CopyQueueHandler()
{
}

bool D3D12CopyQueueHandler::Initialize()
{
	HRESULT hr;
	//Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	cqd.Type = D3D12_COMMAND_LIST_TYPE_COPY;

	hr = mRenderer->GetDevice()->CreateCommandQueue(&cqd, IID_PPV_ARGS(&mCommandQueue));
	if (FAILED(hr))
	{
		return false;
	}

	//Create command allocator. The command allocator object corresponds
	//to the underlying allocations in which GPU commands are stored.
	hr = mRenderer->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&mCommandAllocator));
	if (FAILED(hr))
	{
		return false;
	}

	//Create command list.
	hr = mRenderer->GetDevice()->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COPY,
		mCommandAllocator,
		nullptr,
		IID_PPV_ARGS(&mCommandList4));
	if (FAILED(hr))
	{
		return false;
	}

	hr;
	hr = mRenderer->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence));
	if (FAILED(hr))
	{
		return false;
	}

	mFenceValue = 1;
	//Create an event handle to use for GPU synchronization.
	mEventHandle = CreateEvent(0, false, false, 0);

	return true;
}

void D3D12CopyQueueHandler::DoWork()
{

	//while (true)
	//{
		while (mTextures.size() > 0)
		{
			D3D12Texture* texture = mTextures[0];

#pragma region INITIALIZE_DEFAULT_HEAP

			D3D12_HEAP_PROPERTIES heapProperties = {};
			heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;			//D3D12_HEAP_TYPE_UPLOAD did not work here!
			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.CreationNodeMask = 1; //used when multi-gpu
			heapProperties.VisibleNodeMask = 1; //used when multi-gpu
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			resourceDesc.Alignment = 0;//D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;	//4KB if the texture is <  64KB; 64KB if the texture is < 4MB; else bigger (see flags)
			resourceDesc.Width = texture->mWidth;
			resourceDesc.Height = texture->mHeight;
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 1;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;		//Row major did not work here!

			HRESULT hr = mRenderer->GetDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_COMMON,
				nullptr,
				IID_PPV_ARGS(&texture->mResource)
			);
			if (FAILED(hr))
			{
				return;
			}

#pragma endregion

			//GetRequiredIntermediateSize returns w * h * "sizeof"(Format). Example: 640(width) * 640(height) * 4(Bytes) * 4(Channels)
			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(texture->mResource, 0, 1) + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

#pragma region INITIALIZE_UPLOAD_HEAP
			//Reusing heap-prop
			heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

			//Temporary resource for uploading image (for some reason)
			ID3D12Resource* uploadResource;

			/*
			FOLLOW	-> https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/Samples/Desktop/D3D12SmallResources/src/D3D12SmallResources.cpp
			INFO	-> https://docs.microsoft.com/en-us/windows/desktop/direct3d12/cd3dx12-resource-desc
			INFO	-> https://github.com/Microsoft/DirectX-Graphics-Samples/blob/master/Libraries/D3DX12/d3dx12.h
			*/
			D3D12_RESOURCE_DESC uploadResourceDesc = {};
			uploadResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			uploadResourceDesc.Format = DXGI_FORMAT_UNKNOWN;
			uploadResourceDesc.Alignment = 0;
			uploadResourceDesc.Width = uploadBufferSize;
			uploadResourceDesc.Height = 1;
			uploadResourceDesc.DepthOrArraySize = 1;
			uploadResourceDesc.MipLevels = 1;
			uploadResourceDesc.SampleDesc.Count = 1;
			uploadResourceDesc.SampleDesc.Quality = 0;
			uploadResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			uploadResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

			hr = mRenderer->GetDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&uploadResourceDesc,
				D3D12_RESOURCE_STATE_GENERIC_READ,
				nullptr,
				IID_PPV_ARGS(&uploadResource));
			if (FAILED(hr))
			{
				return;
			}

			D3D12_SUBRESOURCE_DATA textureData = {};
			textureData.pData = reinterpret_cast<void*>(texture->mImage_CPU.data());//;reinterpret_cast<UINT8*>(rgb);
			textureData.RowPitch = texture->mWidth * texture->mBytesPerPixel;
			textureData.SlicePitch = textureData.RowPitch * texture->mHeight;

			UpdateSubresources<1>(mCommandList4, texture->mResource, uploadResource, 0, 0, 1, &textureData);
#pragma endregion

	////// Describe and create a SRV for the texture.
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = resourceDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;

	//for (size_t i = 0; i < m->NUM_SWAP_BUFFERS; i++)
	//{
	//	D3D12_CPU_DESCRIPTOR_HANDLE cdh = mRenderer->mDescriptorHeap[i]->GetCPUDescriptorHandleForHeapStart();
	//	//cdh.ptr += renderer->mCBV_SRV_UAV_DescriptorSize;
	//	renderer->getDevice()->CreateShaderResourceView(resource, &srvDesc, cdh);
	//}

	mCommandList4->Close();
	ID3D12CommandList* ppCommandLists[] = { mCommandList4 };
	mCommandQueue->ExecuteCommandLists(1, ppCommandLists);
		
	WaitForCopy();
	mTextures.erase(mTextures.begin());
		}
	//}
}

void D3D12CopyQueueHandler::LoadTextureToGPU(D3D12Texture * texture)
{
	mTextures.push_back(texture);
}

void D3D12CopyQueueHandler::WaitForCopy()
{
	const UINT64 fence = mFenceValue;
	mCommandQueue->Signal(mFence, fence);
	mFenceValue++;

	//Wait until command queue is done.
	if (mFence->GetCompletedValue() < fence)
	{
		mFence->SetEventOnCompletion(fence, mEventHandle);
		WaitForSingleObject(mEventHandle, INFINITE);
	}
}
