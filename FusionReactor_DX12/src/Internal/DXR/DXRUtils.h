#pragma once
#include "../../D3D12API.hpp"
#include "../Utills/D3D12Utils.h"
#include "DXILShaderCompiler.h"

#include <Windows.h>
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		namespace DXRUtils {
			class PSOBuilder {
			public:
				PSOBuilder();
				~PSOBuilder();

				bool Initialize();

				D3D12_STATE_SUBOBJECT* Append(const D3D12_STATE_SUBOBJECT_TYPE type, const void* desc);
				//bool AddLibrary(const std::string& shaderPath, const std::vector<LPCWSTR>& names, const std::vector<DxcDefine>& defines = std::vector<DxcDefine>(), std::wstring* errorMessage = nullptr);
				bool AddLibrary(const std::wstring& shaderPath, const std::vector<LPCWSTR>& names, const std::vector<DxcDefine>& defines = std::vector<DxcDefine>(), const std::vector<LPCWSTR>* exportNames = nullptr, std::wstring* errorMessage = nullptr);

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
				UINT m_numSubobjects = 0;

				// Settings
				UINT m_maxPayloadSize = 0;
				UINT m_maxAttributeSize = 0;
				UINT m_maxRecursionDepth = 0;
				ID3D12RootSignature** m_globalRootSignature = nullptr;

				// Objects to keep in memory until build() is called
				std::vector<LPCWSTR> m_shaderNames;
				std::vector<D3D12_SUBOBJECT_TO_EXPORTS_ASSOCIATION> m_exportAssociations;
				std::vector<std::vector<D3D12_EXPORT_DESC>> m_exportDescs;
				std::vector<std::vector<LPCWSTR>> m_associationNames;
				std::vector<D3D12_DXIL_LIBRARY_DESC> m_libraryDescs;
				std::vector<D3D12_HIT_GROUP_DESC> m_hitGroupDescs;
			};

			struct ShaderTableData {
				UINT sizeInBytes = 0;
				UINT strideInBytes = 0;
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
				void AddDescriptor(const UINT64& descriptor, UINT instance = 0);
				void AddConstants(UINT numConstants, const float* constants, UINT instance = 0);

				ShaderTableData Build(ID3D12Device5* device);

			private:
				std::vector<LPCWSTR> m_shaderNames;
				UINT m_nInstances = 0;
				UINT m_maxInstanceSize = 0;
				char** m_data = nullptr;
				UINT* m_dataOffset = nullptr;
				ID3D12StateObjectProperties* m_psoProp = nullptr;
			};
		}
	}
}