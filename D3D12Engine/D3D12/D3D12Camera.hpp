#pragma once
#include "../Camera.hpp"
#include <DirectXMath.h>

struct Sphere {
	float radius;
	Float3 center;
};

struct Plane {
	Float3 normal;
	float d;
};

class Frustum {
public:
	Frustum();
	virtual ~Frustum();

	void CreateFrustum(const DirectX::XMFLOAT4X4& viewprojection, const Float3& position, float FOV, float farPlane);

	bool CheckPoint(float x, float y, float z) const;
	bool CheckPoint(DirectX::XMFLOAT3 p) const;
	//bool CheckAgainstFrustum(OBB& obb) const;
	//bool CheckAgainstFrustum(AABA& aaba) const;
	bool CheckAgainstFrustum(const Sphere& sphere) const;

private:
	Plane m_planes[6];
	float DistanceToPlane(const Plane& p, const Float3& point) const;
};

class D3D12Camera : public Camera {
public:
	D3D12Camera();
	virtual ~D3D12Camera();

	// Inherited via Camera
	virtual void SetPosition(const Float3& position) override;
	virtual void Move(const Float3& position) override;
	virtual void Rotate(const Float3& axis, float angle) override;
	virtual void SetTarget(const Float3& target) override;

	virtual void SetPerspectiveProjection(float aspectRatio, float fov, float nearPlane, float farPlane) override;
	virtual void SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane) override;

	virtual MyRay ScreenCoordToRay(const Float2& normalizedScreenCoords) override;

	DirectX::XMFLOAT4X4 GetViewPerspective() const;
	const DirectX::XMFLOAT4X4& GetViewPerspective_ref() const;

	const Frustum& GetFrustum() const;
private:
	mutable DirectX::XMFLOAT4X4 mViewMatrix;
	DirectX::XMFLOAT4X4 mPerspectiveMatrix;
	DirectX::XMFLOAT4X4 m_rotationMatrix;

	mutable DirectX::XMFLOAT4X4 mViewPerspectiveMatrix;
	mutable bool m_vp_needsUpdate = true;

	mutable DirectX::XMFLOAT4X4 m_ViewMatrix_inversed;
	mutable bool m_view_inv_needsUpdate = true;


	mutable Frustum m_frustum;
};