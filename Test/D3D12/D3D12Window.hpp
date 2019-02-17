#pragma once

#include "../Window.hpp"
#include <windows.h>
#include "GlobalSettings.hpp"

struct ID3D12CommandQueue;
struct IDXGISwapChain4;

class D3D12Renderer;

class D3D12Window : public Window {
public:
	D3D12Window(D3D12Renderer* renderer);

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

	//Specialised Functions
	HWND				GetHWND()			const;
	ID3D12CommandQueue* GetCommandQueue()	const;
	IDXGISwapChain4*	GetSwapChain()		const;

	UINT		GetCurrentBackBufferIndex() const;
private:
	ID3D12CommandQueue*			mCommandQueue = nullptr;
	IDXGISwapChain4*			mSwapChain4 = nullptr;
	HWND						mWnd;

	ID3D12DescriptorHeap*		mRenderTargetsHeap = nullptr;
	ID3D12Resource1*			mRenderTargets[NUM_SWAP_BUFFERS] = {};
	UINT						mRenderTargetDescriptorSize = 0;

	D3D12Renderer* mRenderer;

	bool InitializeWindow();		//1.
	bool InitializeCommandQueue();	//2.
	bool InitializeSwapChain();		//3.
	bool InitializeRenderTargets();	//4.
};