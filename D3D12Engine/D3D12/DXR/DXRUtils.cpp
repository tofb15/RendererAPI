#include "DXRUtils.h"
#include <d3d12.h>
#include <comdef.h>

D3D12_HEAP_PROPERTIES DXRUtils::GetDefaultHeapProps()
{
	return D3D12_HEAP_PROPERTIES{
		D3D12_HEAP_TYPE_DEFAULT,
		D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
		D3D12_MEMORY_POOL_UNKNOWN,
		0,
		0
	};
}

ID3D12Resource* DXRUtils::CreateBuffer(ID3D12Device5* device, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC* bufDesc)
{
	D3D12_RESOURCE_DESC newBufDesc = {};
	if (!bufDesc) {
		newBufDesc.Alignment = 0;
		newBufDesc.DepthOrArraySize = 1;
		newBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		newBufDesc.Flags = flags;
		newBufDesc.Format = DXGI_FORMAT_UNKNOWN;
		newBufDesc.Height = 1;
		newBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		newBufDesc.MipLevels = 1;
		newBufDesc.SampleDesc.Count = 1;
		newBufDesc.SampleDesc.Quality = 0;
		newBufDesc.Width = size;

		bufDesc = &newBufDesc;
	}

	ID3D12Resource* pBuffer = nullptr;
	auto hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer));
	if (FAILED(hr)) {
		_com_error err(hr);
		std::cout << err.ErrorMessage() << std::endl;

		hr = device->GetDeviceRemovedReason();
		_com_error err2(hr);
		std::cout << err2.ErrorMessage() << std::endl;
	}
	return pBuffer;
}
