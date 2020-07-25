#include "stdafx.h"
#include "D3D12Camera.hpp"

D3D12Camera::D3D12Camera() {
}

D3D12Camera::~D3D12Camera() {
}

void D3D12Camera::SetPosition(const Float3& position) {
	m_position = position;
	m_vp_needsUpdate = true;
	m_vp_inv_needsUpdate = true;
}

void D3D12Camera::Move(const Float3& position) {
	Float3 dir = GetTargetDirection();
	SetPosition(m_position + position);
	SetTarget(dir + m_position);
}

void D3D12Camera::Rotate(const Float3& axis, float angle) {
	Float3 dir = GetTargetDirection();

	DirectX::XMVECTOR xv = DirectX::XMVector3Transform(
		{ dir.x, dir.y, dir.z, 0.0f },
		DirectX::XMMatrixRotationAxis({ axis.x, axis.y, axis.z }, angle)
	);

	Float3 newDir = { xv.m128_f32[0], xv.m128_f32[1], xv.m128_f32[2] };

	if (std::fabsf(newDir.dot({ 0,1,0 })) > 0.98f) {
		newDir = dir;
	}

	SetTarget(m_position + newDir);
}

void D3D12Camera::SetTarget(const Float3& target) {
	m_target = target;
	m_vp_needsUpdate = true;
	m_vp_inv_needsUpdate = true;
}

void D3D12Camera::SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane) {
	m_fov = fov;
	DirectX::XMStoreFloat4x4(&mPerspectiveMatrix, DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane));
	m_vp_needsUpdate = true;
	m_vp_inv_needsUpdate = true;
}

void D3D12Camera::SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane) {
	DirectX::XMStoreFloat4x4(&mPerspectiveMatrix, DirectX::XMMatrixOrthographicLH(width, height, nearPlane, farPlane));
	m_vp_needsUpdate = true;
	m_vp_inv_needsUpdate = true;
}

MyRay D3D12Camera::ScreenCoordToRay(const Int2& screenCoord) {
	MyRay ray;
	if (m_vp_inv_needsUpdate) {
		GetViewPerspective_ref();
		DirectX::XMMATRIX inv_mat = DirectX::XMLoadFloat4x4(&mViewMatrix);
		inv_mat = DirectX::XMMatrixInverse(nullptr, inv_mat);
		DirectX::XMStoreFloat4x4(&m_ViewPerspectiveMatrix_inverse, inv_mat);
		m_vp_inv_needsUpdate = false;
	}

	ray.SetOrigin(m_position);

	Float2 point;
	point.x = ((2.0 * screenCoord.x / 1920.0) - 1.0) / mPerspectiveMatrix._11;
	point.y = -1 * ((2.0 * screenCoord.y / 1080.0) - 1.0) / mPerspectiveMatrix._22;

	Float3 direction;
	direction.x = (point.x * m_ViewPerspectiveMatrix_inverse._11) + (point.y * m_ViewPerspectiveMatrix_inverse._21) + m_ViewPerspectiveMatrix_inverse._31;
	direction.y = (point.x * m_ViewPerspectiveMatrix_inverse._12) + (point.y * m_ViewPerspectiveMatrix_inverse._22) + m_ViewPerspectiveMatrix_inverse._32;
	direction.z = (point.x * m_ViewPerspectiveMatrix_inverse._13) + (point.y * m_ViewPerspectiveMatrix_inverse._23) + m_ViewPerspectiveMatrix_inverse._33;
	ray.SetDirection(direction);

	return ray;
}

DirectX::XMFLOAT4X4 D3D12Camera::GetViewPerspective() const {
	return GetViewPerspective_ref();
}

const DirectX::XMFLOAT4X4& D3D12Camera::GetViewPerspective_ref() const {
	if (m_vp_needsUpdate) {
		DirectX::XMStoreFloat4x4(&mViewMatrix, DirectX::XMMatrixLookAtLH({ m_position.x, m_position.y, m_position.z }, { m_target.x, m_target.y, m_target.z }, { 0,1,0 }));
		DirectX::XMStoreFloat4x4(&mViewPerspectiveMatrix, DirectX::XMLoadFloat4x4(&mViewMatrix) * DirectX::XMLoadFloat4x4(&mPerspectiveMatrix));

		m_frustum.CreateFrustum(mViewPerspectiveMatrix, m_position, 90, 1000);

		m_vp_needsUpdate = false;
	}
	return mViewPerspectiveMatrix;
}

const Frustum& D3D12Camera::GetFrustum() const {
	if (m_vp_needsUpdate) {
		DirectX::XMStoreFloat4x4(&mViewMatrix, DirectX::XMMatrixLookAtLH({ m_position.x, m_position.y, m_position.z }, { m_target.x, m_target.y, m_target.z }, { 0,1,0 }));
		DirectX::XMStoreFloat4x4(&mViewPerspectiveMatrix, DirectX::XMLoadFloat4x4(&mViewMatrix) * DirectX::XMLoadFloat4x4(&mPerspectiveMatrix));

		m_frustum.CreateFrustum(mViewPerspectiveMatrix, m_position, 90, 1000);

		m_vp_needsUpdate = false;
	}

	return m_frustum;
}

Frustum::Frustum() {
}

Frustum::~Frustum() {
}

void Frustum::CreateFrustum(const DirectX::XMFLOAT4X4& _viewProj, const Float3& position, float FOV, float farPlane) {
	// When multiplied together, the view-projection-matrix contains useful information
	// The column vectors in each of the four columns are combined to extract a plane's normal and distance from origin
	// To make the normals point outwards from the frustum instead of inwards, the values are negated

	float divLength;

	// Top clipping plane
	m_planes[0].normal.x = -(_viewProj._14 - _viewProj._12);
	m_planes[0].normal.y = -(_viewProj._24 - _viewProj._22);
	m_planes[0].normal.z = -(_viewProj._34 - _viewProj._32);
	m_planes[0].d = -(_viewProj._44 - _viewProj._42);

	// Bottom clipping plane
	m_planes[1].normal.x = -(_viewProj._14 + _viewProj._12);
	m_planes[1].normal.y = -(_viewProj._24 + _viewProj._22);
	m_planes[1].normal.z = -(_viewProj._34 + _viewProj._32);
	m_planes[1].d = -(_viewProj._44 + _viewProj._42);

	// Left clipping plane
	m_planes[2].normal.x = -(_viewProj._14 + _viewProj._11);
	m_planes[2].normal.y = -(_viewProj._24 + _viewProj._21);
	m_planes[2].normal.z = -(_viewProj._34 + _viewProj._31);
	m_planes[2].d = -(_viewProj._44 + _viewProj._41);

	// Right clipping plane
	m_planes[3].normal.x = -(_viewProj._14 - _viewProj._11);
	m_planes[3].normal.y = -(_viewProj._24 - _viewProj._21);
	m_planes[3].normal.z = -(_viewProj._34 - _viewProj._31);
	m_planes[3].d = -(_viewProj._44 - _viewProj._41);

	// Near clipping plane
	m_planes[4].normal.x = -(_viewProj._14 + _viewProj._13);
	m_planes[4].normal.y = -(_viewProj._24 + _viewProj._23);
	m_planes[4].normal.z = -(_viewProj._34 + _viewProj._33);
	m_planes[4].d = -(_viewProj._44 + _viewProj._43);

	// Far clipping plane
	m_planes[5].normal.x = -(_viewProj._14 - _viewProj._13);
	m_planes[5].normal.y = -(_viewProj._24 - _viewProj._23);
	m_planes[5].normal.z = -(_viewProj._34 - _viewProj._33);
	m_planes[5].d = -(_viewProj._44 - _viewProj._43);

	// Normalize plane
	for (int i = 0; i < 6; i++) {
		//n = m_planes[i].normal;

		divLength = 1.0f / m_planes[i].normal.length();

		m_planes[i].normal.x *= divLength;
		m_planes[i].normal.y *= divLength;
		m_planes[i].normal.z *= divLength;
		m_planes[i].d *= divLength;
	}
}

bool Frustum::CheckPoint(float x, float y, float z) const {
	return false;
}

bool Frustum::CheckPoint(DirectX::XMFLOAT3 p) const {
	return false;
}

bool Frustum::CheckAgainstFrustum(const Sphere& sphere) const {
	for (int i = 0; i < 6; i++) {
		Plane plane = m_planes[i];
		Float3 p = sphere.center - plane.normal * plane.d;
		float p2 = DistanceToPlane(plane, sphere.center);

		if (p2 < 0 || p2 * p2 < sphere.radius) {
			//Inside
		} else {
			//outside
			return false;
		}
	}

	return true;
}

float Frustum::DistanceToPlane(const Plane& p, const Float3& point) const {
	// Return difference bewtween orthogonal projection and shortest distance to the plane (ax + by + cz = d)
	return p.normal.dot(point) + p.d;
}
