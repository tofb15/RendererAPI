#pragma once
#include <d3d12.h>
#include "GlobalSettings.hpp"

class D3D12Renderer;
class D3D12Camera;

class ParticleSystem
{
public:
	ParticleSystem();
	~ParticleSystem();
	bool Initialize(D3D12Renderer* renderer);

	void Update(float dt, int bufferIndex);
	void Render(ID3D12GraphicsCommandList3* list, int bufferIndex, D3D12Camera* camera);

private:

	enum Shader_Type {
		VS,
		GS,
		PS,
		CS
	};

	const UINT NUM_PARTICLES = 1000000;

	bool CompileShader(ID3DBlob** shaderBlob, const char* name, Shader_Type type);

	bool InitializeRootSignature_Render();
	bool InitializePSO_Render();

	bool InitializeShaders();
	bool InitializeRootSignature_CS();
	bool InitializePSO_CS();
	bool InitializeResources();
	bool InitializeCommandInterfaces();


	D3D12Renderer* m_renderer;

	ID3DBlob* m_cs_blob;
	ID3D12RootSignature* m_rootSignature_CS = nullptr;
	ID3D12PipelineState* m_pipelineState_CS = nullptr;
	ID3D12Resource* m_UA_Resource[NUM_SWAP_BUFFERS] = { nullptr };
	ID3D12Resource* m_UA_Resource_test;

	ID3DBlob* m_vs_blob;
	ID3DBlob* m_gs_blob;
	ID3DBlob* m_ps_blob;
	ID3D12RootSignature* m_rootSignature_render = nullptr;
	ID3D12PipelineState* m_pipelineState_render = nullptr;

	ID3D12GraphicsCommandList3* m_commandList[NUM_SWAP_BUFFERS] = { nullptr };
	ID3D12CommandAllocator* m_commandAllocator[NUM_SWAP_BUFFERS] = { nullptr };
	ID3D12CommandQueue* m_commandQueue = nullptr;

	UINT m_srv_cbv_uav_size;

	ID3D12Fence1*				m_Fence = nullptr;
	HANDLE						m_EventHandle = nullptr;
	UINT64						m_FenceValue = 0;
};
