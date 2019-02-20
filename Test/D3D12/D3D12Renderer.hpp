#pragma once

#include "../Renderer.hpp"
#include "GlobalSettings.hpp"
#include <Windows.h>

struct ID3D12Device4;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct IDXGISwapChain4;
struct ID3D12GraphicsCommandList3;
struct ID3D12DescriptorHeap;
struct ID3D12RootSignature;

class D3D12VertexBuffer;

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

private:

	std::vector<SubmissionItem> items;

#pragma region InitailizeVariables
	ID3D12Device4*				mDevice5			= nullptr;
	ID3D12CommandAllocator*		mCommandAllocator	= nullptr;
	ID3D12GraphicsCommandList3*	mCommandList4		= nullptr;
	ID3D12RootSignature*		mRootSignature		= nullptr;
#pragma endregion

	//ID3D12DescriptorHeap*	mDescriptorHeap[NUM_SWAP_BUFFERS] = {};

	//Functions Here

	bool InitializeDirect3DDevice();					
	bool InitializeCommandInterfaces();	
	bool InitializeFenceAndEventHandle();
	bool InitializeRootSignature();


};