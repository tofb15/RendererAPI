#include "D3D12TextureLoader.hpp"
#include "External/D3DX12/d3dx12.h"
#include "D3D12Renderer.hpp"
#include "D3D12Texture.hpp"
#include <chrono>

D3D12TextureLoader::D3D12TextureLoader(D3D12Renderer * renderer) : m_Renderer(renderer)
{

}

D3D12TextureLoader::~D3D12TextureLoader()
{
	for (auto heap : m_DescriptorHeaps)
	{
		heap->Release();
	}

	for (auto resources : m_TextureResources)
	{
		resources->Release();
	}
}

bool D3D12TextureLoader::Initialize()
{
	mCBV_SRV_UAV_DescriptorSize = m_Renderer->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	HRESULT hr;
	//Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	cqd.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	hr = m_Renderer->GetDevice()->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_CommandQueue));
	if (FAILED(hr))
	{
		return false;
	}

	//Create command allocator. The command allocator object corresponds
	//to the underlying allocations in which GPU commands are stored.
	hr = m_Renderer->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_CommandAllocator));
	if (FAILED(hr))
	{
		return false;
	}

	//Create command list.
	hr = m_Renderer->GetDevice()->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COPY,
		m_CommandAllocator,
		nullptr,
		IID_PPV_ARGS(&m_CommandList4));
	if (FAILED(hr))
	{
		return false;
	}

	m_CommandList4->Close();

	hr = m_Renderer->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence));
	if (FAILED(hr))
	{
		return false;
	}

	m_FenceValue = 1;
	//Create an event handle to use for GPU synchronization.
	m_EventHandle = CreateEvent(0, false, false, 0);



	return true;
}

void D3D12TextureLoader::DoWork()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
		m_cv_not_empty.wait(lock, [this]() {return !mTexturesToLoadToGPU.empty(); });//Force the thread to sleep if there is no texture to load. Mutex will be unlocked aslong as the thread is sleeping.
	}

	while (!stop)
	{
		if(atLeastOneTextureIsLoaded)
			std::this_thread::sleep_for(std::chrono::seconds(5));
		
		
		D3D12Texture* texture;
		//Critical Region.
		{
			std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
			texture = mTexturesToLoadToGPU[0];
			mTexturesToLoadToGPU.erase(mTexturesToLoadToGPU.begin());
		}



		m_CommandAllocator->Reset();
		m_CommandList4->Reset(m_CommandAllocator, nullptr);
		ID3D12Resource* textureResource;

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

		HRESULT hr = m_Renderer->GetDevice()->CreateCommittedResource(
			&heapProperties,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&textureResource)
		);
		if (FAILED(hr))
		{
			return;
		}

#pragma endregion

		//GetRequiredIntermediateSize returns w * h * "sizeof"(Format). Example: 640(width) * 640(height) * 4(Bytes) * 4(Channels)
		const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureResource, 0, 1) + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

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

		hr = m_Renderer->GetDevice()->CreateCommittedResource(
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

		UpdateSubresources<1>(m_CommandList4, textureResource, uploadResource, 0, 0, 1, &textureData);
#pragma endregion

		////// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = resourceDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;

		//if max SRV capacity is reached, Allocate a new DescriptorHeap. 
		unsigned localHeapIndex = mNrOfSRVs % MAX_SRVs_PER_DESCRIPTOR_HEAP;
		if (localHeapIndex == 0) {
			AddDescriptorHeap();
		}


		D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_DescriptorHeaps.back()->GetCPUDescriptorHandleForHeapStart();
		cdh.ptr += localHeapIndex * mCBV_SRV_UAV_DescriptorSize;
		m_Renderer->GetDevice()->CreateShaderResourceView(textureResource, &srvDesc, cdh);


		m_CommandList4->Close();
		ID3D12CommandList* ppCommandLists[] = { m_CommandList4 };
		m_CommandQueue->ExecuteCommandLists(1, ppCommandLists);

		WaitForCopy();
		texture->mGPU_Loader_index = mNrOfSRVs++;
		m_TextureResources.push_back(textureResource);
		{
			std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
			atLeastOneTextureIsLoaded = true;
			if (mTexturesToLoadToGPU.empty()) {
				m_cv_empty.notify_all();//Only used for D3D12TextureLoader::SynchronizeWork() to wake waiting threads
				m_cv_not_empty.wait(lock, [this]() {return !mTexturesToLoadToGPU.empty() || stop; });//Force the thread to sleep if there is no texture to load. Mutex will be unlocked aslong as the thread is sleeping.
			}
		}
	}
}

void D3D12TextureLoader::LoadTextureToGPU(D3D12Texture * texture)
{
	std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
	mTexturesToLoadToGPU.push_back(texture);
	m_cv_not_empty.notify_one();//Wake up the loader thread
	if (!atLeastOneTextureIsLoaded) {
		lock.unlock();
		SynchronizeWork();
	}
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12TextureLoader::GetSpecificTextureGPUAdress(D3D12Texture * texture)
{
	int index = texture->mGPU_Loader_index;
	if (index == -1)
		index = 0;

	int HeapIndex = index / MAX_SRVs_PER_DESCRIPTOR_HEAP;
	int localIndex = index % MAX_SRVs_PER_DESCRIPTOR_HEAP;

	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_DescriptorHeaps[HeapIndex]->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += localIndex * mCBV_SRV_UAV_DescriptorSize;

	return handle;
}

unsigned D3D12TextureLoader::GetNumberOfHeaps()
{
	return m_DescriptorHeaps.size();
}

ID3D12DescriptorHeap ** D3D12TextureLoader::GetAllHeaps()
{
	return m_DescriptorHeaps.data();
}

void D3D12TextureLoader::SynchronizeWork()
{
	std::unique_lock<std::mutex> lock(m_mutex_TextureResources);
	m_cv_empty.wait(lock, [this]() {return mTexturesToLoadToGPU.empty(); });
}

void D3D12TextureLoader::Kill()
{
	std::unique_lock<std::mutex> lock(m_mutex_TextureResources);
	stop = true;
	m_cv_not_empty.notify_all();
}

//ID3D12DescriptorHeap * D3D12TextureLoader::GetDescriptorHeap()
//{
//	return mDescriptorHeap;
//}

void D3D12TextureLoader::WaitForCopy()
{
	const UINT64 fence = m_FenceValue;
	m_CommandQueue->Signal(m_Fence, fence);
	m_FenceValue++;

	//Wait until command queue is done.
	if (m_Fence->GetCompletedValue() < fence)
	{
		m_Fence->SetEventOnCompletion(fence, m_EventHandle);
		WaitForSingleObject(m_EventHandle, INFINITE);
	}
}

bool D3D12TextureLoader::AddDescriptorHeap()
{
	ID3D12DescriptorHeap* descriptorHeap;

	HRESULT hr;
	D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};
	heapDescriptorDesc.NumDescriptors = MAX_SRVs_PER_DESCRIPTOR_HEAP;
	heapDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hr = m_Renderer->GetDevice()->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&descriptorHeap));
	if (FAILED(hr))
		return false;

	m_DescriptorHeaps.push_back(descriptorHeap);
}
