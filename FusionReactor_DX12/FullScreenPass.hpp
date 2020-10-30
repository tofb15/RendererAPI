#pragma once
#include <d3d12.h>

class D3D12API;
class D3D12Window;

class FullScreenPass
{
public:
	FullScreenPass();
	~FullScreenPass();

	bool Initialize(D3D12API* renderer);
	void Record(ID3D12GraphicsCommandList3* list, D3D12Window* window);

private:
	bool InitializeShaders();
	bool InitializePSO();
	bool InitializeRootSignature();

	ID3DBlob* vs_blob;
	ID3DBlob* ps_blob;
	D3D12API* m_d3d12;

	ID3D12RootSignature* m_rootSignature = nullptr;
	ID3D12PipelineState* m_pipelineState = nullptr;

	UINT m_srv_cbv_uav_size;
};

