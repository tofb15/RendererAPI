#include "Camera.hpp"

Camera::~Camera()
{
}

void Camera::SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane)
{
}

Float3 Camera::GetPosition() const
{
	return Float3();
}

Float3 Camera::GetTarget() const
{
	return Float3();
}

Float3 Camera::GetTargetDirection() const
{
	return Float3();
}

bool Camera::HasViewChanged() const
{
	return false;
}

Camera::Camera()
{
}
