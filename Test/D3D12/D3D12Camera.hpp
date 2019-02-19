#pragma once
#include "../Camera.hpp"

class D3D12Camera : public Camera {
public:
	D3D12Camera();
	virtual ~D3D12Camera();


	// Inherited via Camera
	virtual void SetPosition(Float3 position) override;

	virtual void SetTarget(Float3 target) override;

	virtual void SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane) override;

	virtual void SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane) override;

};