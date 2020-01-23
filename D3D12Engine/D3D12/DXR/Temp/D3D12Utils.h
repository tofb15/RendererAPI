#pragma once

#include "..\..\D3D12_FDecl.h"
#include "..\..\D3D12API.hpp"

#include <string>
#include <vector>

constexpr unsigned int MAX_DEBUG_NAME_LENGHT = 32;
constexpr unsigned int PARAM_APPEND = -1;
constexpr unsigned int MAX_RAY_RECURSION_DEPTH = 2;

namespace D3D12Utils{

	class RootSignature
	{
	public:
		RootSignature(const wchar_t* debugName = L"");
		~RootSignature();

		void AddCBV(const char* paramName, unsigned int sRegister = PARAM_APPEND, unsigned int rSpace = 0);
		void AddSRV(const char* paramName, unsigned int sRegister = PARAM_APPEND, unsigned int rSpace = 0);
		void AddUAV(const char* paramName, unsigned int sRegister = PARAM_APPEND, unsigned int rSpace = 0);
		void AddDescriptorTable(const char* paramName, const int type, unsigned int shaderRegister = PARAM_APPEND, unsigned int space = 0, unsigned int numDescriptors = 1);
		void AddStaticSampler();

		/*
			flags -> D3D12_ROOT_SIGNATURE_FLAG_
		*/
		bool Build(D3D12API* d3d12, const int& flags);
		ID3D12RootSignature** Get();

		operator ID3D12RootSignature*() const { return m_signature; }
		operator ID3D12RootSignature&() const { return (*m_signature); }

	private:
		ID3D12RootSignature* m_signature = nullptr;
		std::vector<std::string> m_rootParamNames;
		std::vector<D3D12_ROOT_PARAMETER> m_rootParams;
		std::vector<D3D12_STATIC_SAMPLER_DESC> m_staticSamplerDescs;

		wchar_t m_debugName[MAX_DEBUG_NAME_LENGHT];
		bool m_isBuilt = false;
	};
};