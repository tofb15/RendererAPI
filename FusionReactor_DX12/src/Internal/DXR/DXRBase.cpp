#include "stdafx.h"

#include "DXRBase.h"
#include "FusionReactor/src/Material.hpp"
#include "Shaders\D3D12\DXR\Common_hlsl_cpp.hlsli"
#include "..\..\D3D12Mesh.hpp"
#include "..\D3D12VertexBuffer.hpp"
#include "..\..\D3D12Window.hpp"
#include "..\..\D3D12Texture.hpp"
#include "..\..\D3D12Technique.hpp"
#include "..\..\D3D12ShaderManager.hpp"
#include "..\..\D3D12Material.hpp"
#include "..\D3D12DescriptorHeapManager.hpp"
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		DXRBase::DXRBase(D3D12API* d3d12) : m_d3d12(d3d12) {

		}

		DXRBase::~DXRBase() {
			for (auto& e : m_shaderTable_gen) {
				e.Release();
			}
			for (auto& e : m_shaderTable_hitgroup) {
				e.Release();
			}
			for (auto& e : m_shaderTable_miss) {
				e.Release();
			}
			for (auto& e : m_TLAS_buffers) {
				e.Release();
			}
			for (auto& e : m_BLAS_buffers) {
				for (auto& e2 : e) {
					e2.second.as.Release();
				}
			}

			if (m_cb_scene) {
				m_cb_scene->Release();
			}
		}

		bool DXRBase::Initialize() {
			m_descriptorHeap_CBV_SRV_UAV = m_d3d12->GetDescriptorHeapManager()->GetDescriptorHeap(DESCRIPTOR_TYPE_CBV_SRV_UAV);

			m_uav_output_texture_handles = m_descriptorHeap_CBV_SRV_UAV->GetStaticRange().AllocateSlots(NUM_GPU_BUFFERS);
			m_cbv_scene_handles = m_descriptorHeap_CBV_SRV_UAV->GetStaticRange().AllocateSlots(NUM_GPU_BUFFERS);

			if (!InitializeConstanBuffers()) {
				return false;
			}

#ifdef PERFORMANCE_TESTING
			m_gpuTimer.Init(m_d3d12->GetDevice(), NUM_GPU_BUFFERS);
#endif // PERFORMANCE_TESTING

			return true;
		}

		void DXRBase::SetOutputResources(ID3D12Resource** output, Int2 dim) {
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			m_outputDim = dim;

			for (int i = 0; i < NUM_GPU_BUFFERS; i++) {
				m_d3d12->GetDevice()->CreateUnorderedAccessView(output[i], nullptr, &uavDesc, m_uav_output_texture_handles.GetCPUHandle(i));
			}
		}

		void DXRBase::UpdateAccelerationStructures(std::vector<SubmissionItem>& items, ID3D12GraphicsCommandList4* cmdList) {
			UINT gpuBuffer = m_d3d12->GetGPUBufferIndex();

			for (auto& e : m_BLAS_buffers[gpuBuffer]) {
				e.second.items.clear();
				if (e.first->hasChanged) {
					e.first->hasChanged = false;
					for (size_t i = 0; i < NUM_GPU_BUFFERS; i++) {
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
				for (auto it = m_BLAS_buffers[gpuBuffer].begin(); it != m_BLAS_buffers[gpuBuffer].end();) {
					it->second.as.Release();
					it = m_BLAS_buffers[gpuBuffer].erase(it);
				}
			}

			//Build/Update BLAS
			for (auto& e : items) {
				//BLAS_ID id = static_cast<D3D12Mesh*>(e.blueprint->mesh)->GetID();
				auto search = m_BLAS_buffers[gpuBuffer].find(e.blueprint);
				if (search == m_BLAS_buffers[gpuBuffer].end()) {
					CreateBLAS(e, 0, cmdList);
				} else {
					if (search->second.needsRebuild) {
						search->second.as.Release();
						m_BLAS_buffers[gpuBuffer].erase(search);
						CreateBLAS(e, 0, cmdList);
					} else {
						//Insert instance	
						search->second.items.emplace_back(PerInstance{ e.transform, e.renderflag });
					}
				}
			}

			m_hitGroupShaderRecordsNeededThisFrame = 0;
			//Destroy unused BLASs
			for (auto it = m_BLAS_buffers[gpuBuffer].begin(); it != m_BLAS_buffers[gpuBuffer].end();) {
				if (it->second.items.empty()) {
					it->second.as.Release();
					it = m_BLAS_buffers[gpuBuffer].erase(it);
				} else {
					m_hitGroupShaderRecordsNeededThisFrame += it->second.nGeometries;
					++it;
				}
			}

			//Build/Update TLAS
			CreateTLAS(items.size(), cmdList);
		}

		void DXRBase::UpdateSceneData(D3D12Camera* camera, const std::vector<LightSource>& lights) {
			UINT bufferIndex = m_d3d12->GetGPUBufferIndex();

			HRESULT hr;
			char* gpuData;

			D3D12_RANGE writeRange = {};
			writeRange.Begin = (size_t)(m_cb_scene_buffer_size * bufferIndex);
			writeRange.End = writeRange.Begin + sizeof(DXRShaderCommon::SceneCBuffer);

			//Calculate Invers of ViewProjection matrix
			DirectX::XMMATRIX viewProj_inv = DirectX::XMLoadFloat4x4(&camera->GetViewPerspective());
			//viewProj_inv = DirectX::XMMatrixTranspose(viewProj_inv);
			viewProj_inv = (DirectX::XMMatrixInverse(nullptr, viewProj_inv));

			//Update scene constantbuffer
			hr = m_cb_scene->Map(0, nullptr, (void**)& gpuData);
			if (FAILED(hr)) {
				return;
			}

			gpuData += writeRange.Begin;
			DXRShaderCommon::SceneCBuffer* sceneData = (DXRShaderCommon::SceneCBuffer*)gpuData;

			sceneData->cameraPosition = camera->GetPosition();
			DirectX::XMStoreFloat4x4(&sceneData->projectionToWorld, viewProj_inv);

			if (!lights.empty()) {
				int i = 0;
				for (auto& e : lights) {
					if (!e.m_enabled) {
						continue;
					}
					sceneData->pLight[i].position = e.m_position_center;
					sceneData->pLight[i].reachRadius = e.m_reachRadius;
					sceneData->pLight[i++].color = e.m_color;

					if (i == DXRShaderCommon::MAX_LIGHTS) {
						break;
					}
				}
				sceneData->nLights = i;
			} else {
				sceneData->pLight[0].position = Float3(-10, 50, -10);
			}

			m_cb_scene->Unmap(0, nullptr);
		}

		void DXRBase::Dispatch(ID3D12GraphicsCommandList4* cmdList, D3D12ShaderManager* sm) {
			static unsigned int counter = 0;
			D3D12_DISPATCH_RAYS_DESC desc = {};
			UINT bufferIndex = m_d3d12->GetGPUBufferIndex();

			desc.RayGenerationShaderRecord.StartAddress = m_shaderTable_gen[bufferIndex].resource->GetGPUVirtualAddress();
			desc.RayGenerationShaderRecord.SizeInBytes = m_shaderTable_gen[bufferIndex].sizeInBytes;

			desc.MissShaderTable.StartAddress = m_shaderTable_miss[bufferIndex].resource->GetGPUVirtualAddress();
			desc.MissShaderTable.SizeInBytes = m_shaderTable_miss[bufferIndex].sizeInBytes;
			desc.MissShaderTable.StrideInBytes = m_shaderTable_miss[bufferIndex].strideInBytes;

			desc.HitGroupTable.StartAddress = m_shaderTable_hitgroup[bufferIndex].resource->GetGPUVirtualAddress();
			desc.HitGroupTable.SizeInBytes = m_shaderTable_hitgroup[bufferIndex].sizeInBytes;
			desc.HitGroupTable.StrideInBytes = m_shaderTable_hitgroup[bufferIndex].strideInBytes;

			desc.Width = m_outputDim.x;
			desc.Height = m_outputDim.y;
			desc.Depth = 1;

			cmdList->SetComputeRootSignature(sm->m_globalRootSignature);
			cmdList->SetComputeRootShaderResourceView(0, m_TLAS_buffers[bufferIndex].result->GetGPUVirtualAddress());
			cmdList->SetComputeRootConstantBufferView(1, m_cb_scene->GetGPUVirtualAddress());

			ID3D12DescriptorHeap* descriptorHeaps[] = { m_descriptorHeap_CBV_SRV_UAV->GetDescriptorHeap() };
			cmdList->SetDescriptorHeaps(1, descriptorHeaps);
			cmdList->SetPipelineState1(sm->m_rtxPipelineState);

#ifdef PERFORMANCE_TESTING
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
#endif // PERFORMANCE_TESTING
			}

		void DXRBase::SetAllowAnyHitShader(bool b) {
			if (m_allowAnyhitshaders != b) {
				for (size_t i = 0; i < NUM_GPU_BUFFERS; i++) {
					m_forceBLASRebuild[i] = true;
				}
			}
			m_allowAnyhitshaders = b;
		}

		bool DXRBase::InitializeConstanBuffers() {
			UINT alignTo = D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT;
			//Scene
			m_cb_scene_buffer_size = sizeof(DXRShaderCommon::SceneCBuffer);
			UINT padding = (alignTo - (m_cb_scene_buffer_size % alignTo)) % alignTo;
			m_cb_scene_buffer_size += padding;
			m_cb_scene = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), ((UINT64)m_cb_scene_buffer_size * NUM_GPU_BUFFERS), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12Utils::sUploadHeapProperties);

			D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
			cbvDesc.SizeInBytes = m_cb_scene_buffer_size;
			for (unsigned int i = 0; i < NUM_GPU_BUFFERS; i++) {
				cbvDesc.BufferLocation = m_cb_scene->GetGPUVirtualAddress() + (size_t)i * cbvDesc.SizeInBytes;
				m_d3d12->GetDevice()->CreateConstantBufferView(&cbvDesc, m_cbv_scene_handles.GetCPUHandle(i));
			}
			return true;
		}

		void DXRBase::CreateTLAS(unsigned int numInstanceDescriptors, ID3D12GraphicsCommandList4* cmdList) {
			UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
			AccelerationStructureBuffers& tlas = m_TLAS_buffers[bufferIndex];
			tlas.Release(); //TODO: reuse TLAS insteed of destroying it.


			//=======Allocate GPU memory for Instance Data========
			tlas.instanceDesc = D3D12Utils::CreateBuffer(m_d3d12->GetDevice(), sizeof(D3D12_RAYTRACING_INSTANCE_DESC) * max(numInstanceDescriptors, 1U), D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12Utils::sUploadHeapProperties);
			tlas.instanceDesc->SetName(L"TLAS_INSTANCE_DESC");

			//=======Describe the instances========
			D3D12_RAYTRACING_INSTANCE_DESC* pInstanceDesc;
			tlas.instanceDesc->Map(0, nullptr, (void**)& pInstanceDesc);
			DirectX::XMMATRIX mat;
			UINT blasStartIndex = 0;
			for (auto& blas : m_BLAS_buffers[bufferIndex]) {
				auto& instances = blas.second.items;
				int instanceID = 0;
				for (auto& instance : instances) {
					pInstanceDesc->InstanceID = instanceID;
					pInstanceDesc->InstanceMask = 0xFF; //Todo: make this changable
					if ((instance.renderflags & (int)RenderFlag::Dont_Cast_Shadows)) {
						pInstanceDesc->InstanceMask &= ~DXRShaderCommon::CASTING_SHADOW_FLAG;
					}
					pInstanceDesc->InstanceContributionToHitGroupIndex = blasStartIndex;
					pInstanceDesc->Flags = D3D12_RAYTRACING_INSTANCE_FLAG_NONE;
					pInstanceDesc->AccelerationStructure = blas.second.as.result->GetGPUVirtualAddress();

					// Construct and copy matrix data
					Float3& pos = instance.transform.pos;
					Float3& rot = instance.transform.rotation;
					Float3& scal = instance.transform.scale;

					//mat = DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
					//mat = DirectX::XMMatrixIdentity();
					mat = DirectX::XMMatrixRotationRollPitchYaw(rot.x, rot.y, rot.z);
					mat *= DirectX::XMMatrixScaling(scal.x, scal.y, scal.z);
					mat *= DirectX::XMMatrixTranslation(pos.x, pos.y, pos.z);
					//mat.r[3] = { pos.x, pos.y, pos.z, 1.0f };
					//mat.r[0].m128_f32[0] *= scal.x;
					//mat.r[1].m128_f32[1] *= scal.y;
					//mat.r[2].m128_f32[2] *= scal.z;

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

		void DXRBase::CreateBLAS(const SubmissionItem& item, _D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers* sourceBufferForUpdate) {
			auto bufferIndex = m_d3d12->GetGPUBufferIndex();

			//=======Retrive the vertexbuffer========
			//D3D12_VERTEX_BUFFER_VIEW* bufferView = vb_pos->GetView();

			//=======Describe the geometry========
			D3D12_RAYTRACING_GEOMETRY_DESC geomDesc[MAX_NUM_GEOMETRIES_IN_BLAS] = {};

			std::unordered_map<std::string, std::unordered_map<Mesh::VertexBufferFlag, D3D12VertexBuffer*>>& objects = static_cast<D3D12Mesh*>(item.blueprint->mesh)->GetSubObjects();
			unsigned int nObjects = min(objects.size(), MAX_NUM_GEOMETRIES_IN_BLAS);

			int i = 0;
			for (auto& e : objects) {
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

				if (m_allowAnyhitshaders) {
					geomDesc[i].Flags = (((D3D12Material*)item.blueprint->materials[i])->IsOpaque()) ? D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE : D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION; //Todo: Make this changable		
				} else {
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
			blasData.items.emplace_back(PerInstance{ item.transform, item.renderflag });
			for (size_t i = 0; i < nObjects; i++) {
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

		void DXRBase::UpdateShaderTable(D3D12ShaderManager* sm) {
			UINT bufferIndex = m_d3d12->GetGPUBufferIndex();

			//TODO: dont rebuild tables if not needed.
			if (sm->NeedsPSORebuild()) {
				sm->CreateRaytracingPSO({});
			}
			//===Update Generation Table===
			DXRUtils::ShaderTableBuilder generationTable(1, sm->m_rtxPipelineState, 64);
			generationTable.AddShader(sm->GetRaygenShaderIdentifier());

			generationTable.AddDescriptor(m_uav_output_texture_handles.GetGPUHandle(bufferIndex).ptr);
			m_shaderTable_gen[bufferIndex].Release();
			m_shaderTable_gen[bufferIndex] = generationTable.Build(m_d3d12->GetDevice());

			//===Update Miss Table===
			DXRUtils::ShaderTableBuilder missTable(2, sm->m_rtxPipelineState);
			missTable.AddShader(sm->GetMissShaderIdentifier());
			missTable.AddShader(sm->GetShadowMissShaderIdentifier());

			m_shaderTable_miss[bufferIndex].Release();
			m_shaderTable_miss[bufferIndex] = missTable.Build(m_d3d12->GetDevice());

			//===Update HitGroup Table===
			DXRUtils::ShaderTableBuilder hitGroupTable(m_hitGroupShaderRecordsNeededThisFrame * DXRShaderCommon::N_RAY_TYPES, sm->m_rtxPipelineState, 128);

			UINT blasIndex = 0;
			D3D12_GPU_DESCRIPTOR_HANDLE texture_gdh = m_descriptorRange_dynamic_start.gdh;
			size_t descriptorSize = m_descriptorHeap_CBV_SRV_UAV->GetDescriptorSize();

			for (auto& blas : m_BLAS_buffers[bufferIndex]) {
				D3D12Mesh* mesh = static_cast<D3D12Mesh*>(blas.first->mesh);
				std::unordered_map<std::string, std::unordered_map<Mesh::VertexBufferFlag, D3D12VertexBuffer*>>& objects = mesh->GetSubObjects();
				uint i = 0;
				for (auto& e : objects) {
					UINT64 vb_pos = e.second[Mesh::VERTEX_BUFFER_FLAG_POSITION]->GetResource()->GetGPUVirtualAddress();
					UINT64 vb_norm = e.second[Mesh::VERTEX_BUFFER_FLAG_NORMAL]->GetResource()->GetGPUVirtualAddress();
					UINT64 vb_uv = e.second[Mesh::VERTEX_BUFFER_FLAG_UV]->GetResource()->GetGPUVirtualAddress();
					UINT64 vb_tan_Bi = e.second[Mesh::VERTEX_BUFFER_FLAG_TANGENT_BINORMAL]->GetResource()->GetGPUVirtualAddress();

					//===Add Shader(group) Identifier===
					D3D12Material* material = static_cast<D3D12Material*>(blas.first->materials[i]);
					hitGroupTable.AddShader(sm->GetHitGroupIdentifier(material->GetShaderProgram()));

					//===Add vertexbuffer descriptors===
					hitGroupTable.AddDescriptor(vb_pos, blasIndex);
					hitGroupTable.AddDescriptor(vb_norm, blasIndex);
					hitGroupTable.AddDescriptor(vb_uv, blasIndex);
					hitGroupTable.AddDescriptor(vb_tan_Bi, blasIndex);

					//===Add texture descriptors===
					hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex);
					texture_gdh.ptr += descriptorSize;
					hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex);
					texture_gdh.ptr += descriptorSize;
					hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex);
					texture_gdh.ptr += descriptorSize;
					hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex);
					texture_gdh.ptr += descriptorSize;

					//===ShadowRayHit Shader===
					//TODO:: Fix shadow for alpha tested geometry
					if (!material->IsOpaque() && m_allowAnyhitshaders) {
						//Alphatest geometry shadow hit shader
						hitGroupTable.AddShader(sm->GetHitGroupIdentifier(material->GetShaderProgram() + 1));

						//===Add vertexbuffer descriptors===
						hitGroupTable.AddDescriptor(vb_pos, blasIndex + 1);
						hitGroupTable.AddDescriptor(vb_norm, blasIndex + 1);
						hitGroupTable.AddDescriptor(vb_uv, blasIndex + 1);
						hitGroupTable.AddDescriptor(vb_tan_Bi, blasIndex + 1);

						//===Add texture descriptors===
						texture_gdh.ptr -= (UINT64)(descriptorSize * 4U);
						hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex + 1);
						texture_gdh.ptr += descriptorSize;
						hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex + 1);
						texture_gdh.ptr += descriptorSize;
						hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex + 1);
						texture_gdh.ptr += descriptorSize;
						hitGroupTable.AddDescriptor(texture_gdh.ptr, blasIndex + 1);
						texture_gdh.ptr += descriptorSize;
					} else {
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

		void DXRBase::UpdateDescriptorHeap(ID3D12GraphicsCommandList4* cmdList) {
			UINT bufferIndex = m_d3d12->GetGPUBufferIndex();

			D3D12_CPU_DESCRIPTOR_HANDLE texture_cpu;
			D3D12ResourceView& dynamicDescriptorRange = m_descriptorHeap_CBV_SRV_UAV->GetDynamicRange();
			size_t descriptorSize = dynamicDescriptorRange.GetDescriptorSize();
			m_descriptorRange_dynamic_start = dynamicDescriptorRange.GetNextHandle();

			for (auto& e : m_BLAS_buffers[bufferIndex]) {
				for (size_t i = 0; i < e.second.nGeometries; i++) {
					texture_cpu = m_d3d12->GetTextureLoader()->GetSpecificTextureCPUAdress(static_cast<D3D12Texture*>(e.first->materials[i]->m_materialData.pbrData.color));
					m_d3d12->GetDevice()->CopyDescriptorsSimple(1, dynamicDescriptorRange.AllocateSlot().cdh, texture_cpu, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

					texture_cpu = m_d3d12->GetTextureLoader()->GetSpecificTextureCPUAdress(static_cast<D3D12Texture*>(e.first->materials[i]->m_materialData.pbrData.normal));
					m_d3d12->GetDevice()->CopyDescriptorsSimple(1, dynamicDescriptorRange.AllocateSlot().cdh, texture_cpu, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

					texture_cpu = m_d3d12->GetTextureLoader()->GetSpecificTextureCPUAdress(static_cast<D3D12Texture*>(e.first->materials[i]->m_materialData.pbrData.roughness));
					m_d3d12->GetDevice()->CopyDescriptorsSimple(1, dynamicDescriptorRange.AllocateSlot().cdh, texture_cpu, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

					texture_cpu = m_d3d12->GetTextureLoader()->GetSpecificTextureCPUAdress(static_cast<D3D12Texture*>(e.first->materials[i]->m_materialData.pbrData.metalness));
					m_d3d12->GetDevice()->CopyDescriptorsSimple(1, dynamicDescriptorRange.AllocateSlot().cdh, texture_cpu, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
				}
			}
		}

		void DXRBase::createInitialShaderResources(bool remake) {
		}

#ifdef PERFORMANCE_TESTING

		double* DXRBase::GetGPU_Timers(int& nValues, int& firstValue) {
			m_d3d12->WaitForGPU_ALL();

			while (m_nUnExtractedTimerValues > 0) {
				ExtractGPUTimer();
			}

			nValues = N_TIMER_VALUES;
			firstValue = m_nextTimerIndex;

			return m_timerValue;
		}

		double DXRBase::ExtractGPUTimer() {
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

#endif //PERFORMANCE_TESTING

		void DXRBase::AccelerationStructureBuffers::Release() {
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
		}
	}