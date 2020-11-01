#pragma once
#include "FusionReactor/src/ShaderManager.hpp"
#include <vector>
#include <map>
#include <string>
#include <d3d12.h>
#include <unordered_map>
#include <Windows.h>
#include "DXR/D3D12Utils.h"
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		struct ShaderDefine {
			std::wstring define;
			_Maybenull_ std::wstring value;
		};
		class D3D12API;

		class D3D12ShaderManager : public ShaderManager {
		public:

			D3D12ShaderManager(D3D12API* api);
			bool Initialize();

			virtual ~D3D12ShaderManager();

			//virtual int CreateShaderProgram(Shader VS, Shader GS, Shader PS) override;

			std::string GetVertexDefines(ShaderHandle index) const;
			ID3DBlob* GetShaderBlob(const ShaderHandle& shader);

			bool CreateRaytracingPSO(const std::vector<ShaderDefine>* _defines);
			bool NeedsPSORebuild();

			// Inherited via ShaderManager
			virtual ShaderHandle RegisterShader(const ShaderDescription& shaderDescription) override;
			virtual ShaderProgramHandle RegisterShaderProgram(const ShaderProgramDescription& shaderDescription) override;
			virtual void RecompileShaders();

			const LPCWSTR GetRaygenShaderIdentifier();
			const LPCWSTR GetMissShaderIdentifier();
			const LPCWSTR GetShadowMissShaderIdentifier();
			const LPCWSTR GetHitGroupIdentifier(ShaderProgramHandle sph);
			bool IsHitGroupOpaque(ShaderProgramHandle sph);

			////////////////////////////////////////////////////////////
				/*
				Stores all compiled dxr shaders and shader groups as one single state object.
			*/
			//===Root Signatures===
			ID3D12StateObject* m_rtxPipelineState = nullptr;
			D3D12Utils::RootSignature m_globalRootSignature;
			//TODO: generate local signatures somehow
			D3D12Utils::RootSignature m_localRootSignature_rayGen;
			D3D12Utils::RootSignature m_localRootSignature_miss;
			D3D12Utils::RootSignature m_localRootSignature_hitGroups;
			D3D12Utils::RootSignature m_localRootSignature_empty;
		protected:

			/*
				Stores all compiled raster shaders as induvidual blobs. Raster is no longer supported but might get reintrodused at a later point.
			*/
			std::unordered_map<ShaderHandle, ID3DBlob*> m_raster_shader_blobs;
			std::unordered_map<ShaderHandle, std::string> m_vertexDefines;

			D3D12API* m_d3d12 = nullptr;

		private:
			bool m_psoNeedsRebuild = true;
			int m_identifierCounter = 0;

			ShaderHandle m_nextShaderHandle = 0;
			ShaderProgramHandle m_nextShaderProgramHandle = 0;

			std::unordered_map<std::wstring, std::vector<ShaderHandle>> m_dxr_shader_libraries;
			std::unordered_map<ShaderHandle, ShaderDescription> m_dxr_shader_descriptions;
			std::unordered_map<ShaderHandle, std::wstring> m_dxr_shader_identifiers;
			std::unordered_map<ShaderProgramHandle, ShaderProgramDescription> m_dxr_hitgroups_descriptions;
			std::unordered_map<ShaderProgramHandle, std::wstring> m_dxr_hitgroups_identifiers;

			//===Non-hitgroup shaders==
			const std::wstring m_non_hit_group_shader_file = L"../Shaders/D3D12/DXR/final.hlsl";
			const std::wstring m_shader_rayGenName = L"rayGen";
			const std::wstring m_shader_missName = L"miss";
			const std::wstring m_shader_shadowMissName = L"shadow_GeometryMiss";

			ShaderHandle m_rayGenShader = -1;
			ShaderHandle m_MissShader = -1;
			ShaderHandle m_ShadowMissShader = -1;


			// Used to compile raster shaders. Raster is no longer supported but might get reintrodused at a later point.
			virtual ID3DBlob* CompileShader(const ShaderDescription& sd) /*override*/;
			std::wstring GenerateUniqueIdentifier();
			bool InitializeDXRRootSignatures();
			// TODO: create them dynamically after parsing the shader source (like ShaderPipeline does)
			bool CreateDXRGlobalRootSignature();
			bool CreateRayGenLocalRootSignature();
			bool CreateHitGroupLocalRootSignature();
			bool CreateMissLocalRootSignature();
			bool CreateEmptyLocalRootSignature();
		};
	}
}