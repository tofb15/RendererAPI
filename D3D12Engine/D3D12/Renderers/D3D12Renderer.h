#pragma once
#include "..\..\Renderer.hpp"
#include "..\D3D12_FDecl.h"
#include "..\D3D12API.hpp"

class D3D12Renderer : public Renderer
{
public:
	D3D12Renderer(D3D12API* d3d12);
	~D3D12Renderer();
protected:
	D3D12API* m_d3d12;

private:

};