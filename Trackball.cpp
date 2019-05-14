#include "Trackball.h"

/**
 * Constructor.
 * @param roll_speed the speed of rotation
 */
Trackball::Trackball(int window_width, int window_height, GLfloat roll_speed)
{
	this->windowWidth = window_width;
	this->windowHeight = window_height;

	this->mouseEvent = 0;
	this->rollSpeed = roll_speed;
	this->_quat = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	this->flag = false;
}

/**
 * Convert the mouse cursor coordinate on the window (i.e. from (0,0) to (windowWidth, windowHeight))
 * into normalized screen coordinate (i.e. (-1, -1) to (1, 1)
 */
glm::vec3 Trackball::toScreenCoord(double x, double y)
{
	glm::vec3 coord(0.0f);

	double wX = windowWidth - 1;
	double wY = windowHeight - 1;

	//if (xAxis)
		coord.x = (2 * x - wX) / wX;

	//if (yAxis)
		coord.y = -(2 * y - wY) / wY;

	/* Clamp it to border of the windows, comment these codes to allow rotation when cursor is not over window */
	//coord.x = glm::clamp(coord.x, -1.0f, 1.0f);
	//coord.y = glm::clamp(coord.y, -1.0f, 1.0f);

	float length2 = coord.x * coord.x + coord.y * coord.y;
	if (length2 <= 1.0)
		coord.z = sqrt(1.0 - length2);
	else
	{
		
		/*coord.x *= 1.0f / sqrt(length2);
		coord.y *= 1.0f / sqrt(length2);
		coord.z = 0;*/
		
		//std::cout << "H" << std::endl;
		/*coord.z = 1.0f / (2.0 * sqrt(length2));
		float length = sqrt(length2 + coord.z * coord.z);
		float lengthInv = 1 / length;
		coord = coord * lengthInv;*/
	}

	return glm::normalize(coord);
}

/**
 * Check whether we should start the mouse event
 * Event 0: when no tracking occured
 * Event 1: at the start of tracking, recording the first cursor pos
 * Event 2: tracking of subsequent cursor movement
 */
void Trackball::mouseButtonCallback(bool _track, double x, double y)
{
	if (_track)
	{
		/* Start of trackball, remember the first position */
		prevPos = toScreenCoord(x, y);
		currPos = prevPos;
		this->track = true;
		return;
	}
	else
	{
		this->track = false;
	}
}

float clamp(float val, float l, float h)
{
	if (val > h)
		val = h;
	else if (val < l)
		val = l;
	return val;
}

void Trackball::cursorCallback(GLFWwindow *window, double x, double y)
{
	if (!track)
		return;

	/* Tracking the subsequent */
	currPos = toScreenCoord(x, y);

	float t = glm::dot(prevPos, currPos);

	float dist = glm::distance(prevPos, currPos);

	if (abs(dist) < 0.000001f)
	{
		flag = true;
		//std::cout << "flag" << std::endl;
	}
	else
	{
		flag = false;
	}

	//if (t > 1)
	//{
	//	std::cout << "1" << std::endl;
	//}
	//if (t < 0)
	//{
	//	std::cout << "2" << std::endl;
	//}

	if (t > 1.0)
	{
		t = 1.0;
		//return;
	}
	if (t < -1.0) t = -1.0;
	//if (t < -0.9f) t = -1.0;


	/* Calculate the angle in radians, and clamp it between 0 and 90 degrees */
	angle = acos(t);

	//std::cout << "Angle : " << angle  << "PrevPos: ( " << prevPos.x << " , " << prevPos.y << " ) CurrPos: ( " << currPos.x << " , " << currPos.y<< " )" << std::endl;

	//angle = acos(std::max(0.0f, temp));

	/* Cross product to get the rotation axis, but it's still in camera coordinate */
	
		camAxis = glm::cross(prevPos, currPos);

		camAxis = glm::normalize(camAxis);

		prevPos = currPos;
		
}

/**
 * Create rotation matrix within the camera coordinate,
 * multiply this matrix with view matrix to rotate the camera
 */
glm::quat Trackball::createViewRotationQuat(float deltaTime)
{
	if (flag)
		return glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 axis = glm::normalize(camAxis);
	_quat = glm::angleAxis(glm::degrees(angle) * rollSpeed * deltaTime, axis);
	return _quat;
}

/**
 * Create rotation matrix within the world coordinate,
 * multiply this matrix with model matrix to rotate the object
 */
glm::quat Trackball::createWorldRotationQuat(glm::mat4 view_matrix, float deltaTime, bool& f)
{
	f = flag;
	glm::vec3 axis = glm::mat3(glm::inverse(view_matrix)) * camAxis;
	axis = glm::normalize(axis);
	_quat = glm::angleAxis(glm::degrees(angle) * rollSpeed * deltaTime, axis);
	return _quat;
}

glm::quat Trackball::createModelRotationQuat(glm::mat4 view_matrix, glm::mat4 model_matrix, float deltaTime)
{
	if (flag)
		return glm::quat(0.0f, 0.0f, 0.0f, 1.0f);
	glm::vec3 axis = glm::mat3(glm::inverse(view_matrix * model_matrix)) * camAxis;
	axis = glm::normalize(axis);
	_quat = glm::angleAxis(glm::degrees(angle) * rollSpeed * deltaTime, axis);
	return _quat;
}