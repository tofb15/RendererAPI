#pragma once

#include "../RenderAPI.hpp"
//#include <d3d12.h>
#include <thread>

struct ID3D12Device4;
struct ID3D12CommandAllocator;
struct ID3D12GraphicsCommandList3;

struct ID3D12Resource;
struct ID3D12Fence1;

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

	ID3D12Device4* GetDevice() const;
	D3D12TextureLoader* GetTextureLoader() const;
	D3D12VertexBufferLoader* GetVertexBufferLoader() const;

	USHORT GetNrMeshesCreated() const;
	USHORT GetNrTechniquesCreated() const;
	USHORT GetNrTexturesCreated() const;
	UINT   GetViewSize();

private:
	
	bool InitializeDirect3DDevice();

	USHORT m_meshesCreated = 0;
	USHORT m_techniquesCreated = 0;
	USHORT m_texturesCreated = 0;
	USHORT m_particlesSystemCreated = 0;

	UINT m_cbv_srv_uav_size;

	D3D12TextureLoader* m_textureLoader;
	std::thread m_thread_texture;

	// Default resources
	ID3D12Device4*				m_device								= nullptr;

	// Descriptor heap for texture descriptors
	//ID3D12DescriptorHeap*		m_descriptorHeap[NUM_SWAP_BUFFERS] = { nullptr };

	D3D12VertexBufferLoader*	m_vertexBufferLoader;
	//FullScreenPass* m_fullScreenPass;
	//FXAAPass* m_FXAAPass;
	//D3D12ParticleSystem* m_particleSystem = nullptr;

};