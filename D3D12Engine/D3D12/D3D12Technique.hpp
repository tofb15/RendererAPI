#pragma once

#include "../Technique.hpp"

struct ShaderProgram;
class D3D12RenderState;
class D3D12API;
class D3D12ShaderManager;
struct ID3D12PipelineState;
struct ID3D12RootSignature;

/*
	Describes how a mesh should be rendered.
	Contains a collection of shader data(Material) and specific render states(RenderState)
*/
class D3D12Technique : public Technique{
public:
	D3D12Technique(D3D12API* d3d12, unsigned short id);
	virtual bool Initialize(D3D12RenderState*, ShaderProgram* sp, D3D12ShaderManager* sm);
	bool InitializeRootSignature();

	virtual ~D3D12Technique();

	// Inherited via Technique
	virtual bool Enable() override;
	unsigned short GetID() const;

	ID3D12PipelineState* GetPipelineState();
	ID3D12RootSignature* GetRootSignature() const;

private:
	unsigned short m_id;
	D3D12API* m_d3d12;
	ID3D12PipelineState* m_pipelineState = nullptr;
	ID3D12RootSignature* m_rootSignature = nullptr;

};