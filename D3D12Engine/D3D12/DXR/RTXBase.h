#pragma once
#include "..\D3D12_FDecl.h"
#include "..\..\Math.hpp"
#include <vector>

class RTXBase
{
public:
	RTXBase();
	~RTXBase();

private:
	struct AccelerationStructureBuffers {
		ID3D12Resource* scratch = nullptr;
		ID3D12Resource* result = nullptr;
		ID3D12Resource* instanceDesc = nullptr;    // Used only for top-level AS
		
		bool allowUpdate = false;
		/*void release() {
			if (scratch) {
				scratch->Release();
				scratch = nullptr;
			}
			if (result) {
				result->Release();
				result = nullptr;
			}
			if (instanceDesc) {
				instanceDesc->Release();
				instanceDesc = nullptr;
			}
		}*/
	};

	struct PerInstance {
		Transform transform;
	};

	struct InstanceList {
		AccelerationStructureBuffers blas;
		std::vector<PerInstance> instanceList;
	};

};