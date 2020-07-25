#include "stdafx.h"
#include "Math.hpp"

void MyRay::SetDirection(const Float3& direction) {
	m_direction = direction.normalized();
}

void MyRay::SetOrigin(const Float3& origin) {
	m_origin = origin;
}

Float3 MyRay::GetDirection() const {
	return m_direction;
}

Float3 MyRay::GetOrigin() const {
	return m_origin;
}

Float3 MyRay::ProjectPointToRay(const Float3& point) const {
	return m_direction * m_direction.dot(point - m_origin) + m_origin;
}
