#pragma once
#include "D3D12Renderer.h"

//constexpr UINT NUM_ACCELERATION_STRUCTURES = 2;
namespace FusionReactor {
	namespace FusionReactor_DX12 {
		class DXRBase;
		class D3D12Window;
		class D3D12GBufferRenderer : public D3D12Renderer {
		public:
			static constexpr int NUM_GBUFFERS = 4;

			D3D12GBufferRenderer(D3D12API* d3d12);
			~D3D12GBufferRenderer();

			// Inherited via D3D12Renderer
			virtual bool Initialize() override;

		private:
			bool InitializeCommandInterfaces();
			bool InitializeOutputTextures(D3D12Window* window);
			void ResetCommandListAndAllocator(int index);

			void ClearRenderTarget(ID3D12GraphicsCommandList4* cmdlist);
			bool InitializeMatrixStructuredBuffer();

			// Inherited via D3D12Renderer
			virtual void Submit(const SubmissionItem& item, Camera* camera = nullptr, unsigned char layer = 0) override;
			virtual void ClearSubmissions() override;
			virtual void Frame(Window* window, Camera* camera) override;
			virtual void Present(Window* window, GUI* gui = nullptr) override;
			virtual void ClearFrame() override;
			virtual void SetLightSources(const std::vector<LightSource>& lights) override;

			virtual void  SetSetting(const std::string& setting, float value) override;
			virtual void  SetSetting(const std::string& setting, void* value) override;
			virtual float GetSetting(const std::string& setting) override;
			virtual bool  SaveLastFrame(const std::string& file) override;

#ifdef PERFORMANCE_TESTING
			virtual double* GetGPU_Timers(int& nValues, int& firstValue) override;
#endif // PERFORMANCE_TESTING

		private:
			std::vector<SubmissionItem> m_renderItems;
			//std::vector<SubmissionItem> m_NonOpaqueItems;
			std::vector<LightSource> m_lights;

			DXRBase* m_dxrBase;

			ID3D12CommandAllocator* m_commandAllocators[NUM_GPU_BUFFERS] = { nullptr };
			ID3D12GraphicsCommandList4* m_commandLists[NUM_GPU_BUFFERS] = { nullptr };

			//RenderTargets
			ID3D12DescriptorHeap* m_RenderTargetsHeap = nullptr;
			ID3D12Resource* m_outputTextures[NUM_GPU_BUFFERS][NUM_GBUFFERS] = { nullptr };
			//Depth Stencil
			ID3D12DescriptorHeap* m_DepthStencilHeap = nullptr;
			ID3D12Resource* m_DepthStencil[NUM_GPU_BUFFERS] = { nullptr };

			Int2 m_outputDim;

		};
	}
}