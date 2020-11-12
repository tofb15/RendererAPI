#include "stdafx.h"

#include "Camera.hpp"

namespace FusionReactor {

	Camera::~Camera() {
	}

	void Camera::SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane) {
	}

	void Camera::Clone(Camera* other) const {
		if (this == other) {
			return;
		}

		other->m_position = m_position;
		other->m_target = m_target;
		other->m_fov = m_fov;
	}

	Float3 Camera::GetPosition() const {
		return m_position;
	}

	Float3 Camera::GetTarget() const {
		return m_target;
	}

	Float3 Camera::GetTargetDirection() const {
		return (m_target - m_position).normalized();
	}

	Float3 Camera::GetRight() const {
		return GetTargetDirection().crossLH({ 0.0f,1.0f,0.0f });
	}

	float Camera::GetFOV() const {
		return m_fov;
	}

	bool Camera::HasViewChanged() const {
		return false;
	}

	Camera::Camera() {
	}
}