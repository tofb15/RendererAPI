#include "RenderState.hpp"

RenderState::~RenderState()
{
}

void RenderState::SetWireframe(const bool wf)
{
	m_wireframe = wf;
}

void RenderState::SetFaceCulling(const FaceCulling fc)
{
	m_faceCulling = fc;
}

bool RenderState::GetWireframe() const
{
	return m_wireframe;
}

RenderState::FaceCulling RenderState::GetFaceCulling() const
{
	return m_faceCulling;
}

RenderState::RenderState()
{
	m_wireframe = false;
	m_faceCulling = FaceCulling::NONE;
}
