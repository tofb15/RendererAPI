#pragma once

#include <Windows.h>
#include <vector>
#include <dxcapi.h>
#include <string>
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		class DXILShaderCompiler {
		public:

			struct Desc {
				LPCVOID source = nullptr;
				UINT32 sourceSize = 0;
				LPCWSTR filePath = nullptr;
				LPCWSTR entryPoint = nullptr;
				LPCWSTR targetProfile = nullptr;
				std::vector<LPCWSTR> compileArguments;
				std::vector<DxcDefine> defines;
			};

			DXILShaderCompiler();
			~DXILShaderCompiler();

			HRESULT init();

			// Compiles a shader into a blob
			// Compiles from source if source != nullptr, otherwise from file
			HRESULT compile(Desc* desc, IDxcBlob** ppResult, std::wstring* errorMessage = nullptr);

		private:
			IDxcLibrary* m_library = nullptr;
			IDxcCompiler* m_compiler = nullptr;
			IDxcIncludeHandler* m_includeHandler = nullptr;
			IDxcLinker* m_linker = nullptr;

		};
	}
}