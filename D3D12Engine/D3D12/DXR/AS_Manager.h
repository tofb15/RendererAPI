#pragma once
#include "..\D3D12_FDecl.h"
#include "..\..\Math.hpp"
#include "..\Renderers\D3D12Renderer.h"

#include <vector>
#include <unordered_map>

typedef UINT BLASHandle;
typedef UINT TLASHandle;

class D3D12Mesh;
class D3D12API;

enum D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS;

class AccelerationStructure
{
public:
	AccelerationStructure(D3D12API* d3d12);
	~AccelerationStructure();
	bool UpdateAccelerationStructure(std::vector<SubmissionItem>& items, ID3D12GraphicsCommandList4* cmdList);

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

	std::unordered_map<UINT, std::vector<Transform>> m_blasBuffer;

	bool Build();
	void CreateBLAS(SubmissionItem* mesh, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers* sourceBufferForUpdate);

private:
	D3D12API* m_d3d12;
};