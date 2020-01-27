#pragma once

#include "DXRUtils.h"
#include "..\..\..\Math.hpp"
#include "..\..\D3D12API.hpp"
//#include "..\..\D3D12_FDecl.h"
#include "..\..\Renderers\D3D12Renderer.h"

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

class DXRBase {
public:
	DXRBase(D3D12API* d3d12);
	~DXRBase();

	bool Initialize();
	void SetOutputDescriptor(const D3D12_CPU_DESCRIPTOR_HANDLE& outputDescriptor);
	void UpdateAccelerationStructures(std::vector<SubmissionItem>& items, ID3D12GraphicsCommandList4* cmdList);

	void UpdateSceneData();
	void Dispatch(ID3D12GraphicsCommandList4* cmdList);
	void ReloadShaders();

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
		AccelerationStructureBuffers as;
		std::vector<PerInstance> items;
	};

	struct RayPayload {
		Float4 color;
		UINT recursionDepth;
	};

private:
	bool InitializeRootSignatures();

	// Acceleration structures
	void CreateTLAS(unsigned int numInstanceDescriptors, ID3D12GraphicsCommandList4* cmdList);
	void CreateBLAS(const SubmissionItem& renderCommand, _D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers* sourceBufferForUpdate = nullptr);
	void UpdateShaderTable();

	void updateDescriptorHeap(ID3D12GraphicsCommandList4* cmdList);
	void createInitialShaderResources(bool remake = false);

	// Root signature creation
	// TODO: create them dynamically after parsing the shader source (like ShaderPipeline does)
	bool CreateDXRGlobalRootSignature();
	bool CreateRayGenLocalRootSignature();
	bool CreateHitGroupLocalRootSignature();
	bool CreateMissLocalRootSignature();
	bool CreateEmptyLocalRootSignature();
	bool CreateRaytracingPSO();
	bool CreateDescriptorHeap();

	D3D12_GPU_DESCRIPTOR_HANDLE GetCurrentDescriptorHandle();

private:
	D3D12API* m_d3d12;

	AccelerationStructureBuffers m_TLAS_buffers[NUM_GPU_BUFFERS];
	std::unordered_map<BLAS_ID, BottomLayerData> m_BLAS_buffers[NUM_GPU_BUFFERS];

	D3D12Utils::RootSignature m_globalRootSignature;
	//TODO: generate local signatures
	D3D12Utils::RootSignature m_localRootSignature_rayGen;
	D3D12Utils::RootSignature m_localRootSignature_miss;
	D3D12Utils::RootSignature m_localRootSignature_hitGroups;

	ID3D12StateObject* m_rtxPipelineState;

	//Descriptor Heap
	ID3D12DescriptorHeap* m_descriptorHeap;
	UINT m_descriptorSize;

	//ShaderTables
	DXRUtils::ShaderTableData m_shaderTable_gen[NUM_GPU_BUFFERS];
	DXRUtils::ShaderTableData m_shaderTable_miss[NUM_GPU_BUFFERS];
	DXRUtils::ShaderTableData m_shaderTable_hitgroup[NUM_GPU_BUFFERS];

	//Shader Names
	const WCHAR* m_shader_rayGenName = L"rayGen";
	const WCHAR* m_shader_closestHitName = L"closestHitTriangle";
	const WCHAR* m_shader_missName = L"miss";
	const WCHAR* m_shader_shadowMissName = L"shadowMiss";

	//Shader Group Names
	const WCHAR* m_group_group1 = L"hitGroupTriangle";
};