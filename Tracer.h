#ifndef __TRACER_H
#define __TRACER_H

#include <RenderSurface.h>
#include <math/Math.h>

class Tracer {
public:
	Tracer(RenderSurface &surface);
	virtual ~Tracer();

	void Trace();

private:
	int Cast(colorf &color, const Math::Vector3 &ray, const Math::Vector3 &cam, int depth);
	
	RenderSurface &m_Surface;

	Math::Vector3 m_Camera;
	Math::Vector3 m_Target;
};

#endif
