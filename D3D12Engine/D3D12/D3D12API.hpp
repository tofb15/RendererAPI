#pragma once

#include "../RenderAPI.hpp"
#include "D3D12_FDecl.h"

#include <thread>

constexpr unsigned int NUM_SWAP_BUFFERS = 3;
constexpr int NUM_GPU_BUFFERS = 2;

class D3D12VertexBuffer;
class D3D12TextureLoader;
class D3D12VertexBufferLoader;

/*
	Documentation goes here ^^
*/
class D3D12API : public RenderAPI {
public:
	
	D3D12API();
	virtual ~D3D12API();

	// Inherited via Renderer
	virtual bool Initialize() override;
	
	virtual Camera * MakeCamera() override;
	virtual Window * MakeWindow() override;
	virtual Texture * MakeTexture() override;
	virtual Mesh * MakeMesh() override;
	virtual Terrain* MakeTerrain() override;
	virtual Material * MakeMaterial() override;
	virtual RenderState * MakeRenderState() override;
	virtual Technique * MakeTechnique(RenderState* rs, ShaderProgram* sp, ShaderManager* sm) override;
	virtual ShaderManager * MakeShaderManager() override;
	virtual D3D12VertexBuffer * MakeVertexBuffer();
	virtual Renderer* MakeRenderer(const RendererType rendererType) override;

	ID3D12Device5* GetDevice() const;
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

private:	
	bool InitializeDirect3DDevice();

private:

	USHORT m_meshesCreated = 0;
	USHORT m_techniquesCreated = 0;
	USHORT m_texturesCreated = 0;
	USHORT m_particlesSystemCreated = 0;

	UINT m_cbv_srv_uav_size;

	D3D12TextureLoader* m_textureLoader;
	std::thread m_thread_texture;

	// Default resources
	ID3D12Device5*				m_device								= nullptr;
	D3D12VertexBufferLoader*	m_vertexBufferLoader;

	bool m_gpuSupportRaytracing = false;
	UINT m_GPU_buffer_index = 0;
};