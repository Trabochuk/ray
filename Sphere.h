#ifndef __SPHERE_H
#define __SPHERE_H

#include <math/Vector3.h>
#include <Ray.h>

class Sphere {
public:
	Sphere();
	Sphere(const Math::Vector3 &pos, float radius);
	virtual ~Sphere();

	void SetPos(const Math::Vector3 &pos) { m_Center = pos; }
	void SetRadius(float rad) { m_Radius = rad; }

	bool Intersect(const Ray &ray);
	bool Intersect(const Ray &ray, Math::Vector3 &pos, Math::Vector3 &normal);

private:
	Math::Vector3 m_Center;
	float m_Radius;
};

#endif

