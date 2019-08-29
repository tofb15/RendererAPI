#pragma once

#include "../Technique.hpp"

struct ShaderProgram;
class D3D12RenderState;
class D3D12Renderer;
class D3D12ShaderManager;
struct ID3D12PipelineState;

/*
	Describes how a mesh should be rendered.
	Contains a collection of shader data(Material) and specific render states(RenderState)
*/
class D3D12Technique : public Technique{
public:
	D3D12Technique(D3D12Renderer* renderer, unsigned short id);
	virtual bool Initialize(D3D12RenderState*, ShaderProgram* sp, D3D12ShaderManager* sm);

	virtual ~D3D12Technique();

	// Inherited via Technique
	virtual bool Enable() override;
	unsigned short GetID() const;

	ID3D12PipelineState* GetPipelineState();

	bool GetAllowTransparency();

private:
	unsigned short m_id;
	bool m_allowTransparency = false;
	D3D12Renderer* m_renderer;
	ID3D12PipelineState* m_pipelineState = nullptr;

};