#include "stdafx.h"

#include "DXRBase.h"
#include "..\D3D12Mesh.hpp"
#include "..\D3D12VertexBuffer.hpp"
#include "..\D3D12Window.hpp"
#include "..\D3D12Texture.hpp"
#include "..\D3D12Technique.hpp"
#include"..\Shaders\D3D12\DXR\Common_hlsl_cpp.hlsli"

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

	if (!CreateRaytracingPSO(nullptr)) {
		return false;
	}

	if (!CreateDescriptorHeap()) {
		return false;
	}

	if (!InitializeConstanBuffers()) {
		return false;
	}
	
#ifdef DO_TESTING
	m_gpuTimer.Init(m_d3d12->GetDevice(), NUM_GPU_BUFFERS);
#endif // DO_TESTING

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
		if (e.first->hasChanged) {
			e.first->hasChanged = false;
			for (size_t i = 0; i < NUM_GPU_BUFFERS; i++)
			{
				auto search = m_BLAS_buffers[i].find(e.first);
				if (search != m_BLAS_buffers[i].end()) {
					search->second.needsRebuild = true;
				}
			}
		}
	}

	if (m_forceBLASRebuild[gpuBuffer]) {
		m_forceBLASRebuild[gpuBuffer] = false;
		//Destroy all BLASs
		for (auto it = m_BLAS_buffers[gpuBuffer].begin(); it != m_BLAS_buffers[gpuBuffer].end();)
		{
			it->second.as.Release();
			it = m_BLAS_buffers[gpuBuffer].erase(it);
		}
	}

	//Build/Update BLAS
	for (auto& e : items)
	{
		//BLAS_ID id = static_cast<D3D12Mesh*>(e.blueprint->mesh)->GetID();
		auto search = m_BLAS_buffers[gpuBuffer].find(e.blueprint);
		if (search == m_BLAS_buffers[gpuBuffer].end()) {
			CreateBLAS(e, 0, cmdList);
		}
		else {
			if (search->second.needsRebuild) {
				search->second.as.Release();
				m_BLAS_buffers[gpuBuffer].erase(search);
				CreateBLAS(e, 0, cmdList);
			}
			else {
				//Insert instance	
				search->second.items.emplace_back(PerInstance{ e.transform });
			}
		}
	}

	m_hitGroupShaderRecordsNeededThisFrame = 0;
	//Destroy unused BLASs
	for (auto it = m_BLAS_buffers[gpuBuffer].begin(); it != m_BLAS_buffers[gpuBuffer].end();)
	{
		if (it->second.items.empty()) {
			it->second.as.Release();
			it = m_BLAS_buffers[gpuBuffer].erase(it);
		}
		else {
			m_hitGroupShaderRecordsNeededThisFrame += it->second.nGeometries;
			++it;
		}
	}

	//Build/Update TLAS
	CreateTLAS(items.size(), cmdList);
	UpdateDescriptorHeap(cmdList);
	//Update Shader Table
	UpdateShaderTable();
}

void DXRBase::UpdateSceneData(D3D12Camera* camera, const std::vector<LightSource>& lights)
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
	
	if (!lights.empty()) {
		sceneData->pLight.position = lights.back().m_position_animated;
	}
	else {
		sceneData->pLight.position = Float3(-10, 50, -10);
	}

	m_cb_scene->Unmap(0, nullptr);
}

void DXRBase::Dispatch(ID3D12GraphicsCommandList4* cmdList)
{
	static unsigned int counter = 0;
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
	
#ifdef DO_TESTING
	if (m_nUnExtractedTimerValues == NUM_GPU_BUFFERS) {
		ExtractGPUTimer();
	}

	m_gpuTimer.Start(cmdList, bufferIndex);
	m_nUnExtractedTimerValues++;
	cmdList->DispatchRays(&desc);
	m_gpuTimer.Stop(cmdList, bufferIndex);
	m_gpuTimer.ResolveQueryToCPU(cmdList, bufferIndex);
#else
	cmdList->DispatchRays(&desc);
#endif // DO_TESTING
}

void DXRBase::ReloadShaders(std::vector<ShaderDefine>* defines)
{
	m_d3d12->WaitForGPU_ALL();
	// Recompile hlsl
	CreateRaytracingPSO(defines);
}

void DXRBase::SetAllowAnyHitShader(bool b)
{
	if (m_allowAnyhitshaders != b) {
		for (size_t i = 0; i < NUM_GPU_BUFFERS; i++)
		{
			m_forceBLASRebuild[i] = true;
		}
	}
	m_allowAnyhitshaders = b;
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

	//Empty Rootsignature
	m_localRootSignature_empty = D3D12Utils::RootSignature(L"Root_Empty");
	if (!m_localRootSignature_empty.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
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
	UINT blasStartIndex = 0;
	for (auto& blas : m_BLAS_buffers[bufferIndex])
	{
		auto& instances = blas.second.items;
		int instanceID = 0;
		for (auto& instance : instances)
		{
			//blas.first->techniques[ins];
			
			pInstanceDesc->InstanceID = instanceID;
			pInstanceDesc->InstanceMask = 0x1; //Todo: make this changable
			pInstanceDesc->InstanceContributionToHitGroupIndex = blasStartIndex;
			pInstanceDesc->Flags = (blas.first->allGeometryIsOpaque || !m_allowAnyhitshaders) ? D3D12_RAYTRACING_INSTANCE_FLAG_NONE : D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
			pInstanceDesc->AccelerationStructure = blas.second.as.result->GetGPUVirtualAddress();

			// Construct and copy matrix data
			Float3& pos = instance.transform.pos;
			Float3& rot = instance.transform.rotation;
			Float3& scal = instance.transform.scale;

			mat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
			//mat = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
			//mat = DirectX::XMMatrixIdentity();
			mat.r[3] = { pos.x, pos.y, pos.z, 1.0f };
			mat.r[0].m128_f32[0] *= scal.x;
			mat.r[1].m128_f32[1] *= scal.y;
			mat.r[2].m128_f32[2] *= scal.z;

			DirectX::XMStoreFloat3x4((DirectX::XMFLOAT3X4*)pInstanceDesc->Transform, mat);

			//memcpy(&instance_desc.Transform, &mat, sizeof(instance_desc.Transform));
			instanceID++;
			pInstanceDesc++;
		}

		blasStartIndex += blas.second.nGeometries * DXRShaderCommon::N_RAY_TYPES;
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
	auto bufferIndex = m_d3d12->GetGPUBufferIndex();

	//=======Retrive the vertexbuffer========
	//D3D12_VERTEX_BUFFER_VIEW* bufferView = vb_pos->GetView();
	
	//=======Describe the geometry========
	D3D12_RAYTRACING_GEOMETRY_DESC geomDesc[MAX_NUM_GEOMETRIES_IN_BLAS] = {};

	std::unordered_map<std::string, std::unordered_map<Mesh::VertexBufferFlag, D3D12VertexBuffer*>>& objects = static_cast<D3D12Mesh*>(item.blueprint->mesh)->GetSubObjects();
	unsigned int nObjects = min(objects.size(), MAX_NUM_GEOMETRIES_IN_BLAS);

	int i = 0;
	for (auto& e : objects)
	{
		D3D12VertexBuffer* vb_pos = e.second[Mesh::VERTEX_BUFFER_FLAG_POSITION];
		D3D12_VERTEX_BUFFER_VIEW* bufferView = vb_pos->GetView();

		if (i == nObjects) {
			break;
		}
		geomDesc[i].Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;

		//vertexbuffer
		geomDesc[i].Triangles.VertexBuffer.StartAddress = bufferView->BufferLocation;
		geomDesc[i].Triangles.VertexBuffer.StrideInBytes = bufferView->StrideInBytes;
		geomDesc[i].Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT; //Todo: Make this changable
		geomDesc[i].Triangles.VertexCount = vb_pos->GetNumberOfElements();
		//Indexbuffer
		geomDesc[i].Triangles.IndexBuffer = NULL;
		geomDesc[i].Triangles.IndexFormat = DXGI_FORMAT_UNKNOWN;
		geomDesc[i].Triangles.IndexCount = 0;

		geomDesc[i].Triangles.Transform3x4 = 0;
		//geomDesc[i].Flags = (true) ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION; //Todo: Make this changable		
		
		if (!item.blueprint->allGeometryIsOpaque && m_allowAnyhitshaders) {
			geomDesc[i].Flags = (item.blueprint->alphaTested[i]) ? D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION : D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE; //Todo: Make this changable		
		}
		else {
			geomDesc[i].Flags = D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
		}
		i++;
	}

	//=======Describe the BLAS========
	D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_INPUTS inputs = {};
	inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
	inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
	inputs.pGeometryDescs = geomDesc;
	inputs.NumDescs = nObjects;
	inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_NONE; //Todo: Make this changable

	//=======BLAS PREBUILD========
	D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO asBuildInfo = {};
	m_d3d12->GetDevice()->GetRaytracingAccelerationStructurePrebuildInfo(&inputs, &asBuildInfo);

	//=======Initialize BLAS Data========
	BottomLayerData blasData = {};
	blasData.nGeometries = nObjects;
	blasData.items.emplace_back(PerInstance{ item.transform });
	for (size_t i = 0; i < nObjects; i++)
	{
		blasData.geometryBuffers[i] = geomDesc[i].Triangles.VertexBuffer.StartAddress;
	}
	AccelerationStructureBuffers& asBuff = blasData.as;

	//=======Allocate GPU memory========
	//TODO: reuse old unused allocations.
	asBuff.scratch = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), asBuildInfo.ScratchDataSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12Utils::sDefaultHeapProps);
	asBuff.scratch->SetName(L"BLAS_SCRATCH");
	asBuff.result = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), asBuildInfo.ResultDataMaxSizeInBytes, D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE, D3D12Utils::sDefaultHeapProps);
	asBuff.result->SetName(L"BLAS_RESULT");

	m_BLAS_buffers[bufferIndex].insert({ item.blueprint, blasData });

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
	DXRUtils::ShaderTableBuilder missTable(2, m_rtxPipelineState);
	missTable.AddShader(m_shader_missName);
	missTable.AddShader(m_shader_shadowMissName);
	m_shaderTable_miss[bufferIndex].Release();
	m_shaderTable_miss[bufferIndex] = missTable.Build(m_d3d12->GetDevice());

	//===Update HitGroup Table===
	DXRUtils::ShaderTableBuilder hitGroupTable(m_hitGroupShaderRecordsNeededThisFrame * DXRShaderCommon::N_RAY_TYPES, m_rtxPipelineState, 64);

	UINT blasIndex = 0;
	D3D12_GPU_DESCRIPTOR_HANDLE texture_gdh = m_srv_mesh_textures_handle_start.gdh;
	for (auto& blas : m_BLAS_buffers[bufferIndex])
	{
		D3D12Mesh* mesh = static_cast<D3D12Mesh*>(blas.first->mesh);
		std::unordered_map<std::string, std::unordered_map<Mesh::VertexBufferFlag, D3D12VertexBuffer*>>& objects = mesh->GetSubObjects();
		uint i = 0;
		for (auto& e : objects)
		{
			UINT64 vb_pos =    e.second[Mesh::VERTEX_BUFFER_FLAG_POSITION        ]->GetResource()->GetGPUVirtualAddress();
			UINT64 vb_norm =   e.second[Mesh::VERTEX_BUFFER_FLAG_NORMAL          ]->GetResource()->GetGPUVirtualAddress();
			UINT64 vb_uv =     e.second[Mesh::VERTEX_BUFFER_FLAG_UV              ]->GetResource()->GetGPUVirtualAddress();
			UINT64 vb_tan_Bi = e.second[Mesh::VERTEX_BUFFER_FLAG_TANGENT_BINORMAL]->GetResource()->GetGPUVirtualAddress();

			//===Add Shader(group) Identifier===
			//TODO: identify which geometry need none opaque
			
			hitGroupTable.AddShader((blas.first->alphaTested[i]) ? m_group_group_alphaTest : m_group_group1);
			//if (m_allowAnyhitshaders) {
			//}
			//else {
			//	hitGroupTable.AddShader(m_group_group1);
			//}

			//===Add vertexbuffer descriptors===
			hitGroupTable.AddDescriptor(vb_pos, blasIndex);
			hitGroupTable.AddDescriptor(vb_norm, blasIndex);
			hitGroupTable.AddDescriptor(vb_uv, blasIndex);
			hitGroupTable.AddDescriptor(vb_tan_Bi, blasIndex);

			//===Add texture descriptors===
			hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex);
			texture_gdh.ptr += m_descriptorSize;
			hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex);
			texture_gdh.ptr += m_descriptorSize;

			//===ShadowRayHit Shader===
			if (blas.first->alphaTested[i] && m_allowAnyhitshaders) {
				//Alphatest geometry shadow hit shader
				texture_gdh.ptr -= m_descriptorSize * 2;
				hitGroupTable.AddShader(m_group_group_alphaTest_shadow);

				//===Add vertexbuffer descriptors===
				hitGroupTable.AddDescriptor(vb_pos,		blasIndex + 1);
				hitGroupTable.AddDescriptor(vb_norm,	blasIndex + 1);
				hitGroupTable.AddDescriptor(vb_uv,		blasIndex + 1);
				hitGroupTable.AddDescriptor(vb_tan_Bi,	blasIndex + 1);

				//===Add texture descriptors===
				hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex + 1);
				texture_gdh.ptr += m_descriptorSize;
				hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex + 1);
				texture_gdh.ptr += m_descriptorSize;
			}
			else {
				//Opaque geometry dont need a shadow hit shader
				hitGroupTable.AddShader(L"NULL");
			}

			blasIndex += DXRShaderCommon::N_RAY_TYPES;
			i++;
			if (i == blas.second.nGeometries) {
				break;
			}
		}
	}

	m_shaderTable_hitgroup[bufferIndex].Release();
	m_shaderTable_hitgroup[bufferIndex] = hitGroupTable.Build(m_d3d12->GetDevice());
}

void DXRBase::UpdateDescriptorHeap(ID3D12GraphicsCommandList4* cmdList)
{
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
	m_unused_handle_start_this_frame = m_unreserved_handle_start[bufferIndex];
	m_srv_mesh_textures_handle_start = m_unused_handle_start_this_frame;
	D3D12_CPU_DESCRIPTOR_HANDLE texture_cpu; 
	for (auto& e : m_BLAS_buffers[bufferIndex])
	{
		UINT nTextures = e.first->textures.size();
		for (size_t i = 0; i < e.second.nGeometries; i++)
		{
			texture_cpu = m_d3d12->GetTextureLoader()->GetSpecificTextureCPUAdress(static_cast<D3D12Texture*>(e.first->textures[i * 2]));
			m_d3d12->GetDevice()->CopyDescriptorsSimple(1, m_unused_handle_start_this_frame.cdh, texture_cpu, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_unused_handle_start_this_frame += m_descriptorSize;

			texture_cpu = m_d3d12->GetTextureLoader()->GetSpecificTextureCPUAdress(static_cast<D3D12Texture*>(e.first->textures[i * 2 + 1]));
			m_d3d12->GetDevice()->CopyDescriptorsSimple(1, m_unused_handle_start_this_frame.cdh, texture_cpu, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_unused_handle_start_this_frame += m_descriptorSize;
		}
	}
}

void DXRBase::createInitialShaderResources(bool remake)
{
}

bool DXRBase::CreateRaytracingPSO(std::vector<ShaderDefine>* _defines)
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

	bool allowAnyhit = true;
	if (_defines) {
		for (auto& e : *_defines)
		{
			if (e.define == L"RAY_GEN_ALPHA_TEST" || e.define == L"CLOSEST_HIT_ALPHA_TEST_1" || e.define == L"CLOSEST_HIT_ALPHA_TEST_2") {
				allowAnyhit = false;
			}
			defines.push_back({ e.define.c_str(), e.value.c_str() });
		}
	}

#ifdef DO_TESTING
	SetAllowAnyHitShader(allowAnyhit);
#endif // DO_TESTING


	psoBuilder.AddLibrary("../Shaders/D3D12/DXR/test.hlsl", { m_shader_rayGenName, m_shader_closestHitName, m_shader_closestHitAlphaTestName, m_shader_anyHitName, m_shader_missName, m_shader_shadowMissName}, defines);
	psoBuilder.AddHitGroup(m_group_group1, m_shader_closestHitName);
	psoBuilder.AddHitGroup(m_group_group_alphaTest, m_shader_closestHitAlphaTestName, (m_allowAnyhitshaders) ? m_shader_anyHitName : nullptr);
	psoBuilder.AddHitGroup(m_group_group_alphaTest_shadow, nullptr, m_shader_anyHitName);

	psoBuilder.AddSignatureToShaders({ m_shader_rayGenName },     m_localRootSignature_rayGen.Get());
	psoBuilder.AddSignatureToShaders({ m_group_group1 },          m_localRootSignature_hitGroups.Get());
	psoBuilder.AddSignatureToShaders({ m_group_group_alphaTest }, m_localRootSignature_hitGroups.Get());
	psoBuilder.AddSignatureToShaders({ m_shader_missName },       m_localRootSignature_miss.Get());
	psoBuilder.AddSignatureToShaders({ m_shader_shadowMissName},  m_localRootSignature_empty.Get());
	psoBuilder.AddSignatureToShaders({ m_group_group_alphaTest_shadow }, m_localRootSignature_hitGroups.Get());

	psoBuilder.SetMaxPayloadSize(payloadSize);
	psoBuilder.SetMaxAttributeSize(sizeof(float) * 4);
	psoBuilder.SetMaxRecursionDepth(DXRShaderCommon::MAX_RAY_RECURSION_DEPTH);
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

#ifdef DO_TESTING

double* DXRBase::GetGPU_Timers(int& nValues)
{
	m_d3d12->WaitForGPU_ALL();

	while (m_nUnExtractedTimerValues > 0)
	{
		ExtractGPUTimer();
	}

	nValues = N_TIMER_VALUES;
	return m_timerValue;
}

double DXRBase::ExtractGPUTimer()
{
	if (m_nUnExtractedTimerValues <= 0) {
		return 0;
	}

	m_nUnExtractedTimerValues--;
	UINT bufferIndex = m_d3d12->GetGPUBufferIndex();

	UINT64 queueFreq;
	m_d3d12->GetDirectCommandQueue()->GetTimestampFrequency(&queueFreq);
	double timestampToMs = (1.0 / queueFreq) * 1000.0;

	FR::GPUTimestampPair drawTime = m_gpuTimer.GetTimestampPair(bufferIndex);

	UINT64 dt = drawTime.Stop - drawTime.Start;
	double timeInMs = dt * timestampToMs;

	m_timerValue[m_nextTimerIndex++] = timeInMs;
	m_nextTimerIndex %= N_TIMER_VALUES;

	return timeInMs;
}

#endif //DO_TESTING

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
	m_localRootSignature_hitGroups.AddSRV("VertexBuffer_POS", 1, 0);
	m_localRootSignature_hitGroups.AddSRV("VertexBuffer_Norm", 1, 1);
	m_localRootSignature_hitGroups.AddSRV("VertexBuffer_UV", 1, 2);
	m_localRootSignature_hitGroups.AddSRV("VertexBuffer_TAN_BI", 1, 3);
	m_localRootSignature_hitGroups.AddDescriptorTable("AlbedoColor", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
	m_localRootSignature_hitGroups.AddDescriptorTable("Normalmap", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 1);
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
