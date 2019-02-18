#pragma once

#include "../Technique.hpp"

class D3D12Material;
class D3D12RenderState;

/*
	Describes how a mesh should be rendered.
	Contains a collection of shader data(Material) and specific render states(RenderState)
*/
class D3D12Technique : public Technique{
public:
	D3D12Technique(D3D12Material*, D3D12RenderState*);
private:

};