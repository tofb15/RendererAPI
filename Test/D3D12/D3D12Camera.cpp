#include "D3D12Camera.hpp"

D3D12Camera::D3D12Camera()
{
}

D3D12Camera::~D3D12Camera()
{
}

void D3D12Camera::SetPosition(Float3 position)
{
	m_position = position;
	mHasChanged = true;
}

void D3D12Camera::Move(Float3 position)
{
	Float3 dir = GetTargetDirection();
	SetPosition(m_position + position);
	SetTarget(dir + m_position);
}

void D3D12Camera::Rotate(Float3 axis, float angle)
{
	Float3 dir = GetTargetDirection();
	
	DirectX::XMVECTOR xv = DirectX::XMVector3Transform(
		{ dir.x, dir.y, dir.z, 0.0f },
		DirectX::XMMatrixRotationAxis({ axis.x, axis.y, axis.z }, angle)
	);

	Float3 newDir = { xv.m128_f32[0], xv.m128_f32[1], xv.m128_f32[2] };

	if (std::fabsf(newDir.dot({ 0,1,0 })) > 0.98f)
	{
		newDir = dir;
	}

	SetTarget(m_position + newDir);
}

void D3D12Camera::SetTarget(Float3 target)
{
	m_target = target;
	mHasChanged = true;
}

void D3D12Camera::SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane)
{
	DirectX::XMStoreFloat4x4(&mPerspectiveMatrix, DirectX::XMMatrixPerspectiveFovLH(fov, aspectRatio, nearPlane, farPlane));
	mHasChanged = true;
}

void D3D12Camera::SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane)
{
	DirectX::XMStoreFloat4x4(&mPerspectiveMatrix, DirectX::XMMatrixOrthographicLH(width, height, nearPlane, farPlane));
	mHasChanged = true;
}

DirectX::XMFLOAT4X4 D3D12Camera::GetViewPerspective() const
{
	if (mHasChanged) {
		DirectX::XMStoreFloat4x4(&mViewMatrix, DirectX::XMMatrixLookAtLH({ m_position.x, m_position.y, m_position.z }, { m_target.x, m_target.y, m_target.z }, { 0,1,0 }));
	
		DirectX::XMStoreFloat4x4(&mViewPerspectiveMatrix, DirectX::XMLoadFloat4x4(&mViewMatrix)*DirectX::XMLoadFloat4x4(&mPerspectiveMatrix));

		mHasChanged = false;
	}
	return mViewPerspectiveMatrix;
}