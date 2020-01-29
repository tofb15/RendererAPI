#include "stdafx.h"

#include "DXRBase.h"
#include "..\..\D3D12Mesh.hpp"
#include "..\..\D3D12VertexBuffer.hpp"
#include "..\..\D3D12Window.hpp"


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

	if (!CreateRaytracingPSO()) {
		return false;
	}

	if (!CreateDescriptorHeap()) {
		return false;
	}

	if (!InitializeConstanBuffers()) {
		return false;
	}
	
	return true;
}

void DXRBase::SetOutputResources(ID3D12Resource** output, Int2 dim)
{
	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
	uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
	uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	m_outputDim = dim;

	for (int i = 0; i < NUM_GPU_BUFFERS; i++) {
		m_d3d12->GetDevice()->CreateUnorderedAccessView(output[i], nullptr, &uavDesc, m_uav_output_texture_handle[i].cdh);
	}
}

void DXRBase::UpdateAccelerationStructures(std::vector<SubmissionItem>& items, ID3D12GraphicsCommandList4* cmdList)
{
	UINT gpuBuffer = m_d3d12->GetGPUBufferIndex();

	for (auto& e : m_BLAS_buffers[gpuBuffer])
	{
		e.second.items.clear();
	}

	//Build/Update BLAS
	for (auto& e : items)
	{
		BLAS_ID id = static_cast<D3D12Mesh*>(e.blueprint->mesh)->GetID();
		auto search = m_BLAS_buffers[gpuBuffer].find(id);
		if (search == m_BLAS_buffers[gpuBuffer].end()) {
			CreateBLAS(e, 0, cmdList);
		}
		else {
			//Prepare TLAS update		
			search->second.items.emplace_back(PerInstance{ e.transform });
		}
	}

	//Destroy unused BLASs
	for (auto it = m_BLAS_buffers[gpuBuffer].begin(); it != m_BLAS_buffers[gpuBuffer].end();)
	{
		if (it->second.items.empty()) {
			it->second.as.Release();
			it = m_BLAS_buffers[gpuBuffer].erase(it);
		}
		else {
			++it;
		}
	}

	//Build/Update TLAS
	CreateTLAS(items.size(), cmdList);
	//Update Shader Table
	UpdateShaderTable();
}

void DXRBase::UpdateSceneData(D3D12Camera* camera)
{
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();

	HRESULT hr;
	char* gpuData;

	D3D12_RANGE writeRange = {};
	writeRange.Begin = m_cb_scene_buffer_size * bufferIndex;
	writeRange.End = writeRange.Begin + sizeof(DXRShaderCommon::SceneCBuffer);

	//Calculate Invers of ViewProjection matrix
	DirectX::XMMATRIX viewProj_inv = DirectX::XMLoadFloat4x4(&camera->GetViewPerspective());
	//viewProj_inv = DirectX::XMMatrixTranspose(viewProj_inv);
	viewProj_inv = (DirectX::XMMatrixInverse(nullptr, viewProj_inv));

	//Update scene constantbuffer
	hr = m_cb_scene->Map(0, nullptr, (void**)&gpuData);
	if (FAILED(hr)) {
		return;
	}

	gpuData += writeRange.Begin;
	DXRShaderCommon::SceneCBuffer* sceneData = (DXRShaderCommon::SceneCBuffer*)gpuData;

	sceneData->cameraPosition = camera->GetPosition();
	DirectX::XMStoreFloat4x4(&sceneData->projectionToWorld, viewProj_inv);

	m_cb_scene->Unmap(0, nullptr);
}

void DXRBase::Dispatch(ID3D12GraphicsCommandList4* cmdList)
{
	D3D12_DISPATCH_RAYS_DESC desc = {};
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();

	desc.RayGenerationShaderRecord.StartAddress = m_shaderTable_gen[bufferIndex].resource->GetGPUVirtualAddress();
	desc.RayGenerationShaderRecord.SizeInBytes  = m_shaderTable_gen[bufferIndex].sizeInBytes;

	desc.MissShaderTable.StartAddress  = m_shaderTable_miss[bufferIndex].resource->GetGPUVirtualAddress();
	desc.MissShaderTable.SizeInBytes   = m_shaderTable_miss[bufferIndex].sizeInBytes;
	desc.MissShaderTable.StrideInBytes = m_shaderTable_miss[bufferIndex].strideInBytes;

	desc.HitGroupTable.StartAddress  = m_shaderTable_hitgroup[bufferIndex].resource->GetGPUVirtualAddress();
	desc.HitGroupTable.SizeInBytes   = m_shaderTable_hitgroup[bufferIndex].sizeInBytes;
	desc.HitGroupTable.StrideInBytes = m_shaderTable_hitgroup[bufferIndex].strideInBytes;

	desc.Width = m_outputDim.x;
	desc.Height = m_outputDim.y;
	desc.Depth = 1;

	cmdList->SetComputeRootSignature(m_globalRootSignature);
	cmdList->SetComputeRootShaderResourceView(0, m_TLAS_buffers[bufferIndex].result->GetGPUVirtualAddress());
	cmdList->SetComputeRootConstantBufferView(1, m_cb_scene->GetGPUVirtualAddress());

	cmdList->SetDescriptorHeaps(1, &m_descriptorHeap);
	cmdList->SetPipelineState1(m_rtxPipelineState);
	cmdList->DispatchRays(&desc);
}

void DXRBase::ReloadShaders()
{
	m_d3d12->WaitForGPU_ALL();
	// Recompile hlsl
	CreateRaytracingPSO();
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

bool DXRBase::InitializeConstanBuffers()
{
	UINT alignTo = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
	//Scene
	m_cb_scene_buffer_size = sizeof(DXRShaderCommon::SceneCBuffer);
	UINT padding = (alignTo - (m_cb_scene_buffer_size % alignTo)) % alignTo;
	m_cb_scene_buffer_size += padding;
	m_cb_scene = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), m_cb_scene_buffer_size * NUM_GPU_BUFFERS, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12Utils::sUploadHeapProperties);

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.SizeInBytes = m_cb_scene_buffer_size;
	for (int i = 0; i < NUM_GPU_BUFFERS; i++) {
		cbvDesc.BufferLocation = m_cb_scene->GetGPUVirtualAddress() + i * cbvDesc.SizeInBytes;
		m_d3d12->GetDevice()->CreateConstantBufferView(&cbvDesc, m_cbv_scene_handle[i].cdh);
	}

	////==========DEBUG START, REMOVE THIS=========
	//DirectX::XMMATRIX identity_mat = DirectX::XMMatrixIdentity();
	//DirectX::XMMATRIX transform_mat;

	//char* debugData;
	//HRESULT hr;
	//DXRShaderCommon::SceneCBuffer data;
	//	
	//hr = m_cb_scene->Map(0, nullptr, (void**)&debugData);
	//if (FAILED(hr)) {
	//	return false;
	//}

	//for (size_t i = 0; i < NUM_GPU_BUFFERS; i++)
	//{
	//	data.cameraPosition = Float3(i, i * 2, i * 3 );
	//	transform_mat = DirectX::XMMatrixTranslation(i * 10, i * 20, i * 30);
	//	DirectX::XMStoreFloat4x4(&data.projectionToWorld, transform_mat);
	//	DirectX::XMStoreFloat4x4(&data.viewToWorld, identity_mat);

	//	memcpy(debugData, &data, sizeof(DXRShaderCommon::SceneCBuffer));
	//	debugData += m_cb_scene_buffer_size;
	//}

	//m_cb_scene->Unmap(0, nullptr);
	////==========DEBUG END, REMOVE THIS=========

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
	UINT blasIndex = 0;
	for (auto& blas : m_BLAS_buffers[bufferIndex])
	{
		auto& instances = blas.second.items;
		int instanceID = 0;
		for (auto& instance : instances)
		{
			pInstanceDesc->InstanceID = instanceID;
			pInstanceDesc->InstanceMask = 0x1; //Todo: make this changable
			pInstanceDesc->InstanceContributionToHitGroupIndex = blasIndex; // TODO: Change This
			pInstanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
			pInstanceDesc->AccelerationStructure = blas.second.as.result->GetGPUVirtualAddress();

			// Construct and copy matrix data
			Float3& pos = instance.transform.pos;
			Float3& rot = instance.transform.rotation;
			Float3& scal = instance.transform.scale;

			//mat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
			mat = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
			//mat = DirectX::XMMatrixIdentity();
			//mat.r[3] = { pos.x, pos.y, pos.z, 1.0f };
			//mat.r[0].m128_f32[0] *= scal.x;
			//mat.r[1].m128_f32[1] *= scal.y;
			//mat.r[2].m128_f32[2] *= scal.z;

			DirectX::XMStoreFloat3x4((DirectX::XMFLOAT3X4*)pInstanceDesc->Transform, mat);

			//memcpy(&instance_desc.Transform, &mat, sizeof(instance_desc.Transform));
			instanceID++;
			pInstanceDesc++;
		}
		blasIndex++;
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
	tlas.result = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), asBuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12Utils::sDefaultHeapProps);
	tlas.result->SetName(L"TLAS_RESULT");

	//=======TLAS BUILD========
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;

	asDesc.ScratchAccelerationStructureData = tlas.scratch->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = tlas.result->GetGPUVirtualAddress();
	//asDesc.SourceAccelerationStructureData = NULL; // Todo: Make this changable

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// UAV barrier needed before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = tlas.result;
	cmdList->ResourceBarrier(1, &uavBarrier);
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
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE; //Todo: Make this changable

	//=======BLAS PREBUILD========
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO asBuildInfo = {};
	m_d3d12->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &asBuildInfo);

	//=======Initialize BLAS Data========
	BottomLayerData blasData = {};
	blasData.items.emplace_back(PerInstance{ item.transform });
	blasData.gpu_add_vb_pos = static_cast<D3D12Mesh*>(item.blueprint->mesh)->GetVertexBuffers()->at(0)->GetResource()->GetGPUVirtualAddress();
	AccelerationStructureBuffers& asBuff = blasData.as;

	//=======Allocate GPU memory========
	//TODO: reuse old unused allocations.
	asBuff.scratch = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), asBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12Utils::sDefaultHeapProps);
	asBuff.scratch->SetName(L"BLAS_SCRATCH");
	asBuff.result = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), asBuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12Utils::sDefaultHeapProps);
	asBuff.result->SetName(L"BLAS_RESULT");

	m_BLAS_buffers[bufferIndex].insert({ blasID, blasData });

	//=======BLAS BUILD========
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC asDesc = {};
	asDesc.Inputs = inputs;

	asDesc.ScratchAccelerationStructureData = asBuff.scratch->GetGPUVirtualAddress();
	asDesc.DestAccelerationStructureData = asBuff.result->GetGPUVirtualAddress();
	asDesc.SourceAccelerationStructureData = NULL; // Todo: Make this changable

	cmdList->BuildRaytracingAccelerationStructure(&asDesc, 0, nullptr);

	// We need to insert a UAV barrier before using the acceleration structures in a raytracing operation
	D3D12_RESOURCE_BARRIER uavBarrier = {};
	uavBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
	uavBarrier.UAV.pResource = asBuff.result;
	cmdList->ResourceBarrier(1, &uavBarrier);
}

void DXRBase::UpdateShaderTable()
{
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
	//TODO: dont rebuild tables if not needed.

	//===Update Generation Table===
	DXRUtils::ShaderTableBuilder generationTable(1, m_rtxPipelineState, 64);
	generationTable.AddShader(m_shader_rayGenName);
	generationTable.AddDescriptor(m_uav_output_texture_handle[bufferIndex].gdh.ptr);
	m_shaderTable_gen[bufferIndex].Release();
	m_shaderTable_gen[bufferIndex] = generationTable.Build(m_d3d12->GetDevice());

	//===Update Miss Table===
	DXRUtils::ShaderTableBuilder missTable(1, m_rtxPipelineState);
	missTable.AddShader(m_shader_missName);
	m_shaderTable_miss[bufferIndex].Release();
	m_shaderTable_miss[bufferIndex] = missTable.Build(m_d3d12->GetDevice());

	//===Update HitGroup Table===
	DXRUtils::ShaderTableBuilder hitGroupTable(m_BLAS_buffers[bufferIndex].size(), m_rtxPipelineState);

	UINT blasIndex = 0;
	for (auto& e : m_BLAS_buffers[bufferIndex])
	{
		hitGroupTable.AddShader(m_group_group1);
		hitGroupTable.AddDescriptor(e.second.gpu_add_vb_pos, blasIndex);
		blasIndex++;
	}

	m_shaderTable_hitgroup[bufferIndex].Release();
	m_shaderTable_hitgroup[bufferIndex] = hitGroupTable.Build(m_d3d12->GetDevice());
}

void DXRBase::updateDescriptorHeap(ID3D12GraphicsCommandList4* cmdList)
{

}

void DXRBase::createInitialShaderResources(bool remake)
{
}

bool DXRBase::CreateRaytracingPSO()
{
	if (m_rtxPipelineState) {
		m_rtxPipelineState->Release();
	}

	DXRUtils::PSOBuilder psoBuilder;
	if (!psoBuilder.Initialize()) {
		return false;
	}

	UINT payloadSize = sizeof(DXRShaderCommon::RayPayload);
	std::vector<DxcDefine> defines;

	defines.push_back({ L"TEST_DEFINE" });

	psoBuilder.AddLibrary("../Shaders/D3D12/DXR/test.hlsl", { m_shader_rayGenName, m_shader_closestHitName, m_shader_missName }, defines);
	psoBuilder.AddHitGroup(m_group_group1, m_shader_closestHitName);

	psoBuilder.AddSignatureToShaders({ m_shader_rayGenName }, m_localRootSignature_rayGen.Get());
	psoBuilder.AddSignatureToShaders({ m_group_group1 }, m_localRootSignature_hitGroups.Get());
	psoBuilder.AddSignatureToShaders({ m_shader_missName }, m_localRootSignature_miss.Get());

	psoBuilder.SetMaxPayloadSize(payloadSize);
	psoBuilder.SetMaxAttributeSize(sizeof(float) * 4);
	psoBuilder.SetMaxRecursionDepth(MAX_RAY_RECURSION_DEPTH);
	psoBuilder.SetGlobalSignature(m_globalRootSignature.Get());

	m_rtxPipelineState = psoBuilder.Build(m_d3d12->GetDevice());
	if (!m_rtxPipelineState) {
		return false;
	}

	return true;
}

bool DXRBase::CreateDescriptorHeap()
{
	if (m_descriptorHeap) {
		m_descriptorHeap->Release();
	}

	D3D12_DESCRIPTOR_HEAP_DESC descHeapDesc = {};
	descHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	descHeapDesc.NumDescriptors = NUM_DESCRIPTORS_TOTAL;
	descHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;

	m_descriptorSize = m_d3d12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	HRESULT hr;
	hr = m_d3d12->GetDevice()->CreateDescriptorHeap(&descHeapDesc, IID_PPV_ARGS(&m_descriptorHeap));
	if (FAILED(hr)) {
		return false;
	}

	for (size_t i = 0; i < NUM_GPU_BUFFERS; i++)
	{
		m_descriptorHeap_start[i].cdh = m_descriptorHeap->GetCPUDescriptorHandleForHeapStart();
		m_descriptorHeap_start[i].gdh = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
		m_descriptorHeap_start[i] += m_descriptorSize * NUM_DESCRIPTORS_PER_GPU_BUFFER * i;
	}

	m_unreserved_handle_start = m_descriptorHeap_start;

	auto reserveDescriptor = [&](D3D12Utils::D3D12_DESCRIPTOR_HANDLE_BUFFERED& handle) {	
		handle = m_unreserved_handle_start;
		m_unreserved_handle_start += m_descriptorSize;	
		m_numReservedDescriptors++;
	};

	//Reserved Descriptors
	reserveDescriptor(m_uav_output_texture_handle);
	reserveDescriptor(m_cbv_scene_handle);

	return true;
}

D3D12_GPU_DESCRIPTOR_HANDLE DXRBase::GetCurrentDescriptorHandle()
{
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
	D3D12_GPU_DESCRIPTOR_HANDLE handle = m_descriptorHeap->GetGPUDescriptorHandleForHeapStart();
	handle.ptr += m_descriptorSize * bufferIndex;

	return handle;
}

bool DXRBase::CreateDXRGlobalRootSignature()
{
	m_globalRootSignature = D3D12Utils::RootSignature(L"dxrGlobalRoot");
	m_globalRootSignature.AddSRV("AccelerationStructure", 0);
	m_globalRootSignature.AddCBV("SceneCBuffer", 0);
	//m_globalRootSignature.AddSRV("AccelerationStructure", 0);
	m_globalRootSignature.AddStaticSampler();

	if (!m_globalRootSignature.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_NONE)) {
		return false;
	}

	return true;
}

bool DXRBase::CreateRayGenLocalRootSignature()
{
	m_localRootSignature_rayGen = D3D12Utils::RootSignature(L"Root_RayGenLocal");
	m_localRootSignature_rayGen.AddDescriptorTable("uav_test", D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1);
	//m_localRootSignature_rayGen.Add32BitConstants("Constant", 1, 0, 0);
	if (!m_localRootSignature_rayGen.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
		return false;
	}

	return true;
}

bool DXRBase::CreateHitGroupLocalRootSignature()
{
	m_localRootSignature_hitGroups = D3D12Utils::RootSignature(L"Root_HitGroupLocal");
	m_localRootSignature_hitGroups.AddSRV("VertexBuffer", 1, 0);
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
