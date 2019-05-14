#pragma once

#include <iostream>

#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <math.h>
#include <algorithm>
#include<glm/gtc/quaternion.hpp>
#include<glm/common.hpp>

class Trackball
{
private:
	int windowWidth;
	int windowHeight;
	int mouseEvent;
	GLfloat rollSpeed;
	GLfloat angle;
	glm::vec3 prevPos;
	glm::vec3 currPos;
	glm::vec3 camAxis;
	glm::quat _quat;
	bool flag;
	
public:
	bool track;

	Trackball(int window_width, int window_height, GLfloat roll_speed = 1.0f);
	glm::vec3 toScreenCoord(double x, double y);

	void mouseButtonCallback(bool _track, double x, double y);
	void cursorCallback(GLFWwindow *window, double x, double y);

	glm::quat createViewRotationQuat(float deltaTime);
	glm::quat createWorldRotationQuat(glm::mat4 view_matrix, float deltaTime, bool& f);
	glm::quat createModelRotationQuat(glm::mat4 view_matrix, glm::mat4 model_matrix, float deltaTime);
};