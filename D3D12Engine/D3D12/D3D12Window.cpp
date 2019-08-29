#include "D3D12Window.hpp"
#include <string>
#include <iostream>

#include <DirectXMath.h>

#include "D3D12Renderer.hpp"

#pragma comment (lib, "DXGI.lib")
//#pragma comment (lib, "d3d12.lib")

static bool quit = false;

BYTE g_rawInputBuffer[64];

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	//std::cout << ":	" << std::dec << message << " " << std::hex << message << std::endl;


	/*
		Catch Global Winodw Events.
	*/
	switch (message)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		quit = true;
		break;
	case WM_KEYDOWN:
	{
		short key = static_cast<short>(wParam);
		//std::cout << std::dec << key << std::endl;
		Window::GetGlobalWindowInputHandler().SetKeyDown(static_cast<char>(key), true);
	}
		break;
	case WM_KEYUP:
	{
		short key = static_cast<short>(wParam);
		Window::GetGlobalWindowInputHandler().SetKeyDown(static_cast<char>(key), false);

		if (lParam >> 30 & 1)
		{
			Window::GetGlobalWindowInputHandler().SetKeyPressed(static_cast<char>(key), true);
		}

	}
		break;
	}


	return DefWindowProc(hWnd, message, wParam, lParam);
}

D3D12Window::D3D12Window(D3D12Renderer* renderer)
{
	m_Renderer = renderer;
	m_Viewport = new D3D12_VIEWPORT;
	m_ScissorRect = new D3D12_RECT;

	m_Viewport->TopLeftX = 0.0f;
	m_Viewport->TopLeftY = 0.0f;
	m_Viewport->Width = (float)0;
	m_Viewport->Height = (float)0;
	m_Viewport->MinDepth = 0.0f;
	m_Viewport->MaxDepth = 1.0f;

	m_ScissorRect->left = (long)0;
	m_ScissorRect->right = (long)0;
	m_ScissorRect->top = (long)0;
	m_ScissorRect->bottom = (long)0;

	m_numWaits = 0;
}

D3D12Window::~D3D12Window()
{
	delete m_Viewport;
	delete m_ScissorRect;

	// Wait until each queue has finished executing before releasing resources
	int currentBackBuffer = m_SwapChain4->GetCurrentBackBufferIndex();
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		WaitForGPU((currentBackBuffer + i) % NUM_SWAP_BUFFERS);
	}

	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		m_RenderTargets[i]->Release();
		m_Fence[i]->Release();
	}
	m_RenderTargetsHeap->Release();
	m_DepthStencil->Release();
	m_DepthStencilHeap->Release();
	m_CommandQueue->Release();
	m_SwapChain4->Release();
}

void D3D12Window::SetDimensions(Int2 dimensions)
{
	SetDimensions(dimensions.x, dimensions.y);
}

void D3D12Window::SetDimensions(int w, int h)
{
	m_dimensions.x = w;
	m_dimensions.y = h;

	m_Viewport->Width = (float)m_dimensions.x;
	m_Viewport->Height = (float)m_dimensions.y;

	m_ScissorRect->right = (long)m_dimensions.x;
	m_ScissorRect->bottom = (long)m_dimensions.y;

	//TODO: Change Window Size
	//if (mWnd != NULL)
}

void D3D12Window::SetPosition(Int2 position)
{
}

void D3D12Window::SetPosition(int x, int y)
{
}

void D3D12Window::SetTitle(const char * title)
{
	this->m_title = title;
	if (m_Wnd != NULL)
		SetWindowText(m_Wnd, title);
}

bool D3D12Window::Create(int dimensionX, int dimensionY)
{
	SetDimensions(dimensionX, dimensionY);

	if (!InitializeWindow())
		return false;

	if (!InitializeCommandQueue())
		return false;

	if (!InitializeSwapChain())
		return false;

	if (!InitializeRenderTargets())
		return false;

	if (!InitializeDepthBuffer())
		return false;
	
	if (!InitializeRawInput())
		return false;

	HRESULT hr;
	for (int i = 0; i < NUM_SWAP_BUFFERS; i++)
	{
		hr = m_Renderer->GetDevice()->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_Fence[i]));
		if (FAILED(hr))
		{
			return false;
		}

		m_FenceValue[i] = 1;
		//Create an event handle to use for GPU synchronization.
		m_EventHandle[i] = CreateEvent(0, false, false, 0);
	}

	return true;
}

void D3D12Window::Show()
{
	if (m_Wnd != NULL)
		ShowWindow(m_Wnd, 1);
}

void D3D12Window::Hide()
{
}

void D3D12Window::HandleWindowEvents()
{
	/*Reset Previus State*/
	//m_input.Reset();

	MSG msg = { 0 };
	bool CheckMessage = true;
	Int2 mouseMovement;

	while (CheckMessage)
	{

		if (PeekMessage(&msg, m_Wnd, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);

#pragma region localEventCatching

			/*
				Update the local input handler for each window
			*/
			switch (msg.message)
			{

			case WM_KEYDOWN: {

				short key = static_cast<short>(msg.wParam);
				m_input.SetKeyDown(static_cast<char>(key), true);
			}
				break;
			case WM_KEYUP: {

				short key = static_cast<short>(msg.wParam);
				m_input.SetKeyDown(static_cast<char>(key), false);

				if (msg.lParam >> 30 & 1) {
					m_input.SetKeyPressed(static_cast<char>(key), true);
				}
			}
				break;
			case WM_INPUT:
			{
				UINT dwSize;
				GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

				if (dwSize < 50) {
					GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, (LPVOID)g_rawInputBuffer, &dwSize, sizeof(RAWINPUTHEADER));

					RAWINPUT* rawInput = (RAWINPUT*)g_rawInputBuffer;
					tagRAWMOUSE;
					switch (rawInput->header.dwType)
					{
					case RIM_TYPEMOUSE:
						tagRAWMOUSE m = rawInput->data.mouse;
						if (m.usFlags == 0) {
							mouseMovement.x = static_cast<int>(m.lLastX);
							mouseMovement.y = static_cast<int>(m.lLastY);
						}
						else {
							mouseMovement.x = 0;
							mouseMovement.y = 0;
						}
						break;
					case RIM_TYPEKEYBOARD:
						break;
					default:
						break;
					}
				}
					

				
			}
				break;

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
			default:
				break;
			}
			//std::cout << title << ":	" << std::dec << msg.message << " " << std::hex << msg.message << std::endl;
#pragma endregion

		}
		else {
			CheckMessage = false;
		}
	}
	m_input.SetMouseMovement(mouseMovement);
}

bool D3D12Window::WindowClosed()
{
	return quit;
}

void D3D12Window::ClearRenderTarget(ID3D12GraphicsCommandList3*	commandList)
{
	//Get the handle for the current render target used as back buffer.

	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	cdh.ptr += m_RenderTargetDescriptorSize * m_SwapChain4->GetCurrentBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE cdhds = m_DepthStencilHeap->GetCPUDescriptorHandleForHeapStart();

	float clearColor[] = { 0.2f, 0.2f, 0.2f, 1.0f };
	commandList->ClearRenderTargetView(cdh, clearColor, 0, nullptr);
	commandList->ClearDepthStencilView(cdhds, D3D12_CLEAR_FLAG_DEPTH /*| D3D12_CLEAR_FLAG_STENCIL*/, 1.0f, 0, 0, nullptr);
}

void D3D12Window::SetRenderTarget(ID3D12GraphicsCommandList3*	commandList)
{
	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	cdh.ptr += m_RenderTargetDescriptorSize * m_SwapChain4->GetCurrentBackBufferIndex();

	D3D12_CPU_DESCRIPTOR_HANDLE cdhds = m_DepthStencilHeap->GetCPUDescriptorHandleForHeapStart();
	commandList->OMSetRenderTargets(1, &cdh, true, &cdhds);
}

HWND D3D12Window::GetHWND()
{
	return m_Wnd;
}

ID3D12CommandQueue * D3D12Window::GetCommandQueue()
{
	return m_CommandQueue;
}

IDXGISwapChain4 * D3D12Window::GetSwapChain()
{
	return m_SwapChain4;
}

D3D12_VIEWPORT * D3D12Window::GetViewport()
{
	return m_Viewport;
}

D3D12_RECT * D3D12Window::GetScissorRect()
{
	return m_ScissorRect;
}

ID3D12Resource1 * D3D12Window::GetCurrentRenderTargetResource()
{
	return m_RenderTargets[m_SwapChain4->GetCurrentBackBufferIndex()];
}

UINT D3D12Window::GetCurrentBackBufferIndex() const
{
	return m_SwapChain4->GetCurrentBackBufferIndex();
}

bool D3D12Window::InitializeWindow()
{
	if (m_Wnd != NULL)
		return false;

	if (m_dimensions.x == 0 || m_dimensions.y == 0)
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


	RECT rc = { 0, 0, m_dimensions.x, m_dimensions.y };
	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, false);

	m_Wnd = CreateWindowEx(
		WS_EX_OVERLAPPEDWINDOW,
		"D3D12 Works!",
		m_title,
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
	hr = m_Renderer->GetDevice()->CreateCommandQueue(&cqd, IID_PPV_ARGS(&m_CommandQueue));
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
		m_CommandQueue,
		m_Wnd,
		&scDesc,
		nullptr,
		nullptr,
		&swapChain1);
	if (SUCCEEDED(hr))
	{
		hr = swapChain1->QueryInterface(IID_PPV_ARGS(&m_SwapChain4));
		if (SUCCEEDED(hr))
		{
			m_SwapChain4->Release();
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

	HRESULT hr = m_Renderer->GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_RenderTargetsHeap));
	if (FAILED(hr))
	{
		return false;
	}
	m_RenderTargetsHeap->SetName(L"RT DescHeap");


	//Create resources for the render targets.
	m_RenderTargetDescriptorSize = m_Renderer->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	//mCBV_SRV_UAV_DescriptorSize = mDevice5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
	UINT SRV_SIZE = m_Renderer->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	//One RTV for each frame.
	for (UINT n = 0; n < NUM_SWAP_BUFFERS; n++)
	{
		hr = m_SwapChain4->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n]));
		if (FAILED(hr))
		{
			return false;
		}
		m_RenderTargets[n]->SetName(L"RT");

		m_Renderer->GetDevice()->CreateRenderTargetView(m_RenderTargets[n], nullptr, cdh);
		cdh.ptr += m_RenderTargetDescriptorSize;

		//D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		//srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		//srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		//srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		//srvDesc.Texture2D.MipLevels = 1;

		//D3D12_CPU_DESCRIPTOR_HANDLE cdh2 = m_Renderer->GetDescriptorHeap()->GetCPUDescriptorHandleForHeapStart();
		//cdh2.ptr += (m_Renderer->NUM_DESCRIPTORS_IN_HEAP - 3 * NUM_SWAP_BUFFERS + 3 * n + 2) * SRV_SIZE;
		//
		//std::cout << "RT SRV: " << cdh2.ptr << std::endl;

		//m_Renderer->GetDevice()->CreateShaderResourceView(m_RenderTargets[n], &srvDesc, cdh2);
	}

	return true;
}

bool D3D12Window::InitializeDepthBuffer()
{

	D3D12_HEAP_PROPERTIES heapProperties = {};
	heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
	heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
	heapProperties.CreationNodeMask = 1; //used when multi-gpu
	heapProperties.VisibleNodeMask = 1; //used when multi-gpu
	heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

	D3D12_RESOURCE_DESC resourceDesc = {};
	resourceDesc.Format = DXGI_FORMAT_D32_FLOAT;
	resourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	resourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	resourceDesc.Width =  m_dimensions.x; //Use the dimensions of the window
	resourceDesc.Height = m_dimensions.y; //Use the dimensions of the window
	resourceDesc.DepthOrArraySize = 1;
	resourceDesc.MipLevels = 0;
	resourceDesc.SampleDesc.Count = 1;
	resourceDesc.SampleDesc.Quality = 0;
	resourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;

	D3D12_DEPTH_STENCIL_DESC dsd = {};
	dsd.DepthEnable = true;

	D3D12_CLEAR_VALUE cv = {};
	cv.Format = DXGI_FORMAT_D32_FLOAT;
	cv.DepthStencil.Depth = 1.0f;
	cv.DepthStencil.Stencil = 0;

	HRESULT hr = m_Renderer->GetDevice()->CreateCommittedResource(
		&heapProperties,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&cv,
		IID_PPV_ARGS(&m_DepthStencil)
	);
	if (FAILED(hr))
	{
		return false;
	}

	D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
	dhd.NumDescriptors = 1;
	dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	hr = m_Renderer->GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_DepthStencilHeap));
	if (FAILED(hr))
	{
		return false;
	}

	D3D12_DEPTH_STENCIL_VIEW_DESC dsvd = { };
	dsvd.Format = DXGI_FORMAT_D32_FLOAT;
	dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvd.Flags = D3D12_DSV_FLAG_NONE;
	dsvd.Texture2D.MipSlice = 0;

	m_Renderer->GetDevice()->CreateDepthStencilView(m_DepthStencil, &dsvd, m_DepthStencilHeap->GetCPUDescriptorHandleForHeapStart());

	return true;
}

bool D3D12Window::InitializeRawInput()
{
	m_rawMouseDevice.usUsagePage = 0x01;
	m_rawMouseDevice.usUsage = 0x02;
	m_rawMouseDevice.dwFlags = 0x0;
	m_rawMouseDevice.hwndTarget = m_Wnd;
	
	if (!RegisterRawInputDevices(&m_rawMouseDevice, 1U, sizeof(RAWINPUTDEVICE)))
	{
		return false;
	}
	
	return true;
}

void D3D12Window::WaitForGPU()
{
	static int frames = 0;
	frames++;

#ifdef SINGLE_FENCE
	const int nextFrame = m_SwapChain4->GetCurrentBackBufferIndex();
	const int previousFrame = (nextFrame - 1) % NUM_SWAP_BUFFERS;

	//Tell last frame to signal when done
	const UINT64 fence = m_FenceValue[0];
	m_CommandQueue->Signal(m_Fence[0], fence);
	m_FenceValue[0]++;

	//Wait until command queue is done.
	if (m_Fence[0]->GetCompletedValue() < m_FenceValue[0] - 1)
	{
		numWaits++;
		m_Fence[0]->SetEventOnCompletion(m_FenceValue[0] - 1, m_EventHandle[0]);
		WaitForSingleObject(m_EventHandle[0], INFINITE);
	}
#endif

	const int nextFrame = m_SwapChain4->GetCurrentBackBufferIndex();
	const int previousFrame = (nextFrame - 1) % NUM_SWAP_BUFFERS;
	
	// Tell last frame to signal when done
	const UINT64 fence = m_FenceValue[previousFrame];
	m_CommandQueue->Signal(m_Fence[previousFrame], fence);
	m_FenceValue[previousFrame]++;

	WaitForGPU(nextFrame);

	if (frames > 100) {
		std::string s = std::to_string(m_numWaits);
		s += "\n";
		OutputDebugString(s.c_str());
		frames = 0;
	}
}

void D3D12Window::WaitForGPU(int index)
{
	//Wait until command queue is done.
	if (m_Fence[index]->GetCompletedValue() < m_FenceValue[index] - 1)
	{
		m_numWaits++;
		m_Fence[index]->SetEventOnCompletion(m_FenceValue[index] - 1, m_EventHandle[index]);
		WaitForSingleObject(m_EventHandle[index], INFINITE);
	}
}
