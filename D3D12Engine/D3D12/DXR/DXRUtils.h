#pragma once
#include "../D3D12API.hpp"
#include "DXILShaderCompiler.h"
#include "D3D12Utils.h"

#include <Windows.h>

namespace DXRUtils {
	//typedef enum class GROUP_TYPE
	//{
	//	TRIANGLES = 0,
	//	PROCEDURAL_PRIMITIVE = 0x1
	//} 	GROUP_TYPE;

	//typedef enum class STATE_SUBOBJECT_TYPE
	//{
	//	STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE = 1,
	//	STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE = 2,
	//} 	STATE_SUBOBJECT_TYPE;

	class PSOBuilder {
	public:
		PSOBuilder();
		~PSOBuilder();

		bool Initialize();

		D3D12_STATE_SUBOBJECT* Append(const D3D12_STATE_SUBOBJECT_TYPE type, const void* desc);
		bool AddLibrary(const std::string& shaderPath, const std::vector<LPCWSTR>& names, const std::vector<DxcDefine>& defines = std::vector<DxcDefine>(), std::wstring* errorMessage = nullptr);
		void AddHitGroup(LPCWSTR exportName, LPCWSTR closestHitShaderImport, LPCWSTR anyHitShaderImport = nullptr, LPCWSTR intersectionShaderImport = nullptr, D3D12_HIT_GROUP_TYPE type = D3D12_HIT_GROUP_TYPE_TRIANGLES);
		void AddSignatureToShaders(const std::vector<LPCWSTR>& shaderNames, ID3D12RootSignature** rootSignature);
		void SetGlobalSignature(ID3D12RootSignature** rootSignature);
		void SetMaxPayloadSize(UINT size);
		void SetMaxAttributeSize(UINT size);
		void SetMaxRecursionDepth(UINT depth);
		
		ID3D12StateObject* Build(ID3D12Device5* device);
	private:
		DXILShaderCompiler m_dxilCompiler;
		D3D12_STATE_SUBOBJECT m_start[100];
		UINT m_numSubobjects;

		// Settings
		UINT m_maxPayloadSize;
		UINT m_maxAttributeSize;
		UINT m_maxRecursionDepth;
		ID3D12RootSignature** m_globalRootSignature;

		// Objects to keep in memory until build() is called
		std::vector<LPCWSTR> m_shaderNames;
		std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> m_exportAssociations;
		std::vector<std::vector<D3D12_EXPORT_DESC>> m_exportDescs;
		std::vector<std::vector<LPCWSTR>> m_associationNames;
		std::vector<D3D12_DXIL_LIBRARY_DESC> m_libraryDescs;
		std::vector<D3D12_HIT_GROUP_DESC> m_hitGroupDescs;
	};

	struct ShaderTableData {
		UINT sizeInBytes;
		UINT strideInBytes;
		ID3D12Resource* resource = nullptr;
		void Release() {
			if (resource) {
				resource->Release();
				resource = nullptr;
			}
		}
	};

	class ShaderTableBuilder {
	public:
		ShaderTableBuilder() {};
		ShaderTableBuilder(UINT nInstances, ID3D12StateObject* pso, UINT maxInstanceSize = 32);
		~ShaderTableBuilder();

		void AddShader(const LPCWSTR& shaderName);
		void AddDescriptor(UINT64& descriptor, UINT instance = 0);
		void AddConstants(UINT numConstants, const float* constants, UINT instance = 0);

		ShaderTableData Build(ID3D12Device5* device);

	private:
		std::vector<LPCWSTR> m_shaderNames;
		UINT m_nInstances;
		UINT m_maxInstanceSize;
		char** m_data;
		UINT* m_dataOffset;
		ID3D12StateObjectProperties* m_psoProp;
	};
}