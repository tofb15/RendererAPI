#include "Math.hpp"
#include <vector>

enum class BoundingVolumeType {
	Sphere,
	Box
};

class BoundingVolume {
public:
	BoundingVolume(Float3 origin) : m_origin(origin) {};
	~BoundingVolume() { for (auto& e : subvolumes) { if (e) { delete e; } } };

	virtual float RayIntersection(const MyRay& ray) const = 0;
	virtual bool RayIntersection_Fast(const MyRay& ray) const = 0;
	virtual void SetOrigin(const Float3& origin);
	virtual Float3 GetOrigin() const;
	BoundingVolumeType GetType();

	virtual void SetSize(Float3 size) { m_size = size; };
	virtual Float3 GetSize() const {
		return m_size;
	};
	virtual void SetRadius(float radius) { m_size.x = radius; };
	virtual float GetRadius() const { return  m_size.x; };

	std::vector<BoundingVolume*> subvolumes;
protected:
	Float3 m_origin;
	Float3 m_size;
	BoundingVolumeType m_type = BoundingVolumeType::Box;
};

class BoundingSphere : public BoundingVolume {
public:
	BoundingSphere(Float3 origin, float radius);
	~BoundingSphere();

	/*
		Fast ray vs intersection test.
		This function will returns true if the ray intersects the sphere but will not calculate where on the sphere the collision happen.
	*/
	bool RayIntersection_Fast(const MyRay& ray) const override;
	float RayIntersection(const MyRay& ray) const override { return 0; };

private:
	//float m_radius;
};

class BoundingBox : public BoundingVolume {
public:
	BoundingBox(Float3 origin, Float3 size);
	~BoundingBox();

	/*
		Fast ray vs intersection test.
		This function will returns true if the ray intersects the sphere but will not calculate where on the sphere the collision happen.
	*/
	bool RayIntersection_Fast(const MyRay& ray) const override { return false; };
	float RayIntersection(const MyRay& ray) const override { return 0; };

private:
};