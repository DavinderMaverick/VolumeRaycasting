#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// Represents an RGBA value for a specific iso-value
class TransferFunctionControlPoint
{
public:
	glm::vec4 color;
	int isoValue;
	TransferFunctionControlPoint(float r, float g, float b, int isoValue);
	TransferFunctionControlPoint(float alpha, int isoValue);
	~TransferFunctionControlPoint();
};

