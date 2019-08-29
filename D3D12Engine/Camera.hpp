#pragma once
#include "Math.hpp"

class Camera {
public:
	virtual ~Camera();

	virtual void SetPosition(Float3 position) = 0;
	virtual void Move(Float3 position) = 0;
	virtual void Rotate(Float3 axis, float angle) = 0;
	virtual void SetTarget(Float3 target) = 0;
	virtual void SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane) = 0;
	virtual void SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane) = 0;

	Float3 GetPosition() const;
	Float3 GetTarget() const;
	Float3 GetTargetDirection() const;
	Float3 GetRight() const;

	/*
		@return True if any of the cameras properties have changed since it was last used for rendering.
	*/
	bool HasViewChanged() const;
protected:
	Camera();

	Float3 m_position;
	Float3 m_target;

	//int id;
	//int viewMatrixIndex;
	//int perspectiveMatrixIndex;
	//bool hasViewChanged;

};