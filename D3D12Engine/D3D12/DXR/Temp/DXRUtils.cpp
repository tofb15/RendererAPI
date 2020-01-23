#include "DXRUtils.h"

DXRUtils::PSOBuilder::PSOBuilder()
{
}

DXRUtils::PSOBuilder::~PSOBuilder()
{
}

bool DXRUtils::PSOBuilder::Initialize()
{
	return false;
}

D3D12_STATE_SUBOBJECT* DXRUtils::PSOBuilder::Append(const STATE_SUBOBJECT_TYPE type, const void* desc)
{
	return nullptr;
}

void DXRUtils::PSOBuilder::AddLibrary(const std::string& shaderPath, const std::vector<LPCWSTR>& names, const std::vector<DxcDefine>& defines)
{
}

void DXRUtils::PSOBuilder::AddHitGroup(LPCWSTR exportName, LPCWSTR closestHitShaderImport, LPCWSTR anyHitShaderImport, LPCWSTR intersectionShaderImport, GROUP_TYPE type)
{
}

void DXRUtils::PSOBuilder::AddSignatureToShaders(const std::vector<LPCWSTR>& shaderNames, ID3D12RootSignature** rootSignature)
{
}

void DXRUtils::PSOBuilder::SetGlobalSignature(ID3D12RootSignature** rootSignature)
{
}

void DXRUtils::PSOBuilder::SetMaxPayloadSize(UINT size)
{
}

void DXRUtils::PSOBuilder::SetMaxAttributeSize(UINT size)
{
}

void DXRUtils::PSOBuilder::SetMaxRecursionDepth(UINT depth)
{
}
