#pragma once
#include <d3d12.h>

class FullScreenPass
{
public:
	FullScreenPass();
	~FullScreenPass();

	bool Initialize();
	void Record(ID3D12GraphicsCommandList3* list);
private:
	bool InitializeShaders();
	bool InitializePSO();
	bool InitializeRootSignature();

	ID3DBlob* vs_blob;
	ID3DBlob* ps_blob;
};

