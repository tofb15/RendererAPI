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
	D3D12Technique(D3D12Renderer* renderer);
	virtual bool Initialize(D3D12RenderState*, ShaderProgram* sp, D3D12ShaderManager* sm);

	virtual ~D3D12Technique();

	// Inherited via Technique
	virtual bool Enable() override;

private:
	D3D12Renderer* mRenderer;
	ID3D12PipelineState* mPipelineState = nullptr;

};