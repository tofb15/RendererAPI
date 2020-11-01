#include "stdafx.h"
#include "D3D12GBufferRenderer.h"
#include "..\D3D12Camera.hpp"
#include "..\D3D12Window.hpp"
#include "..\D3D12Technique.hpp"
#include "..\D3D12Mesh.hpp"
#include "..\D3D12Texture.hpp"
#include "..\D3D12VertexBuffer.hpp"
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		D3D12GBufferRenderer::D3D12GBufferRenderer(D3D12API* d3d12) : D3D12Renderer(d3d12) {

		}

		D3D12GBufferRenderer::~D3D12GBufferRenderer() {

		}

		bool D3D12GBufferRenderer::Initialize() {
			if (!InitializeCommandInterfaces()) {
				return false;
			}

			return true;
		}

		bool D3D12GBufferRenderer::InitializeCommandInterfaces() {
			HRESULT hr;

			for (size_t i = 0; i < NUM_GPU_BUFFERS; i++) {
				hr = m_d3d12->GetDevice()->CreateCommandAllocator(
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					IID_PPV_ARGS(&m_commandAllocators[i]));
				if (FAILED(hr)) {
					return false;
				}

				//Create command list.
				hr = m_d3d12->GetDevice()->CreateCommandList(
					0,
					D3D12_COMMAND_LIST_TYPE_DIRECT,
					m_commandAllocators[i],
					nullptr,
					IID_PPV_ARGS(&m_commandLists[i]));
				if (FAILED(hr)) {
					return false;
				}

				//Command lists are created in the recording state. Since there is nothing to
				//record right now and the main loop expects it to be closed, we close it.
				m_commandLists[i]->Close();
			}

			return true;
		}

		bool D3D12GBufferRenderer::InitializeOutputTextures(D3D12Window* window) {
			//If window output has not changed keep the old resources, else create new
			if (window->GetDimensions() == m_outputDim) {
				return true;
			}
			m_outputDim = window->GetDimensions();
			//TODO: A Wait for GPU is Needed here

			//====================Create descriptor heap for RTVs====================
			D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
			dhd.NumDescriptors = NUM_GPU_BUFFERS * NUM_GBUFFERS;
			dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

			if (m_RenderTargetsHeap) {
				m_RenderTargetsHeap->Release();
			}
			HRESULT hr = m_d3d12->GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_RenderTargetsHeap));
			if (FAILED(hr)) {
				return false;
			}
			m_RenderTargetsHeap->SetName(L"GPass - RT DescHeap");

			//====================Create Resources AND RTVs====================
			D3D12_RESOURCE_DESC textureDesc = {};
			textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			textureDesc.Width = m_outputDim.x;
			textureDesc.Height = m_outputDim.y;
			textureDesc.DepthOrArraySize = 1;
			textureDesc.MipLevels = 1;
			textureDesc.SampleDesc.Count = 1;
			textureDesc.SampleDesc.Quality = 0;
			textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			textureDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS | D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
			textureDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
			D3D12_CLEAR_VALUE clearValue = { textureDesc.Format, { 0.00f, 0.00f, 0.00f, 1.0f } };

			UINT rtvSize = m_d3d12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
			for (int i = 0; i < NUM_GPU_BUFFERS; i++) {
				for (size_t j = 0; j < NUM_GBUFFERS; j++) {
					if (m_outputTextures[i][j]) {
						m_outputTextures[i][j]->Release();
					}
					hr = m_d3d12->GetDevice()->CreateCommittedResource(&D3D12Utils::sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &textureDesc, D3D12_RESOURCE_STATE_UNORDERED_ACCESS, &clearValue, IID_PPV_ARGS(&m_outputTextures[i][j]));
					m_outputTextures[i][j]->SetName((L"GBuffer Output Texture #" + std::to_wstring(i) + L"." + std::to_wstring(j)).c_str());
					if (FAILED(hr)) {
						return false;
					}

					m_d3d12->GetDevice()->CreateRenderTargetView(m_outputTextures[i][j], nullptr, cdh);
					cdh.ptr += rtvSize;
				}
			}

			//====================Create Depth Buffers====================

			D3D12_RESOURCE_DESC resourceDesc = {};
			resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
			resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
			resourceDesc.Width = m_outputDim.x; //Use the dimensions of the window
			resourceDesc.Height = m_outputDim.y; //Use the dimensions of the window
			resourceDesc.DepthOrArraySize = 1;
			resourceDesc.MipLevels = 0;
			resourceDesc.SampleDesc.Count = 1;
			resourceDesc.SampleDesc.Quality = 0;
			resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

			//D3D12_DEPTH_STENCIL_DESC dsd = {};
			//dsd.DepthEnable = true;

			dhd = { 0 };
			dhd.NumDescriptors = NUM_GPU_BUFFERS;
			dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			hr = m_d3d12->GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_DepthStencilHeap));
			if (FAILED(hr)) {
				return false;
			}

			//Create Depth Stencil Resources and Views
			D3D12_CLEAR_VALUE cv = {};
			cv.Format = DXGI_FORMAT_D32_FLOAT;
			cv.DepthStencil.Depth = 1.0f;
			cv.DepthStencil.Stencil = 0;

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvd = { };
			dsvd.Format = DXGI_FORMAT_D32_FLOAT;
			dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvd.Flags = D3D12_DSV_FLAG_NONE;
			dsvd.Texture2D.MipSlice = 0;

			UINT dsvSize = m_d3d12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			D3D12_CPU_DESCRIPTOR_HANDLE cdh_depth = m_DepthStencilHeap->GetCPUDescriptorHandleForHeapStart();
			for (size_t i = 0; i < NUM_GPU_BUFFERS; i++) {
				HRESULT hr = m_d3d12->GetDevice()->CreateCommittedResource(&D3D12Utils::sDefaultHeapProps, D3D12_HEAP_FLAG_NONE, &resourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &cv, IID_PPV_ARGS(&m_DepthStencil[i]));
				if (FAILED(hr)) {
					return false;
				}

				m_d3d12->GetDevice()->CreateDepthStencilView(m_DepthStencil[i], &dsvd, cdh_depth);
				cdh.ptr += dsvSize;
			}

			return true;
		}

		void D3D12GBufferRenderer::ResetCommandListAndAllocator(int index) {
			HRESULT hr;
			hr = m_commandAllocators[index]->Reset();
			if (!SUCCEEDED(hr)) {
				printf("Error: Command allocator %d failed to reset\n", index);
			}

			hr = m_commandLists[index]->Reset(m_commandAllocators[index], nullptr);
			if (!SUCCEEDED(hr)) {
				printf("Error: Command list %d failed to reset\n", index);
			}
		}

		void D3D12GBufferRenderer::Submit(const SubmissionItem& item, Camera* camera, unsigned char layer) {
			m_renderItems.emplace_back(item);
		}

		void D3D12GBufferRenderer::ClearSubmissions() {
			m_renderItems.clear();
		}

		void D3D12GBufferRenderer::Frame(Window* window, Camera* camera) {
			UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
			InitializeOutputTextures(static_cast<D3D12Window*>(window));

			ResetCommandListAndAllocator(bufferIndex);
			if (m_renderItems.size() == 0) {
				return;
			}
			ID3D12GraphicsCommandList4* cmdlist = m_commandLists[bufferIndex];

			//Record The Frame
			//TODO: Copy Matrix Data
			//TODO: Setup Texture Data

			//TODO: Set Render Target Barrier
			//TODO: Clear Render Target


				//TODO: SetDescriptorHeaps
				//TODO: SetPipelineState
				//TODO: SetGraphicsRootSignature
				//TODO: SetGraphicsRootSignatureVariables

				//TODO: IASetPrimitiveTopology
				//TODO: RSSetViewports
				//TODO: RSSetScissorRects
				//TODO: SetRenderTarget
				//TODO: IASetVertexBuffers
				//TODO: DrawInstanced
		}

		void D3D12GBufferRenderer::Present(Window* window, GUI* gui) {

		}

		void D3D12GBufferRenderer::ClearFrame() {

		}

		void D3D12GBufferRenderer::SetLightSources(const std::vector<LightSource>& lights) {

		}

		void D3D12GBufferRenderer::SetSetting(const std::string& setting, float value) {

		}

		void D3D12GBufferRenderer::SetSetting(const std::string& setting, void* value) {

		}

		float D3D12GBufferRenderer::GetSetting(const std::string& setting) {
			return 0.0f;
		}

		bool D3D12GBufferRenderer::SaveLastFrame(const std::string& file) {
			return false;
		}

		void D3D12GBufferRenderer::ClearRenderTarget(ID3D12GraphicsCommandList4* cmdlist) {
			UINT bufferIndex = m_d3d12->GetGPUBufferIndex();
		}
	}
}