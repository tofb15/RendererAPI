#pragma once

#include "D3D12Utils.h"
#include "..\..\..\Math.hpp"
#include "..\..\D3D12_FDecl.h"
#include "..\..\D3D12API.hpp"
#include "..\..\Renderers\D3D12Renderer.h"

#include <vector>
#include <unordered_map>

typedef void* _D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS;
typedef unsigned int BLAS_ID;

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

	void UpdateAccelerationStructures(ID3D12GraphicsCommandList4* cmdList);
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

	struct BottomLayerData {
		AccelerationStructureBuffers as;
		std::vector<SubmissionItem> items;
	};

	struct RayPayload {
		Float4 color;
		UINT recursionDepth;
	};

private:
	bool InitializeRootSignatures();

	// Acceleration structures
	void createTLAS(unsigned int numInstanceDescriptors, ID3D12GraphicsCommandList4* cmdList);
	void createBLAS(const SubmissionItem& renderCommand, _D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers* sourceBufferForUpdate = nullptr);

	void updateDescriptorHeap(ID3D12GraphicsCommandList4* cmdList);
	void updateShaderTables();
	void createInitialShaderResources(bool remake = false);

	// Root signature creation
	// TODO: create them dynamically after parsing the shader source (like ShaderPipeline does)
	bool CreateDXRGlobalRootSignature();
	bool CreateRayGenLocalRootSignature();
	bool CreateHitGroupLocalRootSignature();
	bool CreateMissLocalRootSignature();
	bool CreateEmptyLocalRootSignature();
	void CreateRaytracingPSO();

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


	//Shader Names
	const WCHAR* m_shader_rayGenName = L"rayGen";
	const WCHAR* m_shader_closestHitName = L"closestHitTriangle";
	const WCHAR* m_shader_missName = L"miss";
	const WCHAR* m_shader_shadowMissName = L"shadowMiss";

	//Shader Group Names
	const WCHAR* m_group_group1 = L"hitGroupTriangle";
};