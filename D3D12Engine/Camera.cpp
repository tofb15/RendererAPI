#include "stdafx.h"

#include "Camera.hpp"

Camera::~Camera()
{
}

void Camera::SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane)
{
}

Float3 Camera::GetPosition() const
{
	return m_position;
}

Float3 Camera::GetTarget() const
{
	return m_target;
}

Float3 Camera::GetTargetDirection() const
{
	return (m_target - m_position).normalized();
}

Float3 Camera::GetRight() const
{
	return GetTargetDirection().crossLH({ 0.0f,1.0f,0.0f });
}

bool Camera::HasViewChanged() const
{
	return false;
}

Camera::Camera()
{
}
