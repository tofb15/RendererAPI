#include "stdafx.h"

#include "RenderState.hpp"
namespace FusionReactor {

	RenderState::~RenderState() {
	}

	void RenderState::SetWireframe(const bool wf) {
		m_wireframe = wf;
	}

	void RenderState::SetOpaque(const bool opaque) {
		m_opaque = opaque;
	}

	void RenderState::SetFaceCulling(const FaceCulling fc) {
		m_faceCulling = fc;
	}

	void RenderState::SetUsingDepthBuffer(const bool depthBuffer) {
		m_useDepthBuffer = depthBuffer;
	}

	bool RenderState::GetWireframe() const {
		return m_wireframe;
	}

	bool RenderState::IsOpaque() const {
		return m_opaque;
	}

	RenderState::FaceCulling RenderState::GetFaceCulling() const {
		return m_faceCulling;
	}

	bool RenderState::GetIsUsingDepthBuffer() const {
		return m_useDepthBuffer;
	}

	RenderState::RenderState() {
		m_wireframe = false;
		m_opaque = true;
		m_faceCulling = FaceCulling::NONE;
		m_useDepthBuffer = true;
	}
}