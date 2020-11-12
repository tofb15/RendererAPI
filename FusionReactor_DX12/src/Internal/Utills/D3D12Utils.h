#pragma once
#include "..\..\D3D12API.hpp"

#include <d3d12.h>
#include <string>
#include <vector>
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		constexpr unsigned int MAX_DEBUG_NAME_LENGHT = 32;
		constexpr unsigned int PARAM_APPEND = -1;

		namespace D3D12Utils {

			const D3D12_HEAP_PROPERTIES sUploadHeapProperties = {
				D3D12_HEAP_TYPE_UPLOAD,
				D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				D3D12_MEMORY_POOL_UNKNOWN,
				0,
				0,
			};

			const D3D12_HEAP_PROPERTIES sDefaultHeapProps = {
				D3D12_HEAP_TYPE_DEFAULT,
				D3D12_CPU_PAGE_PROPERTY_UNKNOWN,
				D3D12_MEMORY_POOL_UNKNOWN,
				0,
				0
			};

			struct D3D12_DESCRIPTOR_HANDLE {
				D3D12_CPU_DESCRIPTOR_HANDLE cdh;
				D3D12_GPU_DESCRIPTOR_HANDLE gdh;

				void operator+= (int inc) {
					cdh.ptr += inc;
					gdh.ptr += inc;
				}

				void operator-= (int inc) {
					cdh.ptr -= inc;
					gdh.ptr -= inc;
				}
			};

			struct D3D12_DESCRIPTOR_HANDLE_BUFFERED {
				D3D12_DESCRIPTOR_HANDLE dh[NUM_GPU_BUFFERS];

				D3D12_DESCRIPTOR_HANDLE& operator[] (int index) {
					return dh[index];
				}

				void operator+= (int inc) {
					for (size_t i = 0; i < NUM_GPU_BUFFERS; i++) {
						dh[i] += inc;
					}
				}

				void operator-= (int inc) {
					for (size_t i = 0; i < NUM_GPU_BUFFERS; i++) {
						dh[i] -= inc;
					}
				}

				D3D12_DESCRIPTOR_HANDLE_BUFFERED& operator= (D3D12_DESCRIPTOR_HANDLE_BUFFERED& other) {
					for (size_t i = 0; i < NUM_GPU_BUFFERS; i++) {
						dh[i] = other.dh[i];
					}
					return *this;
				}
			};

			//=======Helper functions======
			ID3D12Resource* CreateBuffer(ID3D12Device5* device, UINT64 size, D3D12_RESOURCE_FLAGS flags, D3D12_RESOURCE_STATES initState, const D3D12_HEAP_PROPERTIES& heapProps, D3D12_RESOURCE_DESC* bufDesc = nullptr);
			void SetResourceTransitionBarrier(ID3D12GraphicsCommandList* commandList, ID3D12Resource* resource, D3D12_RESOURCE_STATES StateBefore, D3D12_RESOURCE_STATES StateAfter, UINT subResource);


			//=======Helper classes======

			/*
				Used to generate rootSignature.
			*/
			class RootSignature {
			public:
				RootSignature(const wchar_t* debugName = L"");
				~RootSignature();

				void AddCBV(const char* paramName, unsigned int sRegister = PARAM_APPEND, unsigned int rSpace = 0);
				void AddSRV(const char* paramName, unsigned int sRegister = PARAM_APPEND, unsigned int rSpace = 0);
				void AddUAV(const char* paramName, unsigned int sRegister = PARAM_APPEND, unsigned int rSpace = 0);
				void Add32BitConstants(const char* paramName, UINT nConstants, unsigned int sRegister = PARAM_APPEND, unsigned int rSpace = 0);
				void AddDescriptorTable(const char* paramName, const D3D12_DESCRIPTOR_RANGE_TYPE type, unsigned int shaderRegister = PARAM_APPEND, unsigned int space = 0, unsigned int numDescriptors = 1);
				void AddStaticSampler();

				/*
					flags -> D3D12_ROOT_SIGNATURE_FLAG_
				*/
				bool Build(D3D12API* d3d12, const int& flags);
				ID3D12RootSignature** Get();

				operator ID3D12RootSignature* () const { return m_signature; }
				operator ID3D12RootSignature& () const { return (*m_signature); }

			private:
				ID3D12RootSignature* m_signature = nullptr;
				std::vector<std::string> m_rootParamNames;
				std::vector<D3D12_ROOT_PARAMETER> m_rootParams;
				std::vector<D3D12_STATIC_SAMPLER_DESC> m_staticSamplerDescs;

				wchar_t m_debugName[MAX_DEBUG_NAME_LENGHT];
				bool m_isBuilt = false;
			};
		};
	}
}