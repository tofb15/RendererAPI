#pragma once
#include "D3D12Renderer.h"

constexpr UINT NUM_ACCELERATION_STRUCTURES = 2;

class DXRBase;
class D3D12RaytracerRenderer : public D3D12Renderer
{
public:
	D3D12RaytracerRenderer(D3D12API* d3d12);
	~D3D12RaytracerRenderer();

	// Inherited via D3D12Renderer
	virtual bool Initialize() override;


private:
	// Inherited via D3D12Renderer
	virtual void Submit(SubmissionItem item, Camera* camera = nullptr, unsigned char layer = 0) override;
	virtual void ClearSubmissions() override;
	virtual void Frame(Window* window, Camera* camera) override;
	virtual void Present(Window* window) override;
	virtual void ClearFrame() override;

private:
	std::vector<SubmissionItem> m_OpaqueItems;
	std::vector<SubmissionItem> m_NonOpaqueItems;
	DXRBase* m_dxrBase;
};