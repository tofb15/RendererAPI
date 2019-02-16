#pragma once
#include "Math.hpp"

class Camera {
public:
	virtual void SetPosition(Float3 position) = 0;
	virtual void SetTarget(Float3 target) = 0;
	virtual void SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane) = 0;
	virtual void SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane) = 0;

	Float3 GetPosition() const;
	Float3 GetTarget() const;
	Float3 GetTargetDirection() const;

	bool HasViewChanged() const;

private:
	int id;
	int viewMatrixIndex; //Move?
	int perspectiveMatrixIndex;
	bool hasViewChanged;

	Transform transform;
};