#include "../D3D12Engine/D3D12/DXR/Temp/DXRUtils.h"
//#include "../D3D12Engine/D3D12/DXR/Temp/DXILShaderCompiler.h"

	//==Shader Names==
const WCHAR* m_shader_rayGenName = L"rayGen";
const WCHAR* m_shader_closestHitName = L"closestHitTriangle";
const WCHAR* m_shader_missName = L"miss";
const WCHAR* m_shader_shadowMissName = L"shadowMiss";

//==Shader Group Names==
const WCHAR* m_group_group1 = L"hitGroupTriangle";

ID3D12StateObject* m_rtxPipelineState = nullptr;


int main() {
	std::vector<DxcDefine> defines;

	defines.push_back({ L"ddx(x)" , L"x" });
	defines.push_back({ L"ddy(x)" , L"x" });
	defines.push_back({ L"discard" , L"" });
	defines.push_back({ L"Sample(sampler, uv, ...)" , L"SampleLevel(sampler, uv, 0)" });
	defines.push_back({ L"SampleGrad(sampler, uv, ...)" , L"SampleLevel(sampler, uv, 0)" });
	defines.push_back({ L"clip(x)" , L"" });

	DXRUtils::PSOBuilder pso;
	pso.Initialize();
	pso.AddLibrary("../../Exported_Assets/Shader/23BA625C-DA77-02A6-70C8-48D0368F0809.hlsl", { L"tes123t" }, defines);
	pso.AddLibrary("../Shaders/D3D12/DXR/test.hlsl", { m_shader_rayGenName, m_shader_closestHitName, m_shader_missName }, defines);

	//pso.AddHitGroup(m_group_group1, m_shader_closestHitName);
	//pso.AddSignatureToShaders({ m_shader_rayGenName }, m_localRootSignature_rayGen.Get());
	//pso.AddSignatureToShaders({ m_group_group1 }, m_localRootSignature_hitGroups.Get());
	//pso.AddSignatureToShaders({ m_shader_missName }, m_localRootSignature_miss.Get());
	//pso.SetMaxPayloadSize(payloadSize);
	//pso.SetMaxAttributeSize(sizeof(float) * 4);
	//pso.SetMaxRecursionDepth(MAX_RAY_RECURSION_DEPTH);
	//pso.SetGlobalSignature(m_globalRootSignature.Get());

	//m_rtxPipelineState = psoBuilder.Build(m_d3d12->GetDevice());
	//if (!m_rtxPipelineState) {
	//	return false;
	//}

	return 0;
}