#include "DXRUtils.h"
#include <comdef.h>

DXRUtils::PSOBuilder::PSOBuilder()
{

	//m_associationNames.reserve(20);
	//m_exportAssociations.reserve(20);
	//m_shaderNames.reserve(20);
	//m_exportDescs.reserve(20);
	//m_libraryDescs.reserve(20);
	//m_hitGroupDescs.reserve(20);
}

DXRUtils::PSOBuilder::~PSOBuilder()
{
}

bool DXRUtils::PSOBuilder::Initialize()
{
	if (FAILED(m_dxilCompiler.init())) {
		return false;
	}

	return true;
}

D3D12_STATE_SUBOBJECT* DXRUtils::PSOBuilder::Append(const D3D12_STATE_SUBOBJECT_TYPE type, const void* desc)
{
	D3D12_STATE_SUBOBJECT* so = m_start + m_numSubobjects++;
	so->Type = type;
	so->pDesc = desc;
	return so;
}

void DXRUtils::PSOBuilder::AddLibrary(const std::string& shaderPath, const std::vector<LPCWSTR>& names, const std::vector<DxcDefine>& defines)
{
	// Add names to the list of names/export to be configured in generate()
	m_shaderNames.insert(m_shaderNames.end(), names.begin(), names.end());

	DXILShaderCompiler::Desc shaderDesc;
	shaderDesc.compileArguments.push_back(L"/Gis"); // Declare all variables and values as precise
#ifdef _DEBUG
	shaderDesc.compileArguments.push_back(L"/Zi"); // Debug info
#endif
	std::wstring stemp = std::wstring(shaderPath.begin(), shaderPath.end());
	shaderDesc.filePath = stemp.c_str();
	shaderDesc.entryPoint = L"";
	shaderDesc.targetProfile = L"lib_6_3";
	shaderDesc.defines = defines;

	IDxcBlob* pShaders = nullptr;
	if (FAILED(m_dxilCompiler.compile(&shaderDesc, &pShaders))) {
		MessageBoxA(NULL, "Could not compile shader", "Shader Compiler failed.", 0);
	}

	m_exportDescs.emplace_back(names.size());
	std::vector<D3D12_EXPORT_DESC>& dxilExports = m_exportDescs.back();
	for (int i = 0; i < names.size(); i++) {
		auto& desc = dxilExports[i];
		desc.Name = names[i];
		desc.ExportToRename = nullptr;
		desc.Flags = D3D12_EXPORT_FLAG_NONE;
	}

	// Set up DXIL description
	m_libraryDescs.emplace_back();
	D3D12_DXIL_LIBRARY_DESC& dxilLibraryDesc = m_libraryDescs.back();;
	dxilLibraryDesc.DXILLibrary.pShaderBytecode = pShaders->GetBufferPointer();
	dxilLibraryDesc.DXILLibrary.BytecodeLength = pShaders->GetBufferSize();
	dxilLibraryDesc.pExports = &dxilExports[0];
	dxilLibraryDesc.NumExports = UINT(dxilExports.size());

	Append(D3D12_STATE_SUBOBJECT_TYPE_DXIL_LIBRARY, &dxilLibraryDesc);
}

void DXRUtils::PSOBuilder::AddHitGroup(LPCWSTR exportName, LPCWSTR closestHitShaderImport, LPCWSTR anyHitShaderImport, LPCWSTR intersectionShaderImport, D3D12_HIT_GROUP_TYPE type)
{
	//Init hit group
	m_hitGroupDescs.emplace_back();
	D3D12_HIT_GROUP_DESC& hitGroupDesc = m_hitGroupDescs.back();
	hitGroupDesc.AnyHitShaderImport = anyHitShaderImport;
	hitGroupDesc.ClosestHitShaderImport = closestHitShaderImport;
	hitGroupDesc.HitGroupExport = exportName;
	hitGroupDesc.IntersectionShaderImport = intersectionShaderImport;
	hitGroupDesc.Type = type;

	Append(D3D12_STATE_SUBOBJECT_TYPE_HIT_GROUP, &hitGroupDesc);
}

void DXRUtils::PSOBuilder::AddSignatureToShaders(const std::vector<LPCWSTR>& shaderNames, ID3D12RootSignature** rootSignature)
{
	//TODO: pass in different rootSignatures to different shaders
	D3D12_STATE_SUBOBJECT* signatureSO = Append(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, rootSignature);

	m_associationNames.emplace_back(shaderNames);

	// Bind local root signature shaders
	m_exportAssociations.emplace_back();
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& rayGenLocalRootAssociation = m_exportAssociations.back();
	rayGenLocalRootAssociation.pExports = &m_associationNames.back()[0];
	rayGenLocalRootAssociation.NumExports = UINT(shaderNames.size());
	rayGenLocalRootAssociation.pSubobjectToAssociate = signatureSO; //<-- address to local root subobject

	Append(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &rayGenLocalRootAssociation);

}

void DXRUtils::PSOBuilder::SetGlobalSignature(ID3D12RootSignature** rootSignature)
{
	m_globalRootSignature = rootSignature;
}

void DXRUtils::PSOBuilder::SetMaxPayloadSize(UINT size)
{
	m_maxPayloadSize = size;
}

void DXRUtils::PSOBuilder::SetMaxAttributeSize(UINT size)
{
	m_maxAttributeSize = size;
}

void DXRUtils::PSOBuilder::SetMaxRecursionDepth(UINT depth)
{
	m_maxRecursionDepth = depth;
}

ID3D12StateObject* DXRUtils::PSOBuilder::Build(ID3D12Device5* device)
{
	// Init shader config
	D3D12_RAYTRACING_SHADER_CONFIG shaderConfig = {};
	shaderConfig.MaxAttributeSizeInBytes = m_maxAttributeSize;
	shaderConfig.MaxPayloadSizeInBytes = m_maxPayloadSize;
	auto configSO = Append(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_SHADER_CONFIG, &shaderConfig);

	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION shaderConfigAssociation;
	if (!m_shaderNames.empty()) {
		// Bind the payload size to the programs
		shaderConfigAssociation.pExports = &m_shaderNames[0];
		shaderConfigAssociation.NumExports = UINT(m_shaderNames.size());
		shaderConfigAssociation.pSubobjectToAssociate = configSO; //<-- address to shader config subobject
		Append(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &shaderConfigAssociation);
	}
	// Init pipeline config
	D3D12_RAYTRACING_PIPELINE_CONFIG pipelineConfig;
	pipelineConfig.MaxTraceRecursionDepth = m_maxRecursionDepth;
	Append(D3D12_STATE_SUBOBJECT_TYPE_RAYTRACING_PIPELINE_CONFIG, &pipelineConfig);

	// Append the global root signature (I am GROOT)
	if (m_globalRootSignature)
		Append(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, m_globalRootSignature);

	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = m_numSubobjects;
	desc.pSubobjects = m_start;
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ID3D12StateObject* pso;
	HRESULT hr = device->CreateStateObject(&desc, IID_PPV_ARGS(&pso));
	if (!SUCCEEDED(hr)) {
		_com_error err2(hr);
		std::cout << "Device Status: " << err2.ErrorMessage() << std::endl;

		hr = device->GetDeviceRemovedReason();
		_com_error err(hr);
		OutputDebugString(err.ErrorMessage());
		OutputDebugString("\n");
		std::cout << err.ErrorMessage() << std::endl;
	}

	if (FAILED(hr)) {
		MessageBoxA(NULL, "Could not build PSO", "Shader Compiler failed.", 0);
		return nullptr;
	}

	return pso;
}
