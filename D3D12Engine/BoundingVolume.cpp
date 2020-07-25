#include "stdafx.h"
#include "BoundingVolume.hpp"

BoundingSphere::BoundingSphere(Float3 origin, float radius) : m_origin(origin), m_radius(radius) {
}

BoundingSphere::~BoundingSphere() {
}

void BoundingSphere::SetRadius(float radius) {
	m_radius = radius;
}

void BoundingSphere::SetOrigin(const Float3& origin) {
	m_origin = origin;
}

float BoundingSphere::GetRadius() const {
	return m_radius;
}

Float3 BoundingSphere::GetOrigin() const {
	return m_origin;
}

bool BoundingSphere::RayIntersection_Fast(const MyRay& ray) const {
	Float3 p = ray.ProjectPointToRay(m_origin);
	Float3 delta = p - m_origin;

	float dist1 = delta.length();
	if (dist1 > m_radius) {
		return false;
	} else {
		//We dont need the t values
		return true;
	}
	return false;
}