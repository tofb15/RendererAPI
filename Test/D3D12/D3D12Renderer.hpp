#pragma once

#include "../Renderer.hpp"
#include "GlobalSettings.hpp"
#include <Windows.h>
#include <thread>
#include <map>

struct ID3D12Device4;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct IDXGISwapChain4;
struct ID3D12GraphicsCommandList3;
struct ID3D12DescriptorHeap;
struct ID3D12RootSignature;
struct ID3D12Resource;

class D3D12VertexBuffer;
class D3D12TextureLoader;

/*
	Documentation goes here ^^
*/
class D3D12Renderer : public Renderer {
public:
	
	D3D12Renderer();
	virtual ~D3D12Renderer();

	// Inherited via Renderer
	virtual bool Initialize() override;
	
	virtual Camera * MakeCamera() override;

	virtual Window * MakeWindow() override;

	virtual Texture * MakeTexture() override;

	virtual Mesh * MakeMesh() override;

	virtual Material * MakeMaterial() override;

	virtual RenderState * MakeRenderState() override;

	virtual Technique * MakeTechnique(RenderState* rs, ShaderProgram* sp, ShaderManager* sm) override;

	virtual D3D12VertexBuffer * MakeVertexBuffer();

	virtual ShaderManager * MakeShaderManager() override;

	virtual void Submit(SubmissionItem item) override;

	virtual void ClearSubmissions() override;

	virtual void Frame(Window * window, Camera * c) override;

	virtual void Present(Window * w) override;

	virtual void ClearFrame() override;

	ID3D12Device4* GetDevice() const;
	ID3D12RootSignature* GetRootSignature() const;
	ID3D12GraphicsCommandList3* GetCommandList() const;
	D3D12TextureLoader* GetTextureLoader() const;

private:

	struct SortingItem
	{
		union {
			unsigned int sortingIndex;//4 Bytes
			struct {
				//By setting meshIndex and techniqueIndex, sortingIndex will be set automatically.
				unsigned short meshIndex;		//2 Bytes
				unsigned short techniqueIndex;	//2 Bytes
			};

		};
		//unsigned short techniqueIndex;
		//unsigned short meshIndex;
		SubmissionItem item;
	};

	std::vector<SortingItem> m_items;
	D3D12TextureLoader* m_textureLoader;
	std::thread thread_texture;

	unsigned short m_meshesCreated = 0;
	unsigned short m_techniquesCreated = 0;

#pragma region InitailizeVariables
	ID3D12Device4*				m_device			= nullptr;
	ID3D12CommandAllocator*		mCommandAllocator	= nullptr;
	ID3D12GraphicsCommandList3*	m_commandList		= nullptr;
	ID3D12RootSignature*		mRootSignature		= nullptr;
#pragma endregion

	//ID3D12DescriptorHeap*	mDescriptorHeap[NUM_SWAP_BUFFERS] = {};

	ID3D12Resource*				m_constantBufferResource[NUM_SWAP_BUFFERS] = { nullptr };
	
	// Big descriptor heap resources
	ID3D12DescriptorHeap*		m_descriptorHeap[NUM_SWAP_BUFFERS] = { nullptr };
	unsigned int				m_descriptorHeapSize;

	//Functions Here

	bool InitializeDirect3DDevice();					
	bool InitializeCommandInterfaces();	
	bool InitializeFenceAndEventHandle();
	bool InitializeRootSignature();
	bool InitializeBigConstantBuffer();
	bool InitializeBigDescriptorHeap();


};