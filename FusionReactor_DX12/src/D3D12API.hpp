#pragma once

#include "FusionReactor/src/RenderAPI.hpp"
#include "D3D12_FDecl.h"

#include <thread>

namespace FusionReactor {
	namespace FusionReactor_DX12 {

		constexpr unsigned int NUM_SWAP_BUFFERS = 3;
		constexpr int NUM_GPU_BUFFERS = 2;

		class D3D12VertexBuffer;
		class D3D12ShaderManager;
		class D3D12TextureLoader;
		class D3D12VertexBufferLoader;
		class D3D12DescriptorHeapManager;

		/*
			Documentation goes here ^^
		*/
		class D3D12API : public RenderAPI {
		public:

			static RenderAPI* CreateRenderAPI() { return new D3D12API; };
			D3D12API();
			virtual ~D3D12API();

			// Inherited via Renderer
			virtual bool Initialize() override;

			virtual Camera* MakeCamera() override;
			virtual Window* MakeWindow() override;
			virtual Texture* MakeTexture() override;
			virtual Mesh* MakeMesh() override;
			virtual Terrain* MakeTerrain() override;
			virtual Material* MakeMaterial() override;
			virtual RenderState* MakeRenderState() override;
			virtual Technique* MakeTechnique(RenderState* rs, ShaderProgram* sp, ShaderManager* sm) override;
			virtual D3D12VertexBuffer* MakeVertexBuffer();
			virtual D3D12VertexBuffer* MakeVertexBuffer(const D3D12VertexBuffer& buffer);
			virtual Renderer* MakeRenderer(const RendererType rendererType) override;

			virtual ShaderManager* GetShaderManager() override;
			virtual D3D12ShaderManager* GetShaderManager_D3D12();
			virtual D3D12DescriptorHeapManager* GetDescriptorHeapManager();

			/*
				@Return a pointer to the ID3D12Device5 Interface
			*/
			ID3D12Device5* GetDevice() const;
			/*
				@Return a pointer to the Direct Command Queue Interface
			*/
			ID3D12CommandQueue* GetDirectCommandQueue();
			D3D12TextureLoader* GetTextureLoader() const;
			D3D12VertexBufferLoader* GetVertexBufferLoader() const;

			USHORT GetNrMeshesCreated() const;
			USHORT GetNrTechniquesCreated() const;
			USHORT GetNrTexturesCreated() const;
			UINT   GetViewSize();
			UINT   GetGPUBufferIndex();
			void   IncGPUBufferIndex();

			// Helper method to create resources that require one for each swap buffer
			template <typename T>
			std::vector<T> createFrameResource() {
				auto resource = std::vector<T>();
				resource.resize(NUM_GPU_BUFFERS);
				return resource;
			}

			unsigned __int64 SignalFence();
			void WaitForGPU_ALL();
			void WaitForGPU_BUFFERS(int index);
			void WaitForFenceValue(unsigned __int64 value);

		private:
			bool InitializeDirect3DDevice(); //1.
			bool InitializeCommandQueue();	 //2.
			bool InitializeFence();          //3.
		private:

			USHORT m_meshesCreated = 0;
			USHORT m_techniquesCreated = 0;
			USHORT m_texturesCreated = 0;
			USHORT m_particlesSystemCreated = 0;

			UINT m_cbv_srv_uav_size = 0;

			D3D12TextureLoader* m_textureLoader = nullptr;
			D3D12ShaderManager* m_shadermanager = nullptr;
			D3D12DescriptorHeapManager* m_descriptorHeapManager = nullptr;

			// Default resources
			ID3D12Device5* m_device = nullptr;
			ID3D12CommandQueue* m_CommandQueue_direct = nullptr;
			D3D12VertexBufferLoader* m_vertexBufferLoader = nullptr;

			//Fences for the render targets
			ID3D12Fence1* m_Fence = nullptr;
			HANDLE m_EventHandle = 0;
			unsigned __int64 m_FenceValues_GPU_BUFFERS[NUM_GPU_BUFFERS] = { 0 };
			unsigned __int64 m_currentFenceValue = 0;

			bool m_gpuSupportRaytracing = false;
			UINT m_GPU_buffer_index = 0;


		};

	}
}