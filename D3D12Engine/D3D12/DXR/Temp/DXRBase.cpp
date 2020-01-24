#include "DXRBase.h"
#include <d3d12.h>
#include <DirectXMath.h>

#include "DXRUtils.h"
#include "..\..\D3D12Mesh.hpp"
#include "..\..\D3D12VertexBuffer.hpp"

struct RayPayload {
	Float4 color;
	UINT recursionDepth;
};

DXRBase::DXRBase(D3D12API* d3d12) : m_d3d12(d3d12)
{

}

DXRBase::~DXRBase()
{

}

bool DXRBase::Initialize()
{
	if (!InitializeRootSignatures()) {
		return false;
	}

	///========Compiler Test============
	DXRUtils::PSOBuilder psoBuilder;
	if (!psoBuilder.Initialize()) {
		return false;
	}

	UINT payloadSize = sizeof(RayPayload);
	std::vector<DxcDefine> defines;

	defines.push_back({ L"TEST_DEFINE" });

	psoBuilder.AddLibrary("../assets/D3D12/DXR/test.hlsl", { m_shader_rayGenName, m_shader_closestHitName, m_shader_missName}, defines);
	psoBuilder.AddHitGroup(m_group_group1, m_shader_closestHitName);

	psoBuilder.AddSignatureToShaders({ m_shader_rayGenName }, m_localRootSignature_rayGen.Get());
	psoBuilder.AddSignatureToShaders({ m_group_group1 }, m_localRootSignature_hitGroups.Get());
	psoBuilder.AddSignatureToShaders({ m_shader_missName }, m_localRootSignature_miss.Get());

	psoBuilder.SetMaxPayloadSize(payloadSize);
	psoBuilder.SetMaxAttributeSize(sizeof(float) * 4);
	psoBuilder.SetMaxRecursionDepth(MAX_RAY_RECURSION_DEPTH);
	psoBuilder.SetGlobalSignature(m_globalRootSignature.Get());

	psoBuilder.Build(m_d3d12->GetDevice());

	return true;
}

void DXRBase::UpdateAccelerationStructures(std::vector<SubmissionItem>& items, ID3D12GraphicsCommandList4* cmdList)
{
	int frameIndex = 0;

	for (auto& e : m_BLAS_buffers[frameIndex])
	{
		e.second.items.clear();
	}

	//Build/Update BLAS
	for (auto& e : items)
	{
		BLAS_ID id = static_cast<D3D12Mesh*>(e.blueprint->mesh)->GetID();
		auto search = m_BLAS_buffers[frameIndex].find(id);
		if (search == m_BLAS_buffers[frameIndex].end()) {
			CreateBLAS(e, 0, cmdList);
		}
		else {
			//Prepare TLAS update		
			search->second.items.emplace_back(PerInstance{ e.transform });
		}
	}

	//Destroy unused BLASs
	for (auto it = m_BLAS_buffers[frameIndex].begin(); it != m_BLAS_buffers[frameIndex].end();)
	{
		if (it->second.items.empty()) {
			it->second.as.Release();
			it = m_BLAS_buffers[frameIndex].erase(it);
		}
		else {
			++it;
		}
	}

	//Build/Update TLAS
	CreateTLAS(items.size(), cmdList);
}

void DXRBase::UpdateSceneData()
{

}

void DXRBase::Dispatch(ID3D12GraphicsCommandList4* cmdList)
{
}

void DXRBase::ReloadShaders()
{
}

bool DXRBase::InitializeRootSignatures()
{
	if (!CreateDXRGlobalRootSignature()) {
		return false;
	}

	if (!CreateRayGenLocalRootSignature()) {
		return false;
	}

	if (!CreateMissLocalRootSignature()) {
		return false;
	}

	if (!CreateHitGroupLocalRootSignature()) {
		return false;
	}

	return true;
}

void DXRBase::CreateTLAS(unsigned int numInstanceDescriptors, ID3D12GraphicsCommandList4* cmdList)
{
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();	
	AccelerationStructureBuffers& tlas = m_TLAS_buffers[bufferIndex];
	tlas.Release(); //TODO: reuse TLAS insteed of destroying it.
	
	
	//=======Allocate GPU memory for Instance Data========
	tlas.instanceDesc = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * max(numInstanceDescriptors, 1U), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12Utils::sUploadHeapProperties);
	tlas.instanceDesc->SetName(L"TLAS_INSTANCE_DESC");

	//=======Describe the instances========
	D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
	tlas.instanceDesc->Map(0, nullptr, (void**)&pInstanceDesc);
	DirectX::XMMATRIX mat;
	for (auto& blas : m_BLAS_buffers[bufferIndex])
	{
		auto& instances = blas.second.items;
		int instanceID = 0;
		for (auto& instance : instances)
		{
			D3D12_RAYTRACING_INSTANCE_DESC instance_desc = {};
			instance_desc.InstanceID = instanceID;
			instance_desc.InstanceMask = 0x1; //Todo: make this changable
			instance_desc.InstanceContributionToHitGroupIndex = 0; // TODO: Change This
			instance_desc.Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			instance_desc.AccelerationStructure = blas.second.as.result->GetGPUVirtualAddress();

			// Construct and copy matrix data
			Float3& pos = instance.transform.pos;
			Float3& rot = instance.transform.rotation;
			Float3& scal = instance.transform.scale;

			mat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
			//mat = DirectX::XMMatrixIdentity();
			mat.r[3] = { pos.x, pos.y, pos.z, 1.0f };
			mat.r[0].m128_f32[0] *= scal.x;
			mat.r[1].m128_f32[1] *= scal.y;
			mat.r[2].m128_f32[2] *= scal.z;

			DirectX::XMStoreFloat3x4((DirectX::XMFLOAT3X4*)&instance_desc.Transform, mat);

			//memcpy(&instance_desc.Transform, &mat, sizeof(instance_desc.Transform));
			instanceID++;
			pInstanceDesc++;
		}
	}
	tlas.instanceDesc->Unmap(0, nullptr);

	//=======Describe the TLAS========
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.InstanceDescs = tlas.instanceDesc->GetGPUVirtualAddress();
	inputs.NumDescs = numInstanceDescriptors;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE; //Todo: Make this changable

	//=======TLAS PREBUILD========
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO asBuildInfo = {};
	m_d3d12->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &asBuildInfo);

	//=======Allocate GPU memory TLAS========
	//TODO: reuse old unused allocations.
	tlas.scratch = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), asBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12Utils::sDefaultHeapProps);
	tlas.scratch->SetName(L"TLAS_SCRATCH");
	tlas.result = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), asBuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12Utils::sDefaultHeapProps);
	tlas.result->SetName(L"TLAS_RESULT");

	//=======TLAS BUILD========
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;

	asDesc.ScratchAccelerationStructureData = tlas.scratch->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = tlas.result->GetGPUVirtualAddress();
	asDesc.SourceAccelerationStructureData = NULL; // Todo: Make this changable

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);
}

void DXRBase::CreateBLAS(const SubmissionItem& item, _D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers* sourceBufferForUpdate)
{
	BLAS_ID blasID = static_cast<D3D12Mesh*>(item.blueprint->mesh)->GetID();
	auto bufferIndex = m_d3d12->GetGPUBufferIndex();

	//=======Retrive the vertexbuffer========
	std::vector<D3D12VertexBuffer*>& buffers = *static_cast<D3D12Mesh*>(item.blueprint->mesh)->GetVertexBuffers();
	D3D12_VERTEX_BUFFER_VIEW* bufferView = buffers[0]->GetView();


	//=======Describe the geometry========
	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc = {};

	geomDesc.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

	//vertexbuffer
	geomDesc.Triangles.VertexBuffer.StartAddress = bufferView->BufferLocation;
	geomDesc.Triangles.VertexBuffer.StrideInBytes = bufferView->StrideInBytes;
	geomDesc.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT; //Todo: Make this changable
	geomDesc.Triangles.VertexCount = buffers[0]->GetNumberOfElements();
	//Indexbuffer
	geomDesc.Triangles.IndexBuffer = NULL;
	geomDesc.Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
	geomDesc.Triangles.IndexCount = 0;

	geomDesc.Triangles.Transform3x4 = 0;
	geomDesc.Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; //Todo: Make this changable

	//=======Describe the BLAS========
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.pGeometryDescs = &geomDesc;
	inputs.NumDescs = 1;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE; //Todo: Make this changable

	//=======BLAS PREBUILD========
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO asBuildInfo = {};
	m_d3d12->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &asBuildInfo);

	//=======Allocate GPU memory========
	BottomLayerData blasData = {};
	blasData.items.emplace_back(PerInstance{ item.transform });
	AccelerationStructureBuffers& asBuff = blasData.as;

	//TODO: reuse old unused allocations.
	asBuff.scratch = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), asBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12Utils::sDefaultHeapProps);
	asBuff.scratch->SetName(L"BLAS_SCRATCH");
	asBuff.result = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), asBuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12Utils::sDefaultHeapProps);
	asBuff.result->SetName(L"BLAS_RESULT");

	m_BLAS_buffers[bufferIndex].insert({ blasID, blasData });

	//=======BLAS BUILD========
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;

	asDesc.ScratchAccelerationStructureData = asBuff.scratch->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = asBuff.result->GetGPUVirtualAddress();
	asDesc.SourceAccelerationStructureData = NULL; // Todo: Make this changable

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);
}

void DXRBase::updateDescriptorHeap(ID3D12GraphicsCommandList4* cmdList)
{
}

void DXRBase::updateShaderTables()
{
}

void DXRBase::createInitialShaderResources(bool remake)
{
}

void DXRBase::CreateRaytracingPSO()
{
	DXRUtils::PSOBuilder psoBuilder;

	UINT payloadSize = sizeof(RayPayload);
	std::vector<DxcDefine> defines;
	defines.push_back({ L"DEF" });


	//TODO: MOVE DEFAULT_SHADER_LOCATION
	const std::string DEFAULT_SHADER_LOCATION = "shaders/";
	const std::string DEFAULT_SHADER_NAME = "basic";
	psoBuilder.AddLibrary(DEFAULT_SHADER_LOCATION + "dxr/" + DEFAULT_SHADER_NAME + ".hlsl", { m_shader_rayGenName, m_shader_missName, m_shader_shadowMissName, m_shader_closestHitName }, defines);
	psoBuilder.AddHitGroup(m_group_group1, m_shader_closestHitName);

	psoBuilder.AddSignatureToShaders({ m_shader_rayGenName }, m_localRootSignature_rayGen.Get());
	psoBuilder.AddSignatureToShaders({ m_group_group1 }, m_localRootSignature_hitGroups.Get());
	psoBuilder.AddSignatureToShaders({ m_shader_missName }, m_localRootSignature_miss.Get());

	psoBuilder.SetMaxPayloadSize(payloadSize);
	psoBuilder.SetMaxAttributeSize(sizeof(float) * 4);
	psoBuilder.SetMaxRecursionDepth(MAX_RAY_RECURSION_DEPTH);
	psoBuilder.SetGlobalSignature(m_globalRootSignature.Get());
}

bool DXRBase::CreateDXRGlobalRootSignature()
{
	m_globalRootSignature = D3D12Utils::RootSignature(L"dxrGlobalRoot");
	//m_globalRootSignature.AddDescriptorTable("OutputAlbedoUAV", D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0);
	//m_globalRootSignature.AddSRV("AccelerationStructure", 0);
	//m_globalRootSignature.AddCBV("SceneCBuffer", 0);
	//m_globalRootSignature.AddStaticSampler();

	if (!m_globalRootSignature.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_NONE)) {
		return false;
	}

	return true;
}

bool DXRBase::CreateRayGenLocalRootSignature()
{
	m_localRootSignature_rayGen = D3D12Utils::RootSignature(L"Root_RayGenLocal");
	if (!m_localRootSignature_rayGen.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
		return false;
	}

	return true;
}

bool DXRBase::CreateHitGroupLocalRootSignature()
{
	m_localRootSignature_hitGroups = D3D12Utils::RootSignature(L"Root_HitGroupLocal");
	//m_localRootSignature_hitGroups.AddSRV("VertexBuffer", 1, 0);
	//m_localRootSignature_hitGroups.AddSRV("IndexBuffer", 1, 1);
	//m_localRootSignature_hitGroups.AddCBV("MeshCBuffer", 1, 0);
	//m_localRootSignature_hitGroups.AddDescriptorTable("Textures", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 3); // Textures (t0, t1, t2)
	//m_localRootSignature_hitGroups.AddStaticSampler();
	if (!m_localRootSignature_hitGroups.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
		return false;
	}

	return true;
}

bool DXRBase::CreateMissLocalRootSignature()
{
	//Could be used to bind a skybox.

	m_localRootSignature_miss = D3D12Utils::RootSignature(L"Root_MissLocal");
	if (!m_localRootSignature_miss.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
		return false;
	}

	return true;
}

bool DXRBase::CreateEmptyLocalRootSignature()
{

	return true;
}

void DXRBase::AccelerationStructureBuffers::Release()
{
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
}
