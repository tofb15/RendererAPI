#pragma once
#include "Utills/Math.hpp"
namespace FusionReactor {

	class Camera {
	public:
		virtual ~Camera();

		virtual void SetPosition(const Float3& position) = 0;
		virtual void Move(const Float3& position) = 0;
		virtual void Rotate(const Float3& axis, float angle) = 0;
		virtual void SetTarget(const Float3& target) = 0;
		virtual void SetPerspectiveProjection(float fov, float aspectRatio, float nearPlane, float farPlane) = 0;
		virtual void SetPerspectiveOrthographic(float width, float height, float nearPlane, float farPlane) = 0;

		void Clone(Camera* other) const;

		Float3 GetPosition() const;
		Float3 GetTarget() const;
		Float3 GetTargetDirection() const;
		Float3 GetRight() const;

		float GetFOV() const;

		/*
			@return True if any of the cameras properties have changed since it was last used for rendering.
		*/
		bool HasViewChanged() const;
	protected:
		Camera();

		Float3 m_position;
		Float3 m_target;
		float m_fov = 0;

		//int id;
		//int viewMatrixIndex;
		//int perspectiveMatrixIndex;
		//bool hasViewChanged;

	};
}