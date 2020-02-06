#pragma once
#include "D3D12Renderer.h"

constexpr UINT NUM_ACCELERATION_STRUCTURES = 2;

class DXRBase;
class D3D12Window;
class D3D12RaytracerRenderer : public D3D12Renderer
{
public:
	D3D12RaytracerRenderer(D3D12API* d3d12);
	~D3D12RaytracerRenderer();

	// Inherited via D3D12Renderer
	virtual bool Initialize() override;


private:
	bool InitializeCommandInterfaces();
	bool InitializeOutputTextures(D3D12Window* window);
	void ResetCommandListAndAllocator(int index);

	// Inherited via D3D12Renderer
	virtual void Submit(SubmissionItem item, Camera* camera = nullptr, unsigned char layer = 0) override;
	virtual void ClearSubmissions() override;
	virtual void Frame(Window* window, Camera* camera) override;
	virtual void Present(Window* window, GUI* gui = nullptr) override;
	virtual void ClearFrame() override;
	virtual void Refresh(std::vector<std::wstring>* defines) override;
	virtual void SetLightSources(const std::vector<LightSource>& lights) override;

private:
	std::vector<SubmissionItem> m_OpaqueItems;
	std::vector<SubmissionItem> m_NonOpaqueItems;
	std::vector<LightSource> m_lights;

	DXRBase* m_dxrBase;

	ID3D12CommandAllocator* m_commandAllocators[NUM_GPU_BUFFERS] = { nullptr };
	ID3D12GraphicsCommandList4* m_commandLists[NUM_GPU_BUFFERS] = { nullptr };

	//RenderTargets
	ID3D12Resource* m_outputTextures[NUM_GPU_BUFFERS] = { nullptr };
	Int2 m_outputDim;

};