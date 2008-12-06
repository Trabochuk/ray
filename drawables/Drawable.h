#ifndef __DRAWABLE_H
#define __DRAWABLE_H

#include <libmath/Vector3.h>
#include <shaders/Shader.h>

class Ray;

class Drawable {
protected:
	Drawable();

public:
	virtual ~Drawable();

	virtual bool Intersect(const Ray &ray) const = 0;
	virtual bool Intersect(const Ray &ray, Math::Vector3d &pos, Math::Vector3d &normal) const = 0;

	void SetShader(ShaderPtr s) { m_Shader = s; }
	ShaderPtr GetShader() const { return m_Shader; }

private:
	ShaderPtr m_Shader;
};


#endif

