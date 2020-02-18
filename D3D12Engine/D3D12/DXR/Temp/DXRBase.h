#pragma once

#include "DXRUtils.h"
#include "..\..\..\Math.hpp"
#include "..\..\D3D12API.hpp"
#include "..\..\D3D12Camera.hpp"
#include "..\..\D3D12Window.hpp"
#include "..\..\Renderers\D3D12Renderer.h"
#include <DirectXMath.h>
#include "../../../../Shaders/D3D12/DXR/Common_hlsl_cpp.hlsli"
#include "..\..\..\Light\LightSource.h"

#include <vector>
#include <Windows.h>
#include <unordered_map>

typedef void* _D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS;
typedef unsigned int BLAS_ID;
constexpr UINT NUM_DESCRIPTORS_PER_GPU_BUFFER = 5000;
constexpr UINT NUM_DESCRIPTORS_TOTAL = NUM_DESCRIPTORS_PER_GPU_BUFFER * NUM_GPU_BUFFERS;


//#include "../DX12API.h"
//#include "DXRUtils.h"
//#include "../DX12Utils.h"
//#include "Sail/api/Renderer.h"
//#include "../shader/DX12ConstantBuffer.h"
//#include "../shader/DX12StructuredBuffer.h"
//#include "API/DX12/resources/DX12RenderableTexture.h"
// Include defines shared with dxr shaders
//#include "Sail/../../SPLASH/res/shaders/dxr/Common_hlsl_cpp.hlsl"

//TODO: allow dynamic value
constexpr int MAX_NUM_GEOMETRIES_IN_BLAS = 20;

class DXRBase {
public:
	DXRBase(D3D12API* d3d12);
	~DXRBase();

	bool Initialize();
	void SetOutputResources(ID3D12Resource** output, Int2 dim);
	void UpdateAccelerationStructures(std::vector<SubmissionItem>& items, ID3D12GraphicsCommandList4* cmdList);

	void UpdateSceneData(D3D12Camera* camera, const std::vector<LightSource>& lights);
	void Dispatch(ID3D12GraphicsCommandList4* cmdList);
	void ReloadShaders(std::vector<ShaderDefine>* defines);

	//Settings
	void SetAllowAnyHitShader(bool b);

private:
	struct AccelerationStructureBuffers {
		ID3D12Resource* scratch = nullptr;
		ID3D12Resource* result = nullptr;
		ID3D12Resource* instanceDesc = nullptr;    // Used only for top-level AS
		bool allowUpdate = false;
		void Release();
	};

	struct PerInstance {
		Transform transform;
	};

	struct BottomLayerData {
		bool needsRebuild = false;
		AccelerationStructureBuffers as;
		//number of geometries(meshes) inside the BLAS
		uint nGeometries;
		D3D12_GPU_VIRTUAL_ADDRESS geometryBuffers[MAX_NUM_GEOMETRIES_IN_BLAS];
		std::vector<PerInstance> items;

		BottomLayerData& operator =(const BottomLayerData& other) {
			as = other.as;
			needsRebuild = other.needsRebuild;
			nGeometries = other.nGeometries;
			for (size_t i = 0; i < nGeometries; i++)
			{
				geometryBuffers[i] = other.geometryBuffers[i];
			}
			items = other.items;

			return *this;
		}
	};

private:
	bool InitializeRootSignatures();
	bool InitializeConstanBuffers();

	// Acceleration structures
	void CreateTLAS(unsigned int numInstanceDescriptors, ID3D12GraphicsCommandList4* cmdList);
	void CreateBLAS(const SubmissionItem& renderCommand, _D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers* sourceBufferForUpdate = nullptr);
	void UpdateShaderTable();

	void UpdateDescriptorHeap(ID3D12GraphicsCommandList4* cmdList);
	void createInitialShaderResources(bool remake = false);

	// Root signature creation
	// TODO: create them dynamically after parsing the shader source (like ShaderPipeline does)
	bool CreateDXRGlobalRootSignature();
	bool CreateRayGenLocalRootSignature();
	bool CreateHitGroupLocalRootSignature();
	bool CreateMissLocalRootSignature();
	bool CreateEmptyLocalRootSignature();
	bool CreateRaytracingPSO(std::vector<ShaderDefine>* defines);
	bool CreateDescriptorHeap();

	D3D12_GPU_DESCRIPTOR_HANDLE GetCurrentDescriptorHandle();

private:
	D3D12API* m_d3d12;
	Int2 m_outputDim;

	AccelerationStructureBuffers m_TLAS_buffers[NUM_GPU_BUFFERS];
	std::unordered_map<Blueprint*, BottomLayerData> m_BLAS_buffers[NUM_GPU_BUFFERS];

	D3D12Utils::RootSignature m_globalRootSignature;
	//TODO: generate local signatures
	D3D12Utils::RootSignature m_localRootSignature_rayGen;
	D3D12Utils::RootSignature m_localRootSignature_miss;
	D3D12Utils::RootSignature m_localRootSignature_hitGroups;
	D3D12Utils::RootSignature m_localRootSignature_empty;

	ID3D12StateObject* m_rtxPipelineState = nullptr;

	//==Descriptor Heap==
	UINT m_descriptorSize;
	ID3D12DescriptorHeap* m_descriptorHeap = nullptr;
	D3D12Utils::D3D12_DESCRIPTOR_HANDLE_BUFFERED m_descriptorHeap_start;
	D3D12Utils::D3D12_DESCRIPTOR_HANDLE_BUFFERED m_unreserved_handle_start;
	D3D12Utils::D3D12_DESCRIPTOR_HANDLE m_unused_handle_start_this_frame;
	D3D12Utils::D3D12_DESCRIPTOR_HANDLE m_srv_mesh_textures_handle_start;

	UINT m_numReservedDescriptors = 0;
	D3D12Utils::D3D12_DESCRIPTOR_HANDLE_BUFFERED m_uav_output_texture_handle;
	D3D12Utils::D3D12_DESCRIPTOR_HANDLE_BUFFERED m_cbv_scene_handle;

	//==Constantbuffers==
	ID3D12Resource* m_cb_scene = nullptr;
	UINT m_cb_scene_buffer_size;

	//==ShaderTables==
	DXRUtils::ShaderTableData m_shaderTable_gen[NUM_GPU_BUFFERS];
	DXRUtils::ShaderTableData m_shaderTable_miss[NUM_GPU_BUFFERS];
	DXRUtils::ShaderTableData m_shaderTable_hitgroup[NUM_GPU_BUFFERS];

	uint m_hitGroupShaderRecordsNeededThisFrame;

	//==Shader Names==
	const WCHAR* m_shader_rayGenName = L"rayGen";
	const WCHAR* m_shader_closestHitName = L"closestHitTriangle";
	const WCHAR* m_shader_closestHitAlphaTestName = L"closestHitAlphaTest";
	const WCHAR* m_shader_anyHitName = L"anyHitAlphaTest";
	const WCHAR* m_shader_missName = L"miss";

	const WCHAR* m_shader_shadowMissName = L"shadow_GeometryMiss";

	//==Shader Group Names==
	const WCHAR* m_group_group1 = L"hitGroup";
	const WCHAR* m_group_group_alphaTest = L"hitGroup_alphaTest";
	const WCHAR* m_group_group_alphaTest_shadow = L"hitGroup_alphaTest_shadow";

	//Settings
	bool m_allowAnyhitshaders = true;
	bool m_forceBLASRebuild[NUM_GPU_BUFFERS] = { false };
};