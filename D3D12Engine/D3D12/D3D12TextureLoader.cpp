#include "stdafx.h"

#include "D3D12TextureLoader.hpp"
#include "External/D3DX12/d3dx12.h"
#include "D3D12API.hpp"
#include "D3D12Texture.hpp"
#include <chrono>
#include "External/LodePNG/lodepng.h"
#include "DXR/D3D12Utils.h"

D3D12TextureLoader::D3D12TextureLoader(D3D12API * renderer) : m_d3d12(renderer)
{

}

D3D12TextureLoader::~D3D12TextureLoader()
{
	// This is not necessary since this thread waits for every signal after executing the queue
	//WaitForCopy(m_fenceValue - 1);

	for (auto heap : m_descriptorHeaps)
	{
		heap->Release();
	}

	for (auto resource : m_textureResources)
	{
		resource->Release();
	}

	if (m_uploadResource) {
		m_uploadResource->Release();
	}

	m_commandQueue->Release();
	m_commandAllocator->Release();
	m_commandList->Release();
	m_fence->Release();
}

bool D3D12TextureLoader::Initialize()
{
	m_CBV_SRV_UAV_DescriptorSize = m_d3d12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	HRESULT hr;
	//Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	cqd.Type = D3D12_COMMAND_LIST_TYPE_COPY;
	hr = m_d3d12->GetDevice()->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_commandQueue));
	if (FAILED(hr))
	{
		return false;
	}

	//Create command allocator. The command allocator object corresponds
	//to the underlying allocations in which GPU commands are stored.
	hr = m_d3d12->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_COPY, IID_PPV_ARGS(&m_commandAllocator));
	if (FAILED(hr))
	{
		return false;
	}

	//Create command list.
	hr = m_d3d12->GetDevice()->CreateCommandList(
		0,
		D3D12_COMMAND_LIST_TYPE_COPY,
		m_commandAllocator,
		nullptr,
		IID_PPV_ARGS(&m_commandList));
	if (FAILED(hr))
	{
		return false;
	}

	m_commandList->Close();

	hr = m_d3d12->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_fence));
	if (FAILED(hr))
	{
		return false;
	}

	m_fenceValue = 1;
	//Create an event handle to use for GPU synchronization.
	m_eventHandle = CreateEvent(0, false, false, 0);

	m_gpu_upload_Worker = std::thread(&D3D12TextureLoader::GPUUploaderDoWork, this);
	for (int i = 0; i < N_RAM_LOADER_THREADS; i++) {
		m_ram_upload_Worker[i] = std::thread(&D3D12TextureLoader::RAMUploaderDoWork, this);
	}

	return true;
}

void D3D12TextureLoader::LoadTextureToGPU(D3D12Texture * texture)
{
	std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
	
	if (!texture->IsLoaded() && !texture->IsDDS()) {
		m_texturesToLoadToRAM.push_back(texture);
		m_cv_ram_not_empty.notify_one();//Wake up the RAM loader thread (D3D12TextureLoader::RAMUploaderDoWork)
	}
	else {
		m_texturesToLoadToGPU.push_back(texture);
		m_cv_gpu_not_empty.notify_one();//Wake up the GPU loader thread (D3D12TextureLoader::GPUUploaderDoWork)
	}

	if (!m_atLeastOneTextureIsLoaded) {
		lock.unlock();
		SynchronizeWork();
	}
	
		
}

D3D12_GPU_DESCRIPTOR_HANDLE D3D12TextureLoader::GetSpecificTextureGPUAdress(D3D12Texture * texture)
{
	int index = texture->m_GPU_Loader_index;
	if (index == -1)
		index = 0;

	int HeapIndex = index / MAX_SRVs_PER_DESCRIPTOR_HEAP;
	int localIndex = index % MAX_SRVs_PER_DESCRIPTOR_HEAP;

	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_descriptorHeaps[HeapIndex]->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += localIndex * m_CBV_SRV_UAV_DescriptorSize;

	return handle;
}

D3D12_CPU_DESCRIPTOR_HANDLE D3D12TextureLoader::GetSpecificTextureCPUAdress(D3D12Texture * texture)
{
	std::unique_lock<std::mutex> lock(m_mutex_TextureResources);

	int index = 0;
	if (texture) {
		index = texture->m_GPU_Loader_index;
	} else {
		index = 0;
	}
	if (index == -1)
		index = 0;

	int HeapIndex = index / MAX_SRVs_PER_DESCRIPTOR_HEAP;
	int localIndex = index % MAX_SRVs_PER_DESCRIPTOR_HEAP;

	D3D12_CPU_DESCRIPTOR_HANDLE handle = m_descriptorHeaps[HeapIndex]->GetCPUDescriptorHandleForHeapStart();
	handle.ptr += localIndex * m_CBV_SRV_UAV_DescriptorSize;

	return handle;
}

ID3D12Resource * D3D12TextureLoader::GetResource(int index)
{
	std::unique_lock<std::mutex> lock(m_mutex_TextureResources);
	return m_textureResources[index];
}

unsigned D3D12TextureLoader::GetNumberOfHeaps()
{
	return static_cast<unsigned>(m_descriptorHeaps.size());
}

ID3D12DescriptorHeap ** D3D12TextureLoader::GetAllHeaps()
{
	return m_descriptorHeaps.data();
}

void D3D12TextureLoader::SynchronizeWork()
{
	std::unique_lock<std::mutex> lock(m_mutex_TextureResources);
	m_cv_ram_empty.wait(lock, [this]() {return m_texturesToLoadToRAM.empty(); });
	m_cv_gpu_empty.wait(lock, [this]() {return m_texturesToLoadToGPU.empty(); });
}

void D3D12TextureLoader::Kill()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex_TextureResources);
		m_stop = true;
		m_texturesToLoadToGPU.clear();
		m_texturesToLoadToRAM.clear();

		m_cv_gpu_not_empty.notify_all();
		m_cv_ram_not_empty.notify_all();	
	}

	m_gpu_upload_Worker.join();
	for (int i = 0; i < N_RAM_LOADER_THREADS; i++) {
		m_ram_upload_Worker[i].join();
	}
}

void D3D12TextureLoader::GPUUploaderDoWork()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
		m_cv_gpu_not_empty.wait(lock, [this]() {return !m_texturesToLoadToGPU.empty() || m_stop; });//Force the thread to sleep if there is no texture to load. Mutex will be unlocked aslong as the thread is sleeping.
	}

	while (!m_stop)
	{
		/*if(atLeastOneTextureIsLoaded)
			std::this_thread::sleep_for(std::chrono::seconds(2));*/
		
		
		D3D12Texture* texture;
		bool reuseResource = false;

		//Critical Region.
		{
			std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
			texture = m_texturesToLoadToGPU[0];
			m_texturesToLoadToGPU.erase(m_texturesToLoadToGPU.begin());
		}

		reuseResource = (texture->m_GPU_Loader_index != -1);//If the texture already was loaded, it is just requesting a GPU update.

		ID3D12Resource* textureResource = nullptr;
		D3D12_RESOURCE_DESC resourceDesc = {};
		D3D12_HEAP_PROPERTIES heapProperties = {};
		HRESULT hr;
		
		m_commandAllocator->Reset();
		m_commandList->Reset(m_commandAllocator, nullptr);	
		if (texture->IsDDS()) {
			if (!texture->LoadFromFile_Blocking(&textureResource)) {
				//TODO: Handle Error
			}
			resourceDesc = texture->GetTextureDescription();

		} else {
			resourceDesc = texture->GetTextureDescription();
			if (reuseResource) {
				std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
				textureResource = m_textureResources[texture->m_GPU_Loader_index];
			}

			//resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			//resourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			//resourceDesc.Alignment = 0;//D3D12_SMALL_MSAA_RESOURCE_PLACEMENT_ALIGNMENT;	//4KB if the texture is <  64KB; 64KB if the texture is < 4MB; else bigger (see flags)
			//resourceDesc.Width = texture->m_Width;
			//resourceDesc.Height = texture->m_Height;
			//resourceDesc.DepthOrArraySize = 1;
			//resourceDesc.MipLevels = 1;
			//resourceDesc.SampleDesc.Count = 1;
			//resourceDesc.SampleDesc.Quality = 0;
			//resourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
			//resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;		//Row major did not work here!

			heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heapProperties.CreationNodeMask = 1; //used when multi-gpu
			heapProperties.VisibleNodeMask = 1; //used when multi-gpu
			heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	#pragma region INITIALIZE_DEFAULT_HEAP
			if (!reuseResource) {
				heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;			//D3D12_HEAP_TYPE_UPLOAD did not work here!

				hr = m_d3d12->GetDevice()->CreateCommittedResource(
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
			}
	#pragma endregion

		}
			//GetRequiredIntermediateSize returns w * h * "sizeof"(Format). Example: 640(width) * 640(height) * 4(Bytes) * 4(Channels)
			const UINT64 uploadBufferSize = GetRequiredIntermediateSize(textureResource, 0, 1) + D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT;

	#pragma region INITIALIZE_UPLOAD_HEAP

			//Reuse uploadheap if possible
			if (m_uploadResource_Size < uploadBufferSize) {
				if (m_uploadResource) {
					m_uploadResource->Release();
				}

				//Reusing heap-prop
				heapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;

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

				hr = m_d3d12->GetDevice()->CreateCommittedResource(
					&heapProperties,
					D3D12_HEAP_FLAG_NONE,
					&uploadResourceDesc,
					D3D12_RESOURCE_STATE_GENERIC_READ,
					nullptr,
					IID_PPV_ARGS(&m_uploadResource));
				if (FAILED(hr))
				{
					return;
				}
			}

			D3D12_SUBRESOURCE_DATA textureData = {};
			if (texture->IsDDS()) {
				textureData = texture->GetSubResourceData_DDS()[0];
			} else {
				textureData.pData = reinterpret_cast<void*>(texture->m_Image_CPU.data());//;reinterpret_cast<UINT8*>(rgb);
				textureData.RowPitch = texture->m_Width * texture->m_BytesPerPixel;
				textureData.SlicePitch = textureData.RowPitch * texture->m_Height;
			}

			UpdateSubresources<1>(m_commandList, textureResource, m_uploadResource, 0, 0, 1, &textureData);
			if ((texture->GetFlags() & Texture::TEXTURE_USAGE_CPU_FLAG) == 0) {
				texture->m_Image_CPU.clear();
				texture->m_Image_CPU.shrink_to_fit();
			}

	#pragma endregion


			m_commandList->Close();
			ID3D12CommandList* ppCommandLists[] = { m_commandList };
			m_commandQueue->ExecuteCommandLists(1, ppCommandLists);

#pragma region CreateShaderResourceView
		////// Describe and create a SRV for the texture.
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = resourceDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = resourceDesc.MipLevels;

		if (!reuseResource) {
			//if max SRV capacity is reached, Allocate a new DescriptorHeap. 
			unsigned localHeapIndex = m_nrOfTextures % MAX_SRVs_PER_DESCRIPTOR_HEAP;
			if (localHeapIndex == 0) {
				AddDescriptorHeap();
			}


			D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_descriptorHeaps.back()->GetCPUDescriptorHandleForHeapStart();
			cdh.ptr += localHeapIndex * m_CBV_SRV_UAV_DescriptorSize;
			m_d3d12->GetDevice()->CreateShaderResourceView(textureResource, &srvDesc, cdh);
		}

#pragma endregion


		SignalAndWaitForCopy();

		if (!reuseResource){
			std::unique_lock<std::mutex> lock(m_mutex_TextureResources);
			m_textureResources.push_back(textureResource);
			texture->m_GPU_Loader_index = m_nrOfTextures++;
		}
		
		{
			std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
			m_atLeastOneTextureIsLoaded = true;
			if (m_texturesToLoadToGPU.empty()) {
				m_cv_gpu_empty.notify_all();//Only used for D3D12TextureLoader::SynchronizeWork() to wake waiting threads
				m_cv_gpu_not_empty.wait(lock, [this]() {return !m_texturesToLoadToGPU.empty() || m_stop; });//Force the thread to sleep if there is no texture to load. Mutex will be unlocked aslong as the thread is sleeping.
			}
		}
	}
}
void D3D12TextureLoader::RAMUploaderDoWork()
{
	{
		std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
		m_cv_ram_not_empty.wait(lock, [this]() {return !m_texturesToLoadToRAM.empty() || m_stop; });//Force the thread to sleep if there is no texture to load. Mutex will be unlocked aslong as the thread is sleeping.
	}

	while (!m_stop)
	{
		D3D12Texture* texture;

		{
			//Critical Region.
			std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.
			texture = m_texturesToLoadToRAM[0];
			m_texturesToLoadToRAM.erase(m_texturesToLoadToRAM.begin());
		}
	
		bool textureLoadedToRam = texture->LoadFromFile_Blocking();

		{
			//Critical Region.
			std::unique_lock<std::mutex> lock(m_mutex_TextureResources);//Lock when in scope and unlock when out of scope.		
			if (textureLoadedToRam) {
				m_texturesToLoadToGPU.push_back(texture);
				m_cv_gpu_not_empty.notify_one();//Wake up the GPU loader thread
			}

			if (m_texturesToLoadToRAM.empty()) {
				m_cv_ram_empty.notify_all();//Only used for D3D12TextureLoader::SynchronizeWork() to wake waiting threads
				m_cv_ram_not_empty.wait(lock, [this]() {return !m_texturesToLoadToRAM.empty() || m_stop; });//Force the thread to sleep if there is no texture to load. Mutex will be unlocked aslong as the thread is sleeping.
			}
		}
	}

}

void D3D12TextureLoader::SignalAndWaitForCopy()
{
	const UINT64 fence = m_fenceValue;
	m_commandQueue->Signal(m_fence, fence);
	m_fenceValue++;

	WaitForCopy(fence);
}

void D3D12TextureLoader::WaitForCopy(UINT64 fence)
{
	//Wait until command queue is done.
	if (m_fence->GetCompletedValue() < fence)
	{
		m_fence->SetEventOnCompletion(fence, m_eventHandle);
		WaitForSingleObject(m_eventHandle, INFINITE);
	}
}

bool D3D12TextureLoader::AddDescriptorHeap()
{
	ID3D12DescriptorHeap* descriptorHeap;

	HRESULT hr;
	D3D12_DESCRIPTOR_HEAP_DESC heapDescriptorDesc = {};
	heapDescriptorDesc.NumDescriptors = MAX_SRVs_PER_DESCRIPTOR_HEAP;
	heapDescriptorDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	heapDescriptorDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	hr = m_d3d12->GetDevice()->CreateDescriptorHeap(&heapDescriptorDesc, IID_PPV_ARGS(&descriptorHeap));
	if (FAILED(hr))
		return false;

	m_descriptorHeaps.push_back(descriptorHeap);
	return true;
}
