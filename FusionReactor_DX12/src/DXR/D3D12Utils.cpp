#include "stdafx.h"
#include "D3D12Utils.h"
#include <comdef.h>
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		D3D12Utils::RootSignature::RootSignature(const wchar_t* debugName) {
			wcscpy_s(m_debugName, MAX_DEBUG_NAME_LENGHT, debugName);
		}

		D3D12Utils::RootSignature::~RootSignature() {
			for (auto& param : m_rootParams) {
				if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
					delete param.DescriptorTable.pDescriptorRanges;
			}
			if (m_signature) {
				m_signature->Release();
			}
		}

		void D3D12Utils::RootSignature::AddDescriptorTable(const char* paramName, const D3D12_DESCRIPTOR_RANGE_TYPE type, unsigned int shaderRegister, unsigned int space, unsigned int numDescriptors) {
			D3D12_DESCRIPTOR_RANGE* range = MY_NEW D3D12_DESCRIPTOR_RANGE;
			range->BaseShaderRegister = shaderRegister;
			range->RegisterSpace = space;
			range->NumDescriptors = numDescriptors;
			range->RangeType = type;
			range->OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
			param.DescriptorTable.NumDescriptorRanges = 1;
			param.DescriptorTable.pDescriptorRanges = range;

			m_rootParams.emplace_back(param);
			m_rootParamNames.emplace_back(paramName);
		}

		void D3D12Utils::RootSignature::AddCBV(const char* paramName, unsigned int sRegister, unsigned int rSpace) {
			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
			param.Descriptor.ShaderRegister = (sRegister == PARAM_APPEND) ? m_rootParams.size() : sRegister;
			param.Descriptor.RegisterSpace = rSpace;

			m_rootParams.emplace_back(param);
			m_rootParamNames.emplace_back(paramName);
		}

		void D3D12Utils::RootSignature::AddSRV(const char* paramName, unsigned int sRegister, unsigned int rSpace) {
			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_SRV;
			param.Descriptor.ShaderRegister = (sRegister == PARAM_APPEND) ? m_rootParams.size() : sRegister;
			param.Descriptor.RegisterSpace = rSpace;

			m_rootParams.emplace_back(param);
			m_rootParamNames.emplace_back(paramName);
		}

		void D3D12Utils::RootSignature::AddUAV(const char* paramName, unsigned int sRegister, unsigned int rSpace) {
			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_UAV;
			param.Descriptor.ShaderRegister = (sRegister == PARAM_APPEND) ? m_rootParams.size() : sRegister;
			param.Descriptor.RegisterSpace = rSpace;

			m_rootParams.emplace_back(param);
			m_rootParamNames.emplace_back(paramName);
		}

		void D3D12Utils::RootSignature::Add32BitConstants(const char* paramName, UINT nConstants, unsigned int sRegister, unsigned int rSpace) {
			D3D12_ROOT_PARAMETER param = {};
			param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
			param.Constants.Num32BitValues = nConstants;
			param.Constants.RegisterSpace = rSpace;
			param.Constants.ShaderRegister = sRegister;

			m_rootParams.emplace_back(param);
			m_rootParamNames.emplace_back(paramName);
		}

		void D3D12Utils::RootSignature::AddStaticSampler() {
			D3D12_STATIC_SAMPLER_DESC defaultSampler = { D3D12_FILTER_MIN_MAG_MIP_LINEAR, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, D3D12_TEXTURE_ADDRESS_MODE_WRAP, 0.f, 1, D3D12_COMPARISON_FUNC_ALWAYS, D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE, 0.f, FLT_MAX, 0, 0, D3D12_SHADER_VISIBILITY_ALL };
			m_staticSamplerDescs.push_back(defaultSampler);
		}

		bool D3D12Utils::RootSignature::Build(D3D12API* d3d12, const int& flags) {
			D3D12_ROOT_SIGNATURE_DESC desc = {};
			desc.NumParameters = (UINT)m_rootParams.size();
			desc.pParameters = m_rootParams.data();
			desc.Flags = (D3D12_ROOT_SIGNATURE_FLAGS)(flags);
			desc.NumStaticSamplers = (UINT)m_staticSamplerDescs.size();
			desc.pStaticSamplers = m_staticSamplerDescs.data();

			ID3DBlob* sigBlob = nullptr;
			ID3DBlob* errorBlob = nullptr;
			HRESULT hr;

			hr = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &sigBlob, &errorBlob), errorBlob;
			if (FAILED(hr)) {
				return false;
			}

			hr = d3d12->GetDevice()->CreateRootSignature(0, sigBlob->GetBufferPointer(), sigBlob->GetBufferSize(), IID_PPV_ARGS(&m_signature));

			if (FAILED(hr)) {
				return false;
			}

			m_signature->SetName(m_debugName);

			// Delete memory allocated by descriptor tables
			for (auto& param : m_rootParams) {
				if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
					delete param.DescriptorTable.pDescriptorRanges;
			}

			// Delete data that is not needed after build
			m_rootParams.clear();
			m_staticSamplerDescs.clear();

			if (sigBlob) {
				sigBlob->Release();
			}
			if (errorBlob) {
				errorBlob->Release();
			}
			return true;
		}

		ID3D12RootSignature** D3D12Utils::RootSignature::Get() {
			return &m_signature;
		}

		ID3D12Resource* D3D12Utils::CreateBuffer(ID3D12Device5* device, UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC* bufDesc) {
			D3D12_RESOURCE_DESC newBufDesc = {};
			if (!bufDesc) {
				newBufDesc.Alignment = 0;
				newBufDesc.DepthOrArraySize = 1;
				newBufDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
				newBufDesc.Flags = flags;
				newBufDesc.Format = DXGI_FORMAT_UNKNOWN;
				newBufDesc.Height = 1;
				newBufDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
				newBufDesc.MipLevels = 1;
				newBufDesc.SampleDesc.Count = 1;
				newBufDesc.SampleDesc.Quality = 0;
				newBufDesc.Width = size;

				bufDesc = &newBufDesc;
			}

			ID3D12Resource* pBuffer = nullptr;
			auto hr = device->CreateCommittedResource(&heapProps, D3D12_HEAP_FLAG_NONE, bufDesc, initState, nullptr, IID_PPV_ARGS(&pBuffer));
			if (FAILED(hr)) {
				_com_error err(hr);
				std::cout << err.ErrorMessage() << std::endl;

				hr = device->GetDeviceRemovedReason();
				_com_error err2(hr);
				std::cout << err2.ErrorMessage() << std::endl;
			}
			return pBuffer;
		}

		void D3D12Utils::SetResourceTransitionBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter, UINT subResource) {
			D3D12_RESOURCE_BARRIER barrierDesc = {};

			barrierDesc.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
			barrierDesc.Transition.pResource = resource;
			barrierDesc.Transition.Subresource = subResource;
			barrierDesc.Transition.StateBefore = StateBefore;
			barrierDesc.Transition.StateAfter = StateAfter;

			commandList->ResourceBarrier(1, &barrierDesc);

		}
	}
}