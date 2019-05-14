#pragma once

#include <glad\glad.h>
#include <glm\glm.hpp>
#include <glm\gtc\matrix_transform.hpp>

#include <vector>

const float ZOOM = 45.0f;

class FixedCamera
{
public:
	//Camera Attributes
	glm::vec3 Position;
	glm::vec3 TargetPos;
	glm::vec3 WorldUp;
	glm::mat4 ViewMatrix;
	glm::vec3 Direction;//Actually the opposite Direction in which camera is looking
	glm::vec3 Up;
	glm::vec3 Right;
	float Zoom;

	FixedCamera(glm::vec3 pos = glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3 targetPos = glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f))
	{
		Position = pos;
		WorldUp = up;
		TargetPos = targetPos;
		ViewMatrix = glm::lookAt(Position, TargetPos, WorldUp);
		Direction = glm::normalize(Position - TargetPos);
		Right = glm::normalize(glm::cross(Direction, WorldUp));
		Up = glm::normalize(glm::cross(Right, Direction));
		Zoom = ZOOM;
	}

	glm::mat4 GetViewMatrix()
	{
		ViewMatrix = glm::lookAt(Position, TargetPos, WorldUp);
		Direction = glm::normalize(Position - TargetPos);
		Right = glm::normalize(glm::cross(Direction, WorldUp));
		Up = glm::normalize(glm::cross(Right, Direction));
		return ViewMatrix;
	}

	//Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
	void ProcessMouseScroll(float yoffset)
	{
		if (Zoom >= 1.0f && Zoom <= 45.0f)
			Zoom -= yoffset;
		if (Zoom <= 1.0f)
			Zoom = 1.0f;
		if (Zoom >= 45.0f)
			Zoom = 45.0f;
	}

};