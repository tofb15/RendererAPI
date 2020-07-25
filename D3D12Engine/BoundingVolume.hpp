#include "Math.hpp"

class BoundingVolume {
public:
	BoundingVolume() {};
	~BoundingVolume() {};

	virtual float RayIntersection(const MyRay& ray) const = 0;
	virtual bool RayIntersection_Fast(const MyRay& ray) const = 0;
private:

};

class BoundingSphere : public BoundingVolume {
public:
	BoundingSphere(Float3 origin, float radius);
	~BoundingSphere();

	void SetRadius(float radius);
	void SetOrigin(const Float3& origin);
	float GetRadius() const;
	Float3 GetOrigin() const;

	/*
		Fast ray vs intersection test.
		This function will returns true if the ray intersects the sphere but will not calculate where on the sphere the collision happen.
	*/
	bool RayIntersection_Fast(const MyRay& ray) const;
	float RayIntersection(const MyRay& ray) const { return 0; };

private:
	float m_radius;
	Float3 m_origin;
};