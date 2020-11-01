#include "stdafx.h"

#include "DXRUtils.h"
#include <comdef.h>
#include <cassert>

DXRUtils::PSOBuilder::PSOBuilder() {
	//TODO: Fix this. Program crashes when the vectors expand due to references becoming invalid.
	m_associationNames.reserve(20);
	m_exportAssociations.reserve(20);
	m_shaderNames.reserve(20);
	m_exportDescs.reserve(20);
	m_libraryDescs.reserve(20);
	m_hitGroupDescs.reserve(20);
}

DXRUtils::PSOBuilder::~PSOBuilder() {
}

bool DXRUtils::PSOBuilder::Initialize() {
	if (FAILED(m_dxilCompiler.init())) {
		return false;
	}

	return true;
}

D3D12_STATE_SUBOBJECT* DXRUtils::PSOBuilder::Append(const D3D12_STATE_SUBOBJECT_TYPE type, const void* desc) {
	D3D12_STATE_SUBOBJECT* so = m_start + m_numSubobjects++;
	so->Type = type;
	so->pDesc = desc;
	return so;
}

//bool DXRUtils::PSOBuilder::AddLibrary(const std::string& shaderPath, const std::vector<LPCWSTR>& names, const std::vector<DxcDefine>& defines, std::wstring* errorMessage) {
//	//std::wstring stemp = std::wstring(shaderPath.begin(), shaderPath.end());
//	//AddLibrary(std::wstring stemp = std::wstring(shaderPath.begin(), shaderPath.end()));
//}

bool DXRUtils::PSOBuilder::AddLibrary(const std::wstring& shaderPath, const std::vector<LPCWSTR>& names, const std::vector<DxcDefine>& defines, const std::vector<LPCWSTR>* exportNames, std::wstring* errorMessage) {
	// Add names to the list of names/export to be configured in generate()
	if (exportNames) {
		m_shaderNames.insert(m_shaderNames.end(), exportNames->begin(), exportNames->end());
	} else {
		m_shaderNames.insert(m_shaderNames.end(), names.begin(), names.end());
	}

	DXILShaderCompiler::Desc shaderDesc;
	shaderDesc.compileArguments.push_back(L"/Gis"); // Declare all variables and values as precise
#ifdef _DEBUG
	shaderDesc.compileArguments.push_back(L"/Zi"); // Debug info
#endif
	//std::wstring stemp = std::wstring(shaderPath.begin(), shaderPath.end());
	shaderDesc.filePath = shaderPath.c_str();
	shaderDesc.entryPoint = L"";
	shaderDesc.targetProfile = L"lib_6_3";
	shaderDesc.defines = defines;

	IDxcBlob* pShaders = nullptr;
	if (FAILED(m_dxilCompiler.compile(&shaderDesc, &pShaders, errorMessage))) {
		MessageBoxA(NULL, "Could not compile shader", "Shader Compiler failed.", 0);
		return false;
	}

	m_exportDescs.emplace_back(names.size());
	std::vector<D3D12_EXPORT_DESC>& dxilExports = m_exportDescs.back();
	for (int i = 0; i < names.size(); i++) {
		auto& desc = dxilExports[i];

		if (exportNames) {
			desc.Name = (*exportNames)[i];
			desc.ExportToRename = names[i];
		} else {
			desc.Name = names[i];
			desc.ExportToRename = nullptr;
		}

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

void DXRUtils::PSOBuilder::AddHitGroup(LPCWSTR exportName, LPCWSTR closestHitShaderImport, LPCWSTR anyHitShaderImport, LPCWSTR intersectionShaderImport, D3D12_HIT_GROUP_TYPE type) {
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

void DXRUtils::PSOBuilder::AddSignatureToShaders(const std::vector<LPCWSTR>& shaderNames, ID3D12RootSignature** rootSignature) {
	D3D12_STATE_SUBOBJECT* signatureSO = Append(D3D12_STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE, rootSignature);

	m_associationNames.emplace_back(shaderNames);

	// Bind local root signature shaders
	m_exportAssociations.emplace_back();
	D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION& localRootAssociation = m_exportAssociations.back();
	localRootAssociation.pExports = &m_associationNames.back()[0];
	localRootAssociation.NumExports = UINT(shaderNames.size());
	localRootAssociation.pSubobjectToAssociate = signatureSO; //<-- address to local root subobject

	Append(D3D12_STATE_SUBOBJECT_TYPE_SUBOBJECT_TO_EXPORTS_ASSOCIATION, &localRootAssociation);

}

void DXRUtils::PSOBuilder::SetGlobalSignature(ID3D12RootSignature** rootSignature) {
	m_globalRootSignature = rootSignature;
}

void DXRUtils::PSOBuilder::SetMaxPayloadSize(UINT size) {
	m_maxPayloadSize = size;
}

void DXRUtils::PSOBuilder::SetMaxAttributeSize(UINT size) {
	m_maxAttributeSize = size;
}

void DXRUtils::PSOBuilder::SetMaxRecursionDepth(UINT depth) {
	m_maxRecursionDepth = depth;
}

ID3D12StateObject* DXRUtils::PSOBuilder::Build(ID3D12Device5* device) {
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

	// Append the global root signature
	if (m_globalRootSignature)
		Append(D3D12_STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE, m_globalRootSignature);

	// Create the state
	D3D12_STATE_OBJECT_DESC desc;
	desc.NumSubobjects = m_numSubobjects;
	desc.pSubobjects = m_start;
	desc.Type = D3D12_STATE_OBJECT_TYPE_RAYTRACING_PIPELINE;

	ID3D12StateObject* pso;
	HRESULT hr = device->CreateStateObject(&desc, IID_PPV_ARGS(&pso));
	if (FAILED(hr)) {
		_com_error err2(hr);
		std::cout << "Device Status: " << err2.ErrorMessage() << std::endl;

		hr = device->GetDeviceRemovedReason();
		_com_error err(hr);
		OutputDebugString(err.ErrorMessage());
		OutputDebugString("\n");
		std::cout << err.ErrorMessage() << std::endl;
		return nullptr;
	}

	if (FAILED(hr)) {
		MessageBoxA(NULL, "DXRUtils::PSOBuilder::Build: Could not build PSO", "Shader Compiler failed.", 0);
		return nullptr;
	}

	return pso;
}

DXRUtils::ShaderTableBuilder::ShaderTableBuilder(UINT nInstances, ID3D12StateObject* pso, UINT maxInstanceSize) : m_nInstances(nInstances), m_maxInstanceSize(maxInstanceSize) {
	HRESULT hr = pso->QueryInterface(IID_PPV_ARGS(&m_psoProp));

	if (FAILED(hr)) {
		MessageBoxA(NULL, "DXRUtils::ShaderTableBuilder::ShaderTableBuilder: Could not get PSO Properties.", "DXR Error", 0);
		exit(0);
	}

	m_data = MY_NEW char* [nInstances];
	m_dataOffset = MY_NEW UINT[nInstances];
	for (UINT i = 0; i < nInstances; i++) {
		m_data[i] = MY_NEW char[maxInstanceSize];
		memset(m_data[i], 0, maxInstanceSize);
		m_dataOffset[i] = 0;
	}
}

DXRUtils::ShaderTableBuilder::~ShaderTableBuilder() {
	if (m_data) {
		for (size_t i = 0; i < m_nInstances; i++) {
			if (m_data[i]) {
				delete[] m_data[i];
			}
		}
		delete[] m_data;
	}

	if (m_dataOffset) {
		delete[] m_dataOffset;
	}

	if (m_psoProp) {
		m_psoProp->Release();
	}
}

void DXRUtils::ShaderTableBuilder::AddShader(const LPCWSTR& shaderName) {
	m_shaderNames.push_back(shaderName);
}

void DXRUtils::ShaderTableBuilder::AddDescriptor(const UINT64& descriptor, UINT instance) {
	assert(instance < m_nInstances && "DXRUtils::ShaderTableBuilder::AddDescriptor");
	assert(m_dataOffset[instance] + sizeof(descriptor) <= m_maxInstanceSize && "DXRUtils::ShaderTableBuilder::AddDescriptor bytesPerInstance is too small!");

	char* pData = m_data[instance] + m_dataOffset[instance];
	*(UINT64*)pData = descriptor;
	m_dataOffset[instance] += sizeof(descriptor);
}

void DXRUtils::ShaderTableBuilder::AddConstants(UINT numConstants, const float* constants, UINT instance) {
	UINT size = sizeof(float) * numConstants;
	assert(instance < m_nInstances && "DXRUtils::ShaderTableBuilder::AddConstants");
	assert(m_dataOffset[instance] + size <= m_maxInstanceSize && "DXRUtils::ShaderTableBuilder::AddConstants, bytesPerInstance is too small!");

	char* pData = m_data[instance] + m_dataOffset[instance];
	memcpy(pData, constants, size);
	m_dataOffset[instance] += size;
}

DXRUtils::ShaderTableData DXRUtils::ShaderTableBuilder::Build(ID3D12Device5* device) {
	assert(m_shaderNames.size() == m_nInstances && "DXRUtils::ShaderTableBuilder::Build, All shaders have not been givven a name");
	DXRUtils::ShaderTableData tableData;
	//===Find the largest instance===
	UINT largestInstanceSize = 0;
	for (UINT i = 0; i < m_nInstances; i++) {
		if (m_dataOffset[i] > largestInstanceSize) {
			largestInstanceSize = m_dataOffset[i];
		}
	}
	largestInstanceSize += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
	UINT alignTo = D3D12_RAYTRACING_SHADER_TABLE_BYTE_ALIGNMENT;
	UINT padding = (alignTo - (largestInstanceSize % alignTo)) % alignTo;
	UINT alignedSize = largestInstanceSize + padding;

	//===Set up shadertable===
	tableData.strideInBytes = alignedSize;
	tableData.sizeInBytes = tableData.strideInBytes * m_nInstances;
	tableData.resource = D3D12Utils::CreateBuffer(device, tableData.sizeInBytes, D3D12_RESOURCE_FLAG_NONE, D3D12_RESOURCE_STATE_GENERIC_READ, D3D12Utils::sUploadHeapProperties);
	tableData.resource->SetName(L"SHADER_TABLE");

	//===Copy data to GPU resource===
	char* pData;
	tableData.resource->Map(0, nullptr, (void**)& pData);
	for (UINT i = 0; i < m_nInstances; i++) {
		auto& shader = m_shaderNames[i];

		// Copy shader identifier
		void* shaderID = nullptr;
		//if (shader == 0) {
		//	shader = L"NULL";
		//}
		if (shader == 0 || std::wcscmp(shader, L"NULL") == 0) {
			// NULL shader identifier is valid and will cause no shader to be executed
			pData += alignedSize; // No data, just append padding
		} else {
			shaderID = m_psoProp->GetShaderIdentifier(shader);
			assert(shaderID != nullptr && "Shader Identifier not found in stateObject");
			memcpy(pData, shaderID, D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES);
			pData += D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES;
			// Copy other data (descriptors, constants)
			memcpy(pData, m_data[i], m_dataOffset[i]);
			pData += alignedSize - D3D12_SHADER_IDENTIFIER_SIZE_IN_BYTES; // Append padding
		}
	}
	tableData.resource->Unmap(0, nullptr);

	return tableData;
}
