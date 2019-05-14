#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "TransferFunctionControlPoint.h"

class CubicSplineSegment
{
	glm::vec4 a, b, c, d; // a + b*s + c*s^2 + d*s^3 
public:
	CubicSplineSegment()
	{
	}
	CubicSplineSegment(glm::vec4 a, glm::vec4 b, glm::vec4 c, glm::vec4 d);
	glm::vec4 GetPointOnSpline(float s); // s-> 0 to 1, for a particular segment between knots
	~CubicSplineSegment();
};