#include "D3D12Window.hpp"
#include <string>
#include <iostream>
#include <d3d12.h>
#include <dxgi1_6.h>
#include "D3D12Renderer.hpp"

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	std::cout << ":	" << std::dec << message << " " << std::hex << message << std::endl;

	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		exit(0);
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);
}

D3D12Window::D3D12Window(D3D12Renderer* renderer)
{
	mRenderer = renderer;
}

void D3D12Window::SetDimensions(Int2 dimensions)
{
}

void D3D12Window::SetDimensions(int w, int h)
{
	dimensions.x = w;
	dimensions.y = h;
}

void D3D12Window::SetPosition(Int2 position)
{
}

void D3D12Window::SetPosition(int x, int y)
{
}

void D3D12Window::SetTitle(const char * title)
{
	this->title = title;
	if (mWnd != NULL)
		SetWindowText(mWnd, title);
}

bool D3D12Window::Create()
{
	if (!InitializeWindow())
		return false;

	if (!InitializeSwapChain())
		return false;

	if (!InitializeRenderTargets())
		return false;

	return true;
}

void D3D12Window::Show()
{
	if (mWnd != NULL)
		ShowWindow(mWnd, 1);
}

void D3D12Window::Hide()
{
}

void D3D12Window::HandleWindowEvents()
{
	MSG msg = { 0 };
	bool CheckMessage = true;

	while (CheckMessage)
	{
		if (PeekMessage(&msg, mWnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

#pragma region eventCatching

			/*If Window events needs to change member variables or call functions it should be done here and not inside WndProc.*/
			//switch (msg.message)
			//{
			//case WM_MOUSELEAVE:
			//case WM_NCMOUSELEAVE: //I think NC refers to the windows borders

			//	/*This event is not called if two windows is intersecting and mouse is traveling between them for some reason*/
			//	mMouseInsideWindow = false;
			//	break;
			//case WM_MOUSEMOVE:
			//case WM_NCMOUSEMOVE: //I think NC refers to the windows borders

			//	/*Set all other windows mMouseInsideWindow to false here maybe?*/
			//	mMouseInsideWindow = true;
			//	break;
			//case WM_SETFOCUS:
			//case WM_MOUSEACTIVATE:
			//	mIsInFocus = true;
			//	break;
			//default:
			//	break;
			//}
			//std::cout << title << ":	" << std::dec << msg.message << " " << std::hex << msg.message << std::endl;
#pragma endregion

		}
		else {
			CheckMessage = false;
		}
	}
}

HWND D3D12Window::GetHWND() const
{
	return mWnd;
}

ID3D12CommandQueue * D3D12Window::GetCommandQueue() const
{
	return mCommandQueue;
}

IDXGISwapChain4 * D3D12Window::GetSwapChain() const
{
	return mSwapChain4;
}

UINT D3D12Window::GetCurrentBackBufferIndex() const
{
	return mSwapChain4->GetCurrentBackBufferIndex();
}

bool D3D12Window::InitializeWindow()
{
	if (mWnd != NULL)
		return false;

	if (dimensions.x == 0 || dimensions.y == 0)
		return false;

	WNDCLASSEX wcex = { 0 };
	wcex.cbSize = sizeof(WNDCLASSEX);
	wcex.lpfnWndProc = WndProc;
	wcex.hInstance = nullptr;
	wcex.lpszClassName = "D3D12 Works!";
	if (!RegisterClassEx(&wcex))
	{
		//return false;
	}


	RECT rc = { 0, 0, dimensions.x, dimensions.y };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false);

	mWnd = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		"D3D12 Works!",
		title,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		rc.right - rc.left,
		rc.bottom - rc.top,
		nullptr,
		nullptr,
		nullptr,
		nullptr);


	return true;
}

bool D3D12Window::InitializeCommandQueue()
{
	HRESULT hr;
	//Describe and create the command queue.
	D3D12_COMMAND_QUEUE_DESC cqd = {};
	hr = mRenderer->GetDevice()->CreateCommandQueue(&cqd, IID_PPV_ARGS(&mCommandQueue));
	if (FAILED(hr))
	{
		return false;
	}

	return true;
}

bool D3D12Window::InitializeSwapChain()
{
	HRESULT hr;

	IDXGIFactory5*	factory = nullptr;
	hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));
	if (FAILED(hr))
	{
		return false;
	}

	//Create swap chain.
	DXGI_SWAP_CHAIN_DESC1 scDesc = {};
	scDesc.Width = 0;
	scDesc.Height = 0;
	scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scDesc.Stereo = FALSE;
	scDesc.SampleDesc.Count = 1;
	scDesc.SampleDesc.Quality = 0;
	scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scDesc.BufferCount = NUM_SWAP_BUFFERS;
	scDesc.Scaling = DXGI_SCALING_NONE;
	scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scDesc.Flags = 0;
	scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	IDXGISwapChain1* swapChain1 = nullptr;

	hr = factory->CreateSwapChainForHwnd(
		mCommandQueue,
		mWnd,
		&scDesc,
		nullptr,
		nullptr,
		&swapChain1);
	if (SUCCEEDED(hr))
	{
		hr = swapChain1->QueryInterface(IID_PPV_ARGS(&mSwapChain4));
		if (SUCCEEDED(hr))
		{
			mSwapChain4->Release();
		}
		else
		{
			return false;
		}
	}
	else
	{
		return false;
	}

	factory->Release();

	return true;
}

bool D3D12Window::InitializeRenderTargets()
{
	//Create descriptor heap for render target views.
	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.NumDescriptors = NUM_SWAP_BUFFERS;
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

	HRESULT hr = mRenderer->GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&mRenderTargetsHeap));
	if (FAILED(hr))
	{
		return false;
	}

	//Create resources for the render targets.
	mRenderTargetDescriptorSize = mRenderer->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//mCBV_SRV_UAV_DescriptorSize = mDevice5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE cdh = mRenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();

	//One RTV for each frame.
	for (UINT n = 0; n < NUM_SWAP_BUFFERS; n++)
	{
		hr = mSwapChain4->GetBuffer(n, IID_PPV_ARGS(&mRenderTargets[n]));
		if (FAILED(hr))
		{
			return false;
		}

		mRenderer->GetDevice()->CreateRenderTargetView(mRenderTargets[n], nullptr, cdh);
		cdh.ptr += mRenderTargetDescriptorSize;
	}

	return true;

	return true;
}
