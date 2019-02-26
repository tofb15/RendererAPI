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
	/*
		Set the dimensions for the window.
	*/
	virtual void SetDimensions(Int2 dimensions) override;
	/*
		Set the dimensions for the window.
	*/
	virtual void SetDimensions(int w, int h) override;
	/*
		Set the position of the window.
	*/
	virtual void SetPosition(Int2 position) override;
	/*
		Set the position of the window.
	*/
	virtual void SetPosition(int x, int y) override;
	/*
		Set the title of the window.
	*/
	virtual void SetTitle(const char * title) override;
	/*
		Initialize and create the window.
		Call Show() to show the window.

		@see Winodw::Show()
	*/
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
	ID3D12CommandQueue*			m_CommandQueue = nullptr;
	IDXGISwapChain4*			m_SwapChain4 = nullptr;
	HWND						m_Wnd;

	ID3D12DescriptorHeap*		m_RenderTargetsHeap = nullptr;
	ID3D12Resource1*			m_RenderTargets[NUM_SWAP_BUFFERS] = {};
	UINT						m_RenderTargetDescriptorSize = 0;

	ID3D12DescriptorHeap*		m_DepthStencilHeap = nullptr;
	ID3D12Resource1*			m_DepthStencil = nullptr;

	D3D12_VIEWPORT*				m_Viewport;
	D3D12_RECT*					m_ScissorRect;

	D3D12Renderer* m_Renderer;

	//Fences for the render targets
	ID3D12Fence1*				m_Fence[NUM_SWAP_BUFFERS] = { nullptr };
	HANDLE						m_EventHandle[NUM_SWAP_BUFFERS] = { nullptr };
	UINT64						m_FenceValue[NUM_SWAP_BUFFERS] = { 0 };

	bool InitializeWindow();		//1.
	bool InitializeCommandQueue();	//2.
	bool InitializeSwapChain();		//3.
	bool InitializeRenderTargets();	//4.
	bool InitializeDepthBuffer();	//5.
};