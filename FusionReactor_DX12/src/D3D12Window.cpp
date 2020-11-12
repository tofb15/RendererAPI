#include "stdafx.h"

#include "D3D12Window.hpp"
#include <string>
#include <iostream>

#include <DirectXMath.h>

#include "D3D12API.hpp"

#include "FusionReactor/src/External/IMGUI/imgui.h"
#include "FusionReactor/src/External/IMGUI/imgui_impl_win32.h"
#include "FusionReactor/src/External/IMGUI/imgui_impl_dx12.h"
#include "Internal/D3D12TextureLoader.hpp"
#include "D3D12Texture.hpp"

#pragma comment (lib, "DXGI.lib")
//#pragma comment (lib, "d3d12.lib")

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace FusionReactor {
	namespace FusionReactor_DX12 {
		static bool quit = false;

		BYTE g_rawInputBuffer[64];

		constexpr int WM_SIZE_CUSTOM = WM_USER + 1;

		LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam) {
			//std::cout << ":	" << std::dec << message << " " << std::hex << message << std::endl;


			/*
				Catch Global Winodw Events.
			*/
			switch (message) {
			case WM_SIZE:
			{
				PostMessageW(hWnd, WM_SIZE_CUSTOM, wParam, lParam);
			}
			break;
			case WM_QUIT:
			case WM_CLOSE:
			case WM_DESTROY:
				//PostQuitMessage(0);
				quit = true;
				return 0;
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

				if (lParam >> 30 & 1) {
					Window::GetGlobalWindowInputHandler().SetKeyPressed(static_cast<char>(key), true);
				}
			}
			break;
			}


			return DefWindowProc(hWnd, message, wParam, lParam);
		}

		D3D12Window::D3D12Window(D3D12API* d3d12) {
			m_Wnd = NULL;
			m_d3d12 = d3d12;
			m_Viewport = MY_NEW D3D12_VIEWPORT;
			m_ScissorRect = MY_NEW D3D12_RECT;

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

		D3D12Window::~D3D12Window() {
			m_d3d12->WaitForGPU_ALL();

			ImGui_ImplDX12_Shutdown();
			ImGui_ImplWin32_Shutdown();
			ImGui::DestroyContext();

			if (m_Viewport) {
				delete m_Viewport;
			}
			if (m_ScissorRect) {
				delete m_ScissorRect;
			}

			// Wait until each queue has finished executing before releasing resources
			int currentBackBuffer = m_SwapChain4->GetCurrentBackBufferIndex();

			for (int i = 0; i < NUM_SWAP_BUFFERS; i++) {
				m_RenderTargets[i]->Release();
			}
			if (m_RenderTargetsHeap) {
				m_RenderTargetsHeap->Release();
			}
			if (m_DepthStencil) {
				m_DepthStencil->Release();
			}
			if (m_DepthStencilHeap) {
				m_DepthStencilHeap->Release();
			}
			if (m_GUIDescriptHeap) {
				m_GUIDescriptHeap->Release();
			}
			if (m_SwapChain4) {
				m_SwapChain4->Release();
			}
		}

		void D3D12Window::SetDimensions(const Int2& dimensions) {
			SetDimensions(dimensions.x, dimensions.y);
		}

		void D3D12Window::SetDimensions(int w, int h) {
			m_dimensions.x = w;
			m_dimensions.y = h;

			m_Viewport->Width = (float)m_dimensions.x;
			m_Viewport->Height = (float)m_dimensions.y;

			m_ScissorRect->right = (long)m_dimensions.x;
			m_ScissorRect->bottom = (long)m_dimensions.y;

			//TODO: Change Window Size
			//if (mWnd != NULL)
		}

		void D3D12Window::SetPosition(const Int2& position) {
		}

		void D3D12Window::SetPosition(int x, int y) {
		}

		void D3D12Window::SetTitle(const char* title) {
			this->m_title = title;
			if (m_Wnd != NULL)
				SetWindowText(m_Wnd, title);
		}

		bool D3D12Window::Create(int dimensionX, int dimensionY) {
			SetDimensions(dimensionX, dimensionY);

			if (!InitializeWindow())
				return false;

			if (!InitializeSwapChain())
				return false;

			if (!InitializeRenderTargets())
				return false;

			if (!InitializeDepthBuffer())
				return false;

			if (!InitializeRawInput())
				return false;

			//=========IMGUI==============
			// Setup Dear ImGui context
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

			// Setup Dear ImGui style
			ImGui::StyleColorsDark();
			//ImGui::StyleColorsClassic();

			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = m_GuiDescriptorHeapSize;
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
			if (m_d3d12->GetDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_GUIDescriptHeap)) != S_OK)
				return false;

			// Setup Platform/Renderer bindings
			m_srv_descriptorSize = m_d3d12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			m_descriptorHeap_start.cdh = m_GUIDescriptHeap->GetCPUDescriptorHandleForHeapStart();
			m_descriptorHeap_start.gdh = m_GUIDescriptHeap->GetGPUDescriptorHandleForHeapStart();

			m_unreserved_handle_start = m_descriptorHeap_start;
			m_unreserved_handle_start += m_srv_descriptorSize * 1;
			m_unused_handle_start_this_frame = m_unreserved_handle_start;

			ImGui_ImplWin32_Init(m_Wnd);
			ImGui_ImplDX12_Init(m_d3d12->GetDevice(), NUM_SWAP_BUFFERS,
				DXGI_FORMAT_R8G8B8A8_UNORM, m_GUIDescriptHeap,
				m_GUIDescriptHeap->GetCPUDescriptorHandleForHeapStart(),
				m_GUIDescriptHeap->GetGPUDescriptorHandleForHeapStart());

			return true;
		}

		void D3D12Window::Show() {
			if (m_Wnd != NULL)
				ShowWindow(m_Wnd, 1);
		}

		void D3D12Window::Hide() {
		}

		void D3D12Window::HandleWindowEvents() {
			/*Reset Previus State*/
			m_input.Reset();

			MSG msg = { 0 };
			bool CheckMessage = true;
			Int2 mouseMovement(0, 0);
			int mouseWheelMovement = 0;

			while (CheckMessage) {

				if (PeekMessage(&msg, m_Wnd, 0, 0, PM_REMOVE)) {
					TranslateMessage(&msg);
					DispatchMessage(&msg);

#pragma region localEventCatching
					if (ImGui_ImplWin32_WndProcHandler(m_Wnd, msg.message, msg.wParam, msg.lParam))
						return;

					/*
						Update the local input handler for each window
					*/
					switch (msg.message) {
					case WM_SIZE_CUSTOM:
					{
						if (!m_firstResize) {
							ApplyResize();
						}
						m_firstResize = false;
					}
					break;
					case WM_KEYDOWN:
					{
						short key = static_cast<short>(msg.wParam);
						m_input.SetKeyDown(static_cast<char>(key), true);
					}
					break;
					case WM_KEYUP:
					{

						short key = static_cast<short>(msg.wParam);
						m_input.SetKeyDown(static_cast<char>(key), false);

						if (msg.lParam >> 30 & 1) {
							m_input.SetKeyPressed(static_cast<char>(key), true);
						}
					}
					break;
					case WM_INPUT:
					{
						UINT dwSize = 0;
						GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));

						if (dwSize < 50) {
							GetRawInputData((HRAWINPUT)msg.lParam, RID_INPUT, (LPVOID)g_rawInputBuffer, &dwSize, sizeof(RAWINPUTHEADER));

							RAWINPUT* rawInput = (RAWINPUT*)g_rawInputBuffer;
							tagRAWMOUSE;
							switch (rawInput->header.dwType) {
							case RIM_TYPEMOUSE:
								tagRAWMOUSE m = rawInput->data.mouse;
								if (m.usFlags == 0) {
									mouseMovement.x = static_cast<int>(m.lLastX);
									mouseMovement.y = static_cast<int>(m.lLastY);
								}

								switch (m.usButtonFlags) {
								case RI_MOUSE_LEFT_BUTTON_DOWN:
									m_input.SetMouseKeyDown(WindowInput::MOUSE_KEY_CODE_LEFT, true);
									break;
								case RI_MOUSE_LEFT_BUTTON_UP:
									m_input.SetMouseKeyDown(WindowInput::MOUSE_KEY_CODE_LEFT, false);
									break;
								case RI_MOUSE_MIDDLE_BUTTON_DOWN:
									m_input.SetMouseKeyDown(WindowInput::MOUSE_KEY_CODE_MIDDLE, true);
									break;
								case RI_MOUSE_MIDDLE_BUTTON_UP:
									m_input.SetMouseKeyDown(WindowInput::MOUSE_KEY_CODE_MIDDLE, false);
									break;
								case RI_MOUSE_RIGHT_BUTTON_DOWN:
									m_input.SetMouseKeyDown(WindowInput::MOUSE_KEY_CODE_RIGHT, true);
									break;
								case RI_MOUSE_RIGHT_BUTTON_UP:
									m_input.SetMouseKeyDown(WindowInput::MOUSE_KEY_CODE_RIGHT, false);
									break;
								case RI_MOUSE_WHEEL:
									mouseWheelMovement = (short)rawInput->data.mouse.usButtonData;
									break;
								default:
									break;
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

					default:
						break;
					}
					//std::cout << title << ":	" << std::dec << msg.message << " " << std::hex << msg.message << std::endl;
#pragma endregion

				} else {
					CheckMessage = false;
				}
			}
			m_input.SetMouseMovement(mouseMovement);
			m_input.SetMouseWheelMovement(mouseWheelMovement);
		}

		bool D3D12Window::WindowClosed() {
			return quit;
		}

		void D3D12Window::ClearRenderTarget(ID3D12GraphicsCommandList3* commandList) {
			//Get the handle for the current render target used as back buffer.

			D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
			cdh.ptr += (size_t)(m_RenderTargetDescriptorSize * m_SwapChain4->GetCurrentBackBufferIndex());

			D3D12_CPU_DESCRIPTOR_HANDLE cdhds = m_DepthStencilHeap->GetCPUDescriptorHandleForHeapStart();

			float clearColor[] = { 0.0f, 0.0f, 0.0f, 1.0f };
			commandList->ClearRenderTargetView(cdh, clearColor, 0, nullptr);
			commandList->ClearDepthStencilView(cdhds, D3D12_CLEAR_FLAG_DEPTH /*| D3D12_CLEAR_FLAG_STENCIL*/, 1.0f, 0, 0, nullptr);
		}

		void D3D12Window::SetRenderTarget(ID3D12GraphicsCommandList3* commandList) {
			D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
			cdh.ptr += (size_t)(m_RenderTargetDescriptorSize * m_SwapChain4->GetCurrentBackBufferIndex());

			D3D12_CPU_DESCRIPTOR_HANDLE cdhds = m_DepthStencilHeap->GetCPUDescriptorHandleForHeapStart();
			commandList->OMSetRenderTargets(1, &cdh, true, &cdhds);
		}

		HWND D3D12Window::GetHWND() {
			return m_Wnd;
		}

		IDXGISwapChain4* D3D12Window::GetSwapChain() {
			return m_SwapChain4;
		}

		D3D12_VIEWPORT* D3D12Window::GetViewport() {
			return m_Viewport;
		}

		D3D12_RECT* D3D12Window::GetScissorRect() {
			return m_ScissorRect;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE D3D12Window::GetCurrentRenderTargetGPUDescriptorHandle() {
			D3D12_GPU_DESCRIPTOR_HANDLE handle = m_RenderTargetsHeap->GetGPUDescriptorHandleForHeapStart();
			handle.ptr += m_RenderTargetDescriptorSize * m_SwapChain4->GetCurrentBackBufferIndex();
			return handle;
		}

		ID3D12Resource1* D3D12Window::GetCurrentRenderTargetResource() {
			return m_RenderTargets[m_SwapChain4->GetCurrentBackBufferIndex()];
		}

		ID3D12Resource1** D3D12Window::GetRenderTargetResources() {
			return m_RenderTargets;
		}

		UINT D3D12Window::GetCurrentBackBufferIndex() const {
			return m_SwapChain4->GetCurrentBackBufferIndex();
		}

		bool D3D12Window::InitializeWindow() {
			if (m_Wnd != NULL)
				return false;

			if (m_dimensions.x == 0 || m_dimensions.y == 0)
				return false;

			WNDCLASSEX wcex = { 0 };
			wcex.cbSize = sizeof(WNDCLASSEX);
			wcex.lpfnWndProc = WndProc;
			wcex.hInstance = nullptr;
			wcex.lpszClassName = "D3D12 Works!";
			if (!RegisterClassEx(&wcex)) {
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


		bool D3D12Window::InitializeSwapChain() {
			HRESULT hr;

			IDXGIFactory5* factory = nullptr;
			hr = CreateDXGIFactory(IID_PPV_ARGS(&factory));
			if (FAILED(hr)) {
				return false;
			}

			//Create swap chain.
			DXGI_SWAP_CHAIN_DESC1 scDesc = {};
			scDesc.Width = m_dimensions.x;
			scDesc.Height = m_dimensions.y;
			scDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			scDesc.Stereo = FALSE;
			scDesc.SampleDesc.Count = 1;
			scDesc.SampleDesc.Quality = 0;
			scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			scDesc.BufferCount = NUM_SWAP_BUFFERS;
			scDesc.Scaling = DXGI_SCALING_NONE;
			scDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
			scDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			scDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

			IDXGISwapChain1* swapChain1 = nullptr;

			hr = factory->CreateSwapChainForHwnd(
				m_d3d12->GetDirectCommandQueue(),
				m_Wnd,
				&scDesc,
				nullptr,
				nullptr,
				&swapChain1);
			if (SUCCEEDED(hr)) {
				hr = swapChain1->QueryInterface(IID_PPV_ARGS(&m_SwapChain4));
				if (SUCCEEDED(hr)) {
					m_SwapChain4->Release();
				} else {
					return false;
				}
			} else {
				return false;
			}

			factory->Release();
			return true;
		}

		bool D3D12Window::InitializeRenderTargets() {
			//Create descriptor heap for render target views.
			D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
			dhd.NumDescriptors = NUM_SWAP_BUFFERS;
			dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;

			HRESULT hr = m_d3d12->GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_RenderTargetsHeap));
			if (FAILED(hr)) {
				return false;
			}
			m_RenderTargetsHeap->SetName(L"RT DescHeap");

			//Create resources for the render targets.
			m_RenderTargetDescriptorSize = m_d3d12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

			//mCBV_SRV_UAV_DescriptorSize = mDevice5->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
			UINT SRV_SIZE = m_d3d12->GetDevice()->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			//One RTV for each frame.
			for (UINT n = 0; n < NUM_SWAP_BUFFERS; n++) {
				hr = m_SwapChain4->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n]));
				if (FAILED(hr)) {
					return false;
				}
				m_RenderTargets[n]->SetName(L"RT");

				m_d3d12->GetDevice()->CreateRenderTargetView(m_RenderTargets[n], nullptr, cdh);
				cdh.ptr += m_RenderTargetDescriptorSize;
			}

			return true;
		}

		bool D3D12Window::InitializeDepthBuffer() {

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
			resourceDesc.Width = m_dimensions.x; //Use the dimensions of the window
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

			HRESULT hr = m_d3d12->GetDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resourceDesc,
				D3D12_RESOURCE_STATE_DEPTH_WRITE,
				&cv,
				IID_PPV_ARGS(&m_DepthStencil)
			);
			if (FAILED(hr)) {
				return false;
			}

			D3D12_DESCRIPTOR_HEAP_DESC dhd = {};
			dhd.NumDescriptors = 1;
			dhd.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
			dhd.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

			hr = m_d3d12->GetDevice()->CreateDescriptorHeap(&dhd, IID_PPV_ARGS(&m_DepthStencilHeap));
			if (FAILED(hr)) {
				return false;
			}

			D3D12_DEPTH_STENCIL_VIEW_DESC dsvd = { };
			dsvd.Format = DXGI_FORMAT_D32_FLOAT;
			dsvd.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
			dsvd.Flags = D3D12_DSV_FLAG_NONE;
			dsvd.Texture2D.MipSlice = 0;

			m_d3d12->GetDevice()->CreateDepthStencilView(m_DepthStencil, &dsvd, m_DepthStencilHeap->GetCPUDescriptorHandleForHeapStart());

			return true;
		}

		bool D3D12Window::InitializeRawInput() {
			m_rawMouseDevice.usUsagePage = 0x01;
			m_rawMouseDevice.usUsage = 0x02;
			m_rawMouseDevice.dwFlags = 0x0;
			m_rawMouseDevice.hwndTarget = m_Wnd;

			if (!RegisterRawInputDevices(&m_rawMouseDevice, 1U, sizeof(RAWINPUTDEVICE))) {
				return false;
			}

			return true;
		}

		void D3D12Window::ApplyResize() {
			m_d3d12->WaitForGPU_ALL();

			for (UINT n = 0; n < NUM_SWAP_BUFFERS; n++) {
				m_RenderTargets[n]->Release();
			}

			//m_SwapChain4->SetFullscreenState(msg.wParam == SIZE_MAXIMIZED, NULL);
			HRESULT hr = m_SwapChain4->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
			if (FAILED(hr)) {
				int i = 0;
				//MessageBoxA(NULL, "Winodw Resizing Failed", "Error", 0);
			}

			D3D12_CPU_DESCRIPTOR_HANDLE cdh = m_RenderTargetsHeap->GetCPUDescriptorHandleForHeapStart();
			for (UINT n = 0; n < NUM_SWAP_BUFFERS; n++) {
				hr = m_SwapChain4->GetBuffer(n, IID_PPV_ARGS(&m_RenderTargets[n]));
				if (FAILED(hr)) {
					MessageBoxA(NULL, "Apply Resize Failed", "Error", 0);
					return;
				}
				m_RenderTargets[n]->SetName(L"RT");

				m_d3d12->GetDevice()->CreateRenderTargetView(m_RenderTargets[n], nullptr, cdh);
				cdh.ptr += m_RenderTargetDescriptorSize;
			}
		}

		void D3D12Window::BeginUIRendering() {
			m_unused_handle_start_this_frame = m_unreserved_handle_start;
			//m_unused_handle_start_this_frame += m_srv_descriptorSize * (m_GuiDescriptorHeapSize - 1);
		}

		void* D3D12Window::PrepareTextureForGuiRendering(Texture* texture, bool permanent) {
			D3D12_GPU_DESCRIPTOR_HANDLE target_GPU_Addr;

			//if (m_unused_handle_start_this_frame.gdh.ptr != m_unreserved_handle_start.gdh.ptr) {
			target_GPU_Addr = m_unused_handle_start_this_frame.gdh;
			//}

			auto current_CPU_Addr = m_d3d12->GetTextureLoader()->GetSpecificTextureCPUAdress(static_cast<D3D12Texture*>(texture));
			m_d3d12->GetDevice()->CopyDescriptorsSimple(1, m_unused_handle_start_this_frame.cdh, current_CPU_Addr, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

			m_unused_handle_start_this_frame += m_srv_descriptorSize;
			return (void*)target_GPU_Addr.ptr;
		}

		void D3D12Window::EndUIRendering() {

		}

		ID3D12DescriptorHeap* D3D12Window::GetGUIDescriptorHeap() {
			return m_GUIDescriptHeap;
		}
	}
}