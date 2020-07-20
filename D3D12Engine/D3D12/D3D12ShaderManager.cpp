#include "stdafx.h"

#include "D3D12ShaderManager.hpp"
#include <d3dcompiler.h>
#include <fstream>
#include "D3D12API.hpp"
#include <cassert>
#include "DXR/DXRUtils.h"
#include "..\Shaders\D3D12\DXR\Common_hlsl_cpp.hlsli"
#include <algorithm>

#pragma comment(lib, "d3dcompiler.lib")

D3D12ShaderManager::D3D12ShaderManager(D3D12API* api) {
	m_d3d12 = api;
}

bool D3D12ShaderManager::Initialize() {
	auto manual_dxr_shader_register = [this](const std::wstring& shaderfile, const std::wstring& entrypoint) ->ShaderHandle {
		ShaderDescription sd;
		sd.type = ShaderType::UNKNOWN;
		sd.filename = shaderfile.c_str();

		ShaderHandle handle = m_nextShaderHandle++;
		sd.entrypoint = entrypoint.c_str();
		m_dxr_shader_descriptions[handle] = sd;
		m_dxr_shader_identifiers[handle] = GenerateUniqueIdentifier();
		m_dxr_shader_libraries[shaderfile].push_back(handle);

		return handle;
	};

	//==Register dxr non-hitgroup shaders==
	m_rayGenShader = manual_dxr_shader_register(m_non_hit_group_shader_file, m_shader_rayGenName);
	m_MissShader = manual_dxr_shader_register(m_non_hit_group_shader_file, m_shader_missName);
	m_ShadowMissShader = manual_dxr_shader_register(m_non_hit_group_shader_file, m_shader_shadowMissName);

	if (!InitializeDXRRootSignatures()) {
		return false;
	}

	return true;
}

D3D12ShaderManager::~D3D12ShaderManager() {
	for (auto& blob : m_raster_shader_blobs) {
		blob.second->Release();
	}
	if (m_rtxPipelineState) {
		m_rtxPipelineState->Release();
		m_rtxPipelineState = nullptr;
	}
	for (auto& e : m_raster_shader_blobs) {
		if (e.second) {
			e.second->Release();
		}
	}
}

ID3DBlob* D3D12ShaderManager::CompileShader(const ShaderDescription& sd) {
	//std::vector<ID3DBlob*>& shaderVector = m_raster_shader_blobs[sd.type];

	//if (sd.type == ShaderType::VS) {
	//	m_vertexDefines.push_back(sd.defines);
	//}

	//ID3DBlob* blob;
	//ID3DBlob* blob_err;
	//
	//HRESULT hr;
	//
	//std::string shaderPath = "../assets/D3D12/" + std::string(sd.filename) + std::string(".hlsl");
	//std::ifstream shaderFile(shaderPath);
	//
	//std::string shaderText;
	//std::string completeShaderText("");
	//std::string entry = sd.entrypoint;
	//std::string model;
	//
	//// Set entry point and model based on type
	//switch (sd.type) {
	//case ShaderType::VS: model = "vs_5_1"; break;
	//case ShaderType::FS: model = "ps_5_1"; break;
	//case ShaderType::GS: model = "gs_5_0"; break;
	//	//case ShaderType::CS: entry = "main"; model = "cs_5_0"; break;
	//default: break;
	//}
	//
	//// open the file and read it to a string "shaderText"
	//if (shaderFile.is_open()) {
	//	shaderText = std::string((std::istreambuf_iterator<char>(shaderFile)), std::istreambuf_iterator<char>());
	//	shaderFile.close();
	//} else {
	//	return nullptr;
	//}
	//
	//// Create the complete shader text (defines + file data)
	//completeShaderText += sd.defines;
	//completeShaderText += shaderText;
	//
	//hr = D3DCompile(
	//	completeShaderText.c_str(),
	//	completeShaderText.size(),
	//	nullptr,
	//	nullptr,
	//	nullptr,
	//	entry.c_str(),
	//	model.c_str(),
	//	D3DCOMPILE_ENABLE_UNBOUNDED_DESCRIPTOR_TABLES,
	//	0U,
	//	&blob,
	//	&blob_err
	//);
	//if (FAILED(hr)) {
	//	OutputDebugStringA((char*)blob_err->GetBufferPointer());
	//	return nullptr;
	//}
	//
	//return blob;
	return nullptr;
}

bool D3D12ShaderManager::InitializeDXRRootSignatures() {
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

bool D3D12ShaderManager::CreateDXRGlobalRootSignature() {
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

bool D3D12ShaderManager::CreateRayGenLocalRootSignature() {
	m_localRootSignature_rayGen = D3D12Utils::RootSignature(L"Root_RayGenLocal");
	m_localRootSignature_rayGen.AddDescriptorTable("uav_test", D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1);
	//m_localRootSignature_rayGen.Add32BitConstants("Constant", 1, 0, 0);
	if (!m_localRootSignature_rayGen.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
		return false;
	}

	return true;
}

bool D3D12ShaderManager::CreateHitGroupLocalRootSignature() {
	m_localRootSignature_hitGroups = D3D12Utils::RootSignature(L"Root_HitGroupLocal");
	m_localRootSignature_hitGroups.AddSRV("VertexBuffer_POS", 1, 0);
	m_localRootSignature_hitGroups.AddSRV("VertexBuffer_Norm", 1, 1);
	m_localRootSignature_hitGroups.AddSRV("VertexBuffer_UV", 1, 2);
	m_localRootSignature_hitGroups.AddSRV("VertexBuffer_TAN_BI", 1, 3);
	m_localRootSignature_hitGroups.AddDescriptorTable("AlbedoColor", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);
	m_localRootSignature_hitGroups.AddDescriptorTable("AlphaMapTextures", D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 3, 0, -1);

	if (!m_localRootSignature_hitGroups.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
		return false;
	}

	return true;
}

bool D3D12ShaderManager::CreateMissLocalRootSignature() {
	//Could be used to bind a skybox.

	m_localRootSignature_miss = D3D12Utils::RootSignature(L"Root_MissLocal");
	if (!m_localRootSignature_miss.Build(m_d3d12, D3D12_ROOT_SIGNATURE_FLAG_LOCAL_ROOT_SIGNATURE)) {
		return false;
	}

	return true;
}

bool D3D12ShaderManager::CreateEmptyLocalRootSignature() {
	return true;
}

std::string D3D12ShaderManager::GetVertexDefines(ShaderHandle index) const {
	return m_vertexDefines.at(index);
}

ID3DBlob* D3D12ShaderManager::GetShaderBlob(const ShaderHandle& shader) {
	return m_raster_shader_blobs[shader];
}

bool D3D12ShaderManager::CreateRaytracingPSO(const std::vector<ShaderDefine>* _defines) {
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
		for (auto& e : *_defines) {
			if (e.define == L"RAY_GEN_ALPHA_TEST" || e.define == L"CLOSEST_HIT_ALPHA_TEST_1" || e.define == L"CLOSEST_HIT_ALPHA_TEST_2") {
				allowAnyhit = false;
			}

			defines.push_back({ e.define.c_str(), e.value.c_str() });
		}
	}

	//===Register all shader files and inform which shader entry-points exist within each.===
	for (auto& lib : m_dxr_shader_libraries) {
		std::vector<LPCWSTR> shaderEntypoints;
		std::vector<LPCWSTR> shaderIdentifiers;

		shaderEntypoints.resize(lib.second.size());
		shaderIdentifiers.resize(lib.second.size());

		std::transform(lib.second.begin(), lib.second.end(), shaderEntypoints.begin(), [this](ShaderHandle sh)->LPCWSTR {
			return m_dxr_shader_descriptions[sh].entrypoint.c_str();
			});
		std::transform(lib.second.begin(), lib.second.end(), shaderIdentifiers.begin(), [this](ShaderHandle sh)->LPCWSTR { return m_dxr_shader_identifiers[sh].c_str(); });

		psoBuilder.AddLibrary(lib.first, shaderEntypoints, defines, &shaderIdentifiers);
		//psoBuilder.AddLibrary(lib.first, shaderEntypoints, defines);
	}

	//===Register all shader hitgroups and inform which shaders should be used for it.===
	for (auto& e : m_dxr_hitgroups_identifiers) {
		ShaderProgramDescription& spd = m_dxr_hitgroups_descriptions[e.first];
		LPCWSTR closestHit = (spd.CHS > 0) ? m_dxr_shader_identifiers[spd.CHS].c_str() : nullptr;
		LPCWSTR anyHit = (spd.AHS > 0) ? m_dxr_shader_identifiers[spd.AHS].c_str() : nullptr;
		LPCWSTR instersection = (spd.IS > 0) ? m_dxr_shader_identifiers[spd.IS].c_str() : nullptr;
		psoBuilder.AddHitGroup(e.second.c_str(), closestHit, anyHit, instersection);
	}

	//===Bind Local rootsignatures non-hitgroup shaders (raygen, miss, etc.)===
	psoBuilder.AddSignatureToShaders({ m_dxr_shader_identifiers[m_rayGenShader].c_str() }, m_localRootSignature_rayGen.Get());
	psoBuilder.AddSignatureToShaders({ m_dxr_shader_identifiers[m_MissShader].c_str() }, m_localRootSignature_miss.Get());
	psoBuilder.AddSignatureToShaders({ m_dxr_shader_identifiers[m_ShadowMissShader].c_str() }, m_localRootSignature_empty.Get());

	//psoBuilder.AddSignatureToShaders({ m_group_alphaTest_shadow }, m_localRootSignature_hitGroups.Get());
	//===Bind Local rootsignatures to all hit groups===
	for (auto& e : m_dxr_hitgroups_identifiers) {
		psoBuilder.AddSignatureToShaders({ e.second.c_str() }, m_localRootSignature_hitGroups.Get());
	}

	psoBuilder.SetMaxPayloadSize(payloadSize);
	psoBuilder.SetMaxAttributeSize(sizeof(float) * 4);
	psoBuilder.SetMaxRecursionDepth(DXRShaderCommon::MAX_RAY_RECURSION_DEPTH);
	psoBuilder.SetGlobalSignature(m_globalRootSignature.Get());

	m_rtxPipelineState = psoBuilder.Build(m_d3d12->GetDevice());
	if (!m_rtxPipelineState) {
		return false;
	}

	m_psoNeedsRebuild = false;
	return true;
}

bool D3D12ShaderManager::NeedsPSORebuild() {
	return m_psoNeedsRebuild;
}

ShaderHandle D3D12ShaderManager::RegisterShader(const ShaderDescription& sd) {
	assert((sd.type == ShaderType::CLOSEST_HIT || sd.type == ShaderType::ANY_HIT || sd.type == ShaderType::INTERSECTION) && "Only DXR shaders are supported at the moment.");
	ShaderHandle handle = m_nextShaderHandle++;

	//TODO:: Check if this shader is allready Registerd
	m_dxr_shader_descriptions[handle] = sd;
	m_dxr_shader_identifiers[handle] = GenerateUniqueIdentifier();
	m_dxr_shader_libraries[sd.filename].push_back(handle);

	m_psoNeedsRebuild = true;
	return handle;
}

ShaderProgramHandle D3D12ShaderManager::RegisterShaderProgram(const ShaderProgramDescription& shaderProgramDescription) {
	ShaderProgramHandle handle = m_nextShaderProgramHandle++;

	//TODO: check if this shader program is allready Registerd.
	//TODO: check if the description is valid.

	m_dxr_hitgroups_descriptions[handle] = shaderProgramDescription;
	m_dxr_hitgroups_identifiers[handle] = GenerateUniqueIdentifier();

	if (shaderProgramDescription.AHS >= 0) {
		ShaderProgramDescription spd_shadow;
		ShaderProgramHandle shadowHandle = m_nextShaderProgramHandle++;

		spd_shadow.AHS = shaderProgramDescription.AHS;
		m_dxr_hitgroups_descriptions[shadowHandle] = spd_shadow;
		m_dxr_hitgroups_identifiers[shadowHandle] = m_dxr_hitgroups_identifiers[handle] + L"_shadow";
	}

	m_psoNeedsRebuild = true;
	return handle;
}

const LPCWSTR D3D12ShaderManager::GetRaygenShaderIdentifier() {
	return m_dxr_shader_identifiers[m_rayGenShader].c_str();
}

const LPCWSTR D3D12ShaderManager::GetMissShaderIdentifier() {
	return m_dxr_shader_identifiers[m_MissShader].c_str();
}

const LPCWSTR D3D12ShaderManager::GetShadowMissShaderIdentifier() {
	return m_dxr_shader_identifiers[m_ShadowMissShader].c_str();
}

const LPCWSTR D3D12ShaderManager::GetHitGroupIdentifier(ShaderProgramHandle sph) {
	return m_dxr_hitgroups_identifiers[sph].c_str();
}

bool D3D12ShaderManager::IsHitGroupOpaque(ShaderProgramHandle sph) {
	return m_dxr_hitgroups_descriptions[sph].AHS < 0;
}

std::wstring D3D12ShaderManager::GenerateUniqueIdentifier() {
	return L"id" + std::to_wstring(m_identifierCounter++);
}
