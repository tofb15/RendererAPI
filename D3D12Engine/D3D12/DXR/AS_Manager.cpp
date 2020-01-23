#include "AS_Manager.h"
#include <d3d12.h>
#include "..\D3D12VertexBuffer.hpp"
#include "..\D3D12Mesh.hpp"
#include "..\D3D12API.hpp"
#include "..\DXR\DXRUtils.h"

AccelerationStructure::AccelerationStructure(D3D12API* d3d12) : m_d3d12(d3d12)
{
}

AccelerationStructure::~AccelerationStructure()
{
}

bool AccelerationStructure::UpdateAccelerationStructure(std::vector<SubmissionItem>& items, ID3D12GraphicsCommandList4* cmdList)
{
	// Clear old instance lists
	for (auto& it : m_blasBuffer) {
		it.second.clear();
	}

	for (auto& e : items)
	{
		UINT id = (UINT)static_cast<D3D12Mesh*>(e.blueprint->mesh)->GetID();

		if (m_blasBuffer.count(id)) {
			m_blasBuffer[id].emplace_back(e.transform);
		}
		else {
			CreateBLAS(&e, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE, cmdList, nullptr);
		}
	}

	return false;
}

bool AccelerationStructure::Build()
{
	return false;
}

void AccelerationStructure::CreateBLAS(SubmissionItem* item, D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers* sourceBufferForUpdate)
{
	AccelerationStructureBuffers blas;

	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};
	auto vb = (*static_cast<D3D12Mesh*>(item->blueprint->mesh)->GetVertexBuffers())[0];

	geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
	geomDesc.Triangles.VertexBuffer.StartAddress = vb->GetResource()->GetGPUVirtualAddress();
	geomDesc.Triangles.VertexBuffer.StrideInBytes = vb->GetElementSize();
	geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
	geomDesc.Triangles.VertexCount = vb->GetNumberOfElements();
	geomDesc.Triangles.IndexBuffer = NULL;
	geomDesc.Triangles.IndexCount = 0;

	// Get the size requirements for the scratch and AS buffers
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.Flags = flags; // Changing this flag depending on mesh can speed up performance significantly!
	//if (performInplaceUpdate) {
	//	inputs.Flags |= D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE;
	//}
	inputs.NumDescs = 1;
	inputs.pGeometryDescs = &geomDesc;
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;

	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info = {};
	m_d3d12->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &info);

	// TODO: make sure buffer size is >= info.UpdateScratchDataSize in bytes
	if (true/*!performInplaceUpdate*/) {
		// Create the buffers. They need to support UAV, and since we are going to immediately use them, we create them with an unordered-access state
		blas.scratch = DXRUtils::CreateBuffer(m_d3d12->GetDevice(), info.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS,DXRUtils::GetDefaultHeapProps());
		blas.scratch->SetName(L"BLAS_SCRATCH");
		blas.result = DXRUtils::CreateBuffer(m_d3d12->GetDevice(), info.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, DXRUtils::GetDefaultHeapProps());
		blas.result->SetName(L"BLAS_RESULT");
	}

	// Create the bottom-level AS
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;
	asDesc.ScratchAccelerationStructureData = blas.scratch->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = blas.result->GetGPUVirtualAddress();
	if (inputs.Flags & D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PERFORM_UPDATE) {
		asDesc.SourceAccelerationStructureData = sourceBufferForUpdate->result->GetGPUVirtualAddress();
	}

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = blas.result;
	cmdList->ResourceBarrier(1, &uavBarrier);

	if (true/*!performInplaceUpdate*/) {
		// Insert BLAS into buttom buffer map
		m_blasBuffer.insert({ static_cast<D3D12Mesh*>(item->blueprint->mesh)->GetID(), {item->transform} });
	}
}