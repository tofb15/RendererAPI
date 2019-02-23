#include "D3D12Camera.hpp"

D3D12Camera::D3D12Camera()
{
}

D3D12Camera::~D3D12Camera()
{
}

void D3D12Camera::SetPosition(Float3 position)
{
	mPosition = position;
	mHasChanged = true;
}

void D3D12Camera::Move(Float3 position)
{
	SetPosition(mPosition + position);
}

void D3D12Camera::SetTarget(Float3 target)
{
	mTarget = target;
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
		DirectX::XMStoreFloat4x4(&mViewMatrix, DirectX::XMMatrixLookAtLH({ mPosition.x, mPosition.y, mPosition.z }, { mTarget.x, mTarget.y, mTarget.z }, { 0,1,0 }));
	
		DirectX::XMStoreFloat4x4(&mViewPerspectiveMatrix, DirectX::XMMatrixTranspose(DirectX::XMLoadFloat4x4(&mViewMatrix) * DirectX::XMLoadFloat4x4(&mPerspectiveMatrix)));

		mHasChanged = false;
	}
	return mViewPerspectiveMatrix;
}
