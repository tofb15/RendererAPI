#pragma once

#include "../Renderer.hpp"
#include <Windows.h>

struct ID3D12Device4;
struct ID3D12CommandQueue;
struct ID3D12CommandAllocator;
struct IDXGISwapChain4;
struct ID3D12GraphicsCommandList3;

/*
	Documentation goes here ^^
*/
class D3D12Renderer : public Renderer {
public:
	
	D3D12Renderer();

	// Inherited via Renderer
	virtual bool Initialize() override;

	virtual Camera * MakeCamera() override;

	virtual Window * MakeWindow() override;

	virtual Texture * MakeTexture() override;

	virtual Mesh * MakeMesh() override;

	virtual Material * MakeMaterial() override;

	virtual RenderState * MakeRenderState() override;

	virtual Technique * MakeTechnique(Material *, RenderState *) override;

	virtual void Submit(SubmissionItem item) override;

	virtual void ClearSubmissions() override;

	virtual void Frame(Window* window) override;

	virtual void Present() override;

	virtual void ClearFrame() override;

	ID3D12Device4* GetDevice() const;
private:
	//Variables Here
	static const unsigned int NUM_SWAP_BUFFERS = 2; //Number of buffers

#pragma region InitailizeVariables
	ID3D12Device4*				mDevice5			= nullptr;
	ID3D12CommandQueue*			mCommandQueue		= nullptr;
	ID3D12CommandAllocator*		mCommandAllocator	= nullptr;
	IDXGISwapChain4*			mSwapChain4			= nullptr;
	ID3D12GraphicsCommandList3*	mCommandList4		= nullptr;
#pragma endregion


	//Functions Here

	bool InitializeDirect3DDevice(HWND wndHandle);					
	bool InitializeCommandInterfacesAndSwapChain(HWND wndHandle);	
	bool InitializeFenceAndEventHandle();							
	bool InitializeRenderTargets();									
	void InitializeViewportAndScissorRect(unsigned int width, unsigned int height);

};