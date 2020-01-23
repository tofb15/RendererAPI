#include "DXRBase.h"
#include <d3d12.h>

#include "DXRUtils.h"

DXRBase::DXRBase(D3D12API* d3d12) : m_d3d12(d3d12)
{}

DXRBase::~DXRBase()
{
}

bool DXRBase::Initialize()
{
	if (!InitializeRootSignatures()) {
		return false;
	}

	return true;
}

void DXRBase::UpdateAccelerationStructures(ID3D12GraphicsCommandList4* cmdList)
{
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

void DXRBase::createTLAS(unsigned int numInstanceDescriptors, ID3D12GraphicsCommandList4* cmdList)
{
}

void DXRBase::createBLAS(const SubmissionItem& renderCommand, _D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAGS flags, ID3D12GraphicsCommandList4* cmdList, AccelerationStructureBuffers* sourceBufferForUpdate)
{
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
	psoBuilder.AddLibrary(DEFAULT_SHADER_LOCATION + "dxr/" + DEFAULT_SHADER_NAME + ".hlsl", { m_shader_rayGenName, m_shader_missName, m_shader_shadowMissName, m_shader_closestHitName}, defines);
	psoBuilder.AddHitGroup(m_group_group1, m_shader_closestHitName);

	psoBuilder.AddSignatureToShaders({ m_shader_rayGenName }, m_localRootSignature_rayGen.Get());
	psoBuilder.AddSignatureToShaders({ m_group_group1}, m_localRootSignature_hitGroups.Get());
	psoBuilder.AddSignatureToShaders({ m_shader_missName}, m_localRootSignature_miss.Get());

	psoBuilder.SetMaxPayloadSize(payloadSize);
	psoBuilder.SetMaxAttributeSize(sizeof(float) * 4);
	psoBuilder.SetMaxRecursionDepth(MAX_RAY_RECURSION_DEPTH);
	psoBuilder.SetGlobalSignature(m_globalRootSignature.Get());
}

bool DXRBase::CreateDXRGlobalRootSignature()
{
	m_globalRootSignature = D3D12Utils::RootSignature(L"dxrGlobalRoot");
	m_globalRootSignature.AddDescriptorTable("OutputAlbedoUAV", D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 0);
	m_globalRootSignature.AddSRV("AccelerationStructure", 0);
	m_globalRootSignature.AddCBV("SceneCBuffer", 0);
	m_globalRootSignature.AddStaticSampler();

	if (!m_globalRootSignature.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_NONE)) {
		return false;
	}

	return true;
}

bool DXRBase::CreateRayGenLocalRootSignature()
{
	m_localRootSignature_rayGen = D3D12Utils::RootSignature(L"Root_RayGenLocal");
	if (!m_localRootSignature_hitGroups.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
		return false;
	}

	return true;
}

bool DXRBase::CreateHitGroupLocalRootSignature()
{
	m_localRootSignature_hitGroups = D3D12Utils::RootSignature(L"Root_HitGroupLocal");
	m_localRootSignature_hitGroups.AddSRV("VertexBuffer", 1, 0);
	m_localRootSignature_hitGroups.AddSRV("IndexBuffer", 1, 1);
	m_localRootSignature_hitGroups.AddCBV("MeshCBuffer", 1, 0);
	m_localRootSignature_hitGroups.AddDescriptorTable("Textures", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0, 3); // Textures (t0, t1, t2)
	m_localRootSignature_hitGroups.AddStaticSampler();
	if (!m_localRootSignature_hitGroups.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
		return false;
	}

	return true;
}

bool DXRBase::CreateMissLocalRootSignature()
{
	//Could be used to bind a skybox.

	m_localRootSignature_miss = D3D12Utils::RootSignature(L"Root_MissLocal");
	if (!m_localRootSignature_hitGroups.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
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
