#pragma once

#include "../Window.hpp"
#include <windows.h>
#include "GlobalSettings.hpp"

//struct ID3D12CommandQueue;
//struct IDXGISwapChain4;
//struct ID3D12DescriptorHeap;
//struct ID3D12Resource1;
//struct D3D12_VIEWPORT;
//struct D3D12_RECT;

#include <d3d12.h>
#include <dxgi1_6.h>
class D3D12Renderer;

class D3D12Window : public Window {
public:
	D3D12Window(D3D12Renderer* renderer);
	virtual ~D3D12Window();

	// Inherited via Window
	virtual void SetDimensions(Int2 dimensions) override;

	virtual void SetDimensions(int w, int h) override;

	virtual void SetPosition(Int2 position) override;

	virtual void SetPosition(int x, int y) override;

	virtual void SetTitle(const char * title) override;

	virtual bool Create() override;

	virtual void Show() override;

	virtual void Hide() override;

	virtual void HandleWindowEvents() override;

	virtual bool WindowClosed() override;

	//Specialised Functions
	/*
		Clears the current render target with the current backbuffer index.
		The rendertarget does not have to be set with SetRenderTarget() to be cleared.

		@see GetCurrentBackBufferIndex()
	*/
	void ClearRenderTarget(ID3D12GraphicsCommandList3*	commandList);
	/*
		Set the current backbuffer to be used for rendering.

		@see GetCurrentBackBufferIndex()
	*/
	void SetRenderTarget(ID3D12GraphicsCommandList3*	commandList);
	HWND				GetHWND();
	/*
		@Return a pointer to the Command Queue Interface
	*/
	ID3D12CommandQueue* GetCommandQueue();
	/*
		@Return a pointer to the Swap Chain Interface
	*/
	IDXGISwapChain4*	GetSwapChain();
	D3D12_VIEWPORT*		GetViewport();
	D3D12_RECT*			GetScissorRect();
	/*
		@Return A ID3D12Resource1*, pointing to the current rendertarget with the index given by GetCurrentBackBufferIndex().
		@See GetCurrentBackBufferIndex()
	*/
	ID3D12Resource1*	GetCurrentRenderTargetResource();
	/*
		Used to get the current back buffer index for this specific window. This can be used to syncronize other resources(like Constant Buffers) used to render this frame.
		The Backbuffer index changes each time present() is called on the swap chain.

		@Return current backbuffer index.
	*/
	UINT GetCurrentBackBufferIndex() const;
	void WaitForGPU();

private:
	ID3D12CommandQueue*			mCommandQueue = nullptr;
	IDXGISwapChain4*			mSwapChain4 = nullptr;
	HWND						mWnd;

	ID3D12DescriptorHeap*		mRenderTargetsHeap = nullptr;
	ID3D12Resource1*			mRenderTargets[NUM_SWAP_BUFFERS] = {};
	UINT						mRenderTargetDescriptorSize = 0;

	D3D12_VIEWPORT*				mViewport;
	D3D12_RECT*					mScissorRect;

	D3D12Renderer* mRenderer;

	//Fences for the render targets
	ID3D12Fence1*				mFence = nullptr;
	HANDLE						mEventHandle = nullptr;
	UINT64						mFenceValue = 0;

	bool InitializeWindow();		//1.
	bool InitializeCommandQueue();	//2.
	bool InitializeSwapChain();		//3.
	bool InitializeRenderTargets();	//4.
};