#pragma once
#include <d3d12.h>
#include "GlobalSettings.hpp"

class D3D12Renderer;
class D3D12Window;

class FXAAPass
{
public:
	FXAAPass();
	~FXAAPass();

	bool Initialize(D3D12Renderer* renderer);
	void ApplyFXAA(D3D12Window* window);

private:
	bool InitializeShaders();
	bool InitializeRootSignature();
	bool InitializePSO();
	bool InitializeResources();
	bool InitializeCommandInterfaces();

	ID3DBlob* m_cs_blob;
	D3D12Renderer* m_renderer;
	ID3D12RootSignature* m_rootSignature = nullptr;
	ID3D12PipelineState* m_pipelineState = nullptr;
	ID3D12Resource* m_UA_Resource[NUM_SWAP_BUFFERS] = {nullptr};
	ID3D12Resource* m_UA_Resource_test;

	ID3D12GraphicsCommandList3* m_commandList[NUM_SWAP_BUFFERS] = { nullptr };
	ID3D12CommandAllocator* m_commandAllocator[NUM_SWAP_BUFFERS] = { nullptr };
	ID3D12CommandQueue* m_commandQueue = nullptr;

	ID3D12Fence1*				m_Fence = nullptr ;
	HANDLE						m_EventHandle = nullptr ;
	UINT64						m_FenceValue = 0;

	UINT m_srv_cbv_uav_size;
};

