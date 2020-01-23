#pragma once
#include "../../D3D12API.hpp"
#include "../Temp/DXILShaderCompiler.h"

#include <Windows.h>

namespace DXRUtils {
	typedef enum class GROUP_TYPE
	{
		TRIANGLES = 0,
		PROCEDURAL_PRIMITIVE = 0x1
	} 	GROUP_TYPE;

	typedef enum class STATE_SUBOBJECT_TYPE
	{
		STATE_SUBOBJECT_TYPE_GLOBAL_ROOT_SIGNATURE = 1,
		STATE_SUBOBJECT_TYPE_LOCAL_ROOT_SIGNATURE = 2,
	} 	STATE_SUBOBJECT_TYPE;

	class PSOBuilder {
	public:
		PSOBuilder();
		~PSOBuilder();

		bool Initialize();

		D3D12_STATE_SUBOBJECT* Append(const STATE_SUBOBJECT_TYPE type, const void* desc);
		void AddLibrary(const std::string& shaderPath, const std::vector<LPCWSTR>& names, const std::vector<DxcDefine>& defines = std::vector<DxcDefine>());
		void AddHitGroup(LPCWSTR exportName, LPCWSTR closestHitShaderImport, LPCWSTR anyHitShaderImport = nullptr, LPCWSTR intersectionShaderImport = nullptr, GROUP_TYPE type = GROUP_TYPE::TRIANGLES);
		void AddSignatureToShaders(const std::vector<LPCWSTR>& shaderNames, ID3D12RootSignature** rootSignature);
		void SetGlobalSignature(ID3D12RootSignature** rootSignature);
		void SetMaxPayloadSize(UINT size);
		void SetMaxAttributeSize(UINT size);
		void SetMaxRecursionDepth(UINT depth);

	private:

	};
}