#include "stdafx.h"
#include "BoundingVolume.hpp"

void BoundingVolume::SetOrigin(const Float3& origin) {
	m_origin = origin;
}
Float3 BoundingVolume::GetOrigin() const {
	return m_origin;
}

BoundingVolumeType BoundingVolume::GetType() {
	return m_type;
}

BoundingSphere::BoundingSphere(Float3 origin, float radius) : BoundingVolume(origin) {
	m_size.x = radius;
	m_type = BoundingVolumeType::Sphere;
}

BoundingSphere::~BoundingSphere() {
}

bool BoundingSphere::RayIntersection_Fast(const MyRay& ray) const {
	Float3 p = ray.ProjectPointToRay(m_origin);
	Float3 delta = p - m_origin;

	float dist1 = delta.length();
	if (dist1 > GetRadius()) {
		return false;
	} else {
		//We dont need the t values
		return true;
	}
	return false;
}

BoundingBox::BoundingBox(Float3 origin, Float3 size) : BoundingVolume(origin) {
	m_size = size;
	m_type = BoundingVolumeType::Box;
}

BoundingBox::~BoundingBox() {
}
