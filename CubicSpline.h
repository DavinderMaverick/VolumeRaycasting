#pragma once

#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "TransferFunctionControlPoint.h"
#include "CubicSplineSegment.h"

std::vector<CubicSplineSegment> CalculateCubicSpline(std::vector<TransferFunctionControlPoint> v)
{
	int n = v.size() - 1; //n = number of segments

	//For each segment within a Cubic Spline (Total n+1 segments) 
	std::vector<glm::vec4> gamma(n + 1);
	std::vector<glm::vec4> delta(n + 1);
	std::vector<glm::vec4> D(n + 1);

	/* We solve the equation
	   [2 1       ] [D[0]]   [3(v[1] - v[0])  ]
	   |1 4 1     | |D[1]|   |3(v[2] - v[0])  |
	   |  1 4 1   | | .  | = |      .         |
	   |    ..... | | .  |   |      .         |
	   |     1 4 1| | .  |   |3(v[n] - v[n-2])|
	   [       1 2] [D[n]]   [3(v[n] - v[n-1])]

	   by using row operations to convert the matrix to upper triangular
	   and then back sustitution.  The D[i] are the derivatives at the knots.
	   */

	gamma[0] = glm::vec4(0.0f);
	gamma[0].x = 1.0 / 2.0f;
	gamma[0].y = 1.0 / 2.0f;
	gamma[0].z = 1.0 / 2.0f;
	gamma[0].w = 1.0 / 2.0f;

	for (int i = 1; i < n; i++)
	{
		gamma[i] = glm::vec4(1.0f) / ((glm::vec4(4.0f) * glm::vec4(1.0f)) - gamma[i - 1]);
	}
	gamma[n] = glm::vec4(1.0f) / ((glm::vec4(2.0f) * glm::vec4(1.0f)) - gamma[n - 1]);

	delta[0] = glm::vec4(3.0f) * (v[1].color - v[0].color) * gamma[0];

	for (int i = 1; i < n; i++)
	{
		delta[i] = (glm::vec4(3.0f) * (v[i + 1].color - v[i - 1].color) - delta[i - 1]) * gamma[i];
	}
	delta[n] = (glm::vec4(3.0f) * (v[n].color - v[n - 1].color) - delta[n - 1]) * gamma[n];

	D[n] = delta[n];
	for (int i = n - 1; i >= 0; i--)
	{
		D[i] = delta[i] - gamma[i] * D[i + 1];
	}

	//now compute the coefficients of cubic polynomial for each segment within a cubic spline
	std::vector<CubicSplineSegment> C(n);
	for (int i = 0; i < n; i++)
	{
		glm::vec4 a = v[i].color;
		glm::vec4 b = D[i];
		glm::vec4 c = glm::vec4(3.0f) * (v[i + 1].color - v[i].color) - glm::vec4(2.0f) * D[i] - D[i + 1];
		glm::vec4 d = glm::vec4(2.0f) * (v[i].color - v[i + 1].color) + D[i] + D[i + 1];
		C[i] = CubicSplineSegment(a, b, c, d);
	}

	return C;
}