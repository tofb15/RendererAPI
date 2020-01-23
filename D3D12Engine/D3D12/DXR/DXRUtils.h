#pragma once
#include "..\D3D12_FDecl.h"
#include "..\..\Math.hpp"

namespace DXRUtils {
	inline D3D12_HEAP_PROPERTIES GetDefaultHeapProps();
	ID3D12Resource* CreateBuffer(ID3D12Device5* device, uint64_t size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC* bufDesc = nullptr);

}
