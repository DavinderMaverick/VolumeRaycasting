#include "CubicSplineSegment.h"

CubicSplineSegment::CubicSplineSegment(glm::vec4 a, glm::vec4 b, glm::vec4 c, glm::vec4 d)
{
	this->a = a;
	this->b = b;
	this->c = c;
	this->d = d;
}

//evaluate the point using a cubic equation
glm::vec4 CubicSplineSegment::GetPointOnSpline(float s)
{
	return (((d * s) + c) * s + b) * s + a;
}

CubicSplineSegment::~CubicSplineSegment()
{
}
