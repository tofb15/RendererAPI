#pragma once

#include "FusionReactor/src/Window.hpp"
#include "D3D12API.hpp"
#include "DXR/D3D12Utils.h"
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>

class D3D12API;
class D3D12Window : public Window {
public:
	D3D12Window(D3D12API* d3d12);
	virtual ~D3D12Window();

	// Inherited via Window
	/*
		Set the dimensions for the window.
	*/
	virtual void SetDimensions(const Int2& dimensions) override;
	/*
		Set the dimensions for the window.
	*/
	virtual void SetDimensions(int w, int h) override;
	/*
		Set the position of the window.
	*/
	virtual void SetPosition(const Int2& position) override;
	/*
		Set the position of the window.
	*/
	virtual void SetPosition(int x, int y) override;
	/*
		Set the title of the window.
	*/
	virtual void SetTitle(const char* title) override;
	/*
		Initialize and create the window.
		Call Show() to show the window.

		@see Winodw::Show()
	*/
	virtual bool Create(int dimensionX, int dimensionY) override;

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
	void ClearRenderTarget(ID3D12GraphicsCommandList3* commandList);
	/*
		Set the current backbuffer to be used for rendering.

		@see GetCurrentBackBufferIndex()
	*/
	void SetRenderTarget(ID3D12GraphicsCommandList3* commandList);
	HWND GetHWND();

	/*
		@Return a pointer to the Swap Chain Interface
	*/
	IDXGISwapChain4* GetSwapChain();
	D3D12_VIEWPORT* GetViewport();
	D3D12_RECT* GetScissorRect();
	/*
		@Return A D3D12_GPU_DESCRIPTOR_HANDLE*, pointing to the current rendertargets descriptor handle with the index given by GetCurrentBackBufferIndex().
		@See GetCurrentBackBufferIndex()
	*/
	D3D12_GPU_DESCRIPTOR_HANDLE	GetCurrentRenderTargetGPUDescriptorHandle();

	/*
		@Return A ID3D12Resource1*, pointing to the current rendertarget with the index given by GetCurrentBackBufferIndex().
		@See GetCurrentBackBufferIndex()
	*/
	ID3D12Resource1* GetCurrentRenderTargetResource();
	/*
		@Return A ID3D12Resource1**, pointing to an array of rendertarget resources.
	*/
	ID3D12Resource1** GetRenderTargetResources();

	/*
		Used to get the current back buffer index for this specific window. This can be used to syncronize other resources(like Constant Buffers) used to render this frame.
		The Backbuffer index changes each time present() is called on the swap chain.

		@Return current backbuffer index.
	*/
	UINT GetCurrentBackBufferIndex() const;

	/*
		Not used anymore
	*/
	virtual void BeginUIRendering() override;

	virtual void* PrepareTextureForGuiRendering(Texture* texture, bool permanent) override;

	/*
		Not used anymore
	*/
	virtual void EndUIRendering() override;

	ID3D12DescriptorHeap* GetGUIDescriptorHeap();

private:
	bool m_firstResize = true; //Used to skip the first resize message
	IDXGISwapChain4* m_SwapChain4 = nullptr;
	HWND						m_Wnd;
	ID3D12DescriptorHeap* m_GUIDescriptHeap = nullptr;
	D3D12Utils::D3D12_DESCRIPTOR_HANDLE m_descriptorHeap_start;
	D3D12Utils::D3D12_DESCRIPTOR_HANDLE m_unreserved_handle_start;
	D3D12Utils::D3D12_DESCRIPTOR_HANDLE m_unused_handle_start_this_frame;
	UINT m_srv_descriptorSize = 0;
	int m_GuiDescriptorHeapSize = 50;


	ID3D12DescriptorHeap* m_RenderTargetsHeap = nullptr;
	ID3D12Resource1* m_RenderTargets[NUM_SWAP_BUFFERS] = {};
	UINT						m_RenderTargetDescriptorSize = 0;

	ID3D12DescriptorHeap* m_DepthStencilHeap = nullptr;
	ID3D12Resource1* m_DepthStencil = nullptr;

	D3D12_VIEWPORT* m_Viewport;
	D3D12_RECT* m_ScissorRect;

	D3D12API* m_d3d12;

	RAWINPUTDEVICE				m_rawMouseDevice = { 0 };
	Int2						m_mouseMovement;

	int							m_numWaits;

	bool InitializeWindow();		//1.
	bool InitializeSwapChain();		//2.
	bool InitializeRenderTargets();	//3.
	bool InitializeDepthBuffer();	//4.
	bool InitializeRawInput();

	void ApplyResize();
};