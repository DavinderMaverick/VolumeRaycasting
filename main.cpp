#include <iostream>
#include <string>
#include <sstream>
#include <vector>
#include <math.h>
#include <fstream>

#include <glad\glad.h>
#include <GLFW\glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include "shader.h"
#include "fixedcamera.h"

#include "CubicSpline.h"
#include "Trackball.h"

using namespace std;

const float SPEED = 5.0f;

//Load 3D Volume as Texture Function
//volume dimensions
const int XDIM = 128;
const int YDIM = 256;
const int ZDIM = 256;

//Prototypes
bool createWindowAndRC(string windowTitle, GLFWwindow **window);
void framebuffer_size_callback(GLFWwindow *window, int width, int height);
void processInput(GLFWwindow *window);
double calcFPS(GLFWwindow *window, double theTimeInterval = 1.0, string theWindowTitle = "NONE");
bool LoadVolume(string volume_file);
GLuint GenerateVolumeTexture();
void saveTextureToBMP(GLuint &textureID, string outputFileName);
void loadAndCreateTexture(GLuint &texture, string fileName);
void scroll_callback(GLFWwindow * window, double xoffset, double yoffset);
GLuint computeTransferFunction(vector<TransferFunctionControlPoint> colorKnots, vector<TransferFunctionControlPoint> alphaKnots);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void cursor_position_callback(GLFWwindow * window, double xpos, double ypos);
GLuint setUpEmptyTexture();
void generateGradients(int sampleSize);
float sampleVolume(int x, int y, int z);
int clip(int n, int lower, int upper);
void filterNxNxN(int n);
glm::vec3 sampleNxNxN(int x, int y, int z, int n);
bool isInBounds(int x, int y, int z);
GLuint loadGradientTexture();
glm::vec3 sampleNxNxN(int x, int y, int z, int n);
glm::vec3 sampleGradients(int x, int y, int z);
bool fileExists(string fileName);
ostream& operator<<(ostream& out, const glm::vec3 &v);
istream& operator>>(istream& in, glm::vec3 &v);
void readGradientsFromFile();

//settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

//camera
FixedCamera camera(glm::vec3(-2.5f, 0, 0));
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
//const float ZOOM = 45.0f;

//timing
float deltaTime = 0.0f; //Time between current frame and last frame
float lastFrame = 0.0f; //Time of last frame

static Trackball trackball(SCR_WIDTH, SCR_HEIGHT, 2.0f);

string windowTitle = "MultiPass GPU Raycasting";

vector<float> scalars;
vector<glm::vec3> gradients;

int currentShaderIndex = 2;

string gradientFileName = "gradients.bin";

bool mouseBtnPressed = false;

struct Volume
{
	GLubyte* pData;
	vector<float> scalars;
	vector<glm::vec3> gradients;
}volumeData;

struct Transform
{
	glm::quat rotQuat;
}cubeTransform;

int main()
{
	stbi_flip_vertically_on_write(true);
		
	if (!LoadVolume("male.raw"))
	{
		cout << "Volume Loading Error" << endl;
		return -1;
	}

	cout << "Loading and setting up volume data" << endl;

	if (fileExists(gradientFileName))
	{
		readGradientsFromFile();
	}
	else
	{
		generateGradients(1);
	}

	cout << "Volume Data setup complete" << endl;

	GLFWwindow *window = nullptr;

	if (!createWindowAndRC(windowTitle, &window))
	{
		cout << "Window and Rendering Context Creation Error" << endl;
		return -1;
	}

	glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwSetCursorPosCallback(window, cursor_position_callback);

	// configure global opengl state
	glEnable(GL_DEPTH_TEST);
	

	//Load Volume Texture from File
	GLuint volumeTex = GenerateVolumeTexture();
	GLuint gradientTexture = loadGradientTexture();

	cubeTransform.rotQuat = glm::quat(0.0f, 0.0f, 0.0f, 1.0f);

	vector<TransferFunctionControlPoint> colorKnots = {
		TransferFunctionControlPoint(.91f, .7f, .61f, 0),
		TransferFunctionControlPoint(.91f, .7f, .61f, 80),
		TransferFunctionControlPoint(1.0f, 1.0f, .85f, 82),
		TransferFunctionControlPoint(1.0f, 1.0f, .85f, 256)
	};

	vector<TransferFunctionControlPoint> alphaKnots = {
		TransferFunctionControlPoint(0.0f, 0),
		TransferFunctionControlPoint(0.0f, 40),
		TransferFunctionControlPoint(0.2f, 60),
		TransferFunctionControlPoint(0.05f, 63),
		TransferFunctionControlPoint(0.0f, 80),
		TransferFunctionControlPoint(0.9f, 82),
		TransferFunctionControlPoint(1.0f, 256)
	};

	GLuint transferFuncTexture = computeTransferFunction(colorKnots, alphaKnots);

	//First Pass Setup
	//build and compile our shader program
	Shader firstPassShader("firstPass.vert", "firstPass.frag");

	//setup vertex data (and buffer(s)) and configure vertex attributes
	//unit cube vertices 
	GLfloat cubeVertices[] = {
	-0.5, -0.5, -0.5,
	-0.5, -0.5, 0.5,
	-0.5, 0.5, -0.5,
	-0.5, 0.5, 0.5,
	0.5, -0.5, -0.5,
	0.5, -0.5, 0.5,
	0.5, 0.5, -0.5,
	0.5, 0.5, 0.5,
	};

	GLuint cubeIndices[] = {
	1,5,7,
	7,3,1,
	0,2,6,
	6,4,0,
	0,1,3,
	3,2,0,
	7,5,4,
	4,6,7,
	2,3,7,
	7,6,2,
	1,0,4,
	4,5,1
	};

	GLuint cubeVAO, cubeVBO, cubeEBO;

	glGenVertexArrays(1, &cubeVAO);
	glGenBuffers(1, &cubeVBO);
	glGenBuffers(1, &cubeEBO);

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s)
	glBindVertexArray(cubeVAO);

	glBindBuffer(GL_ARRAY_BUFFER, cubeVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(cubeVertices), cubeVertices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, cubeEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cubeIndices), cubeIndices, GL_STATIC_DRAW);

	//Creating the render target
	GLuint firstPassFrameBuffer = 0;
	glGenFramebuffers(1, &firstPassFrameBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, firstPassFrameBuffer);

	//The textures we're going to render to
	GLuint renderTextureFront, renderTextureBack;

	renderTextureFront = setUpEmptyTexture();

	renderTextureBack = setUpEmptyTexture();
		
	//Configure Framebuffer

	// Set "renderTextureFront" as our colour attachement #0
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, renderTextureFront, 0);

	// Set "renderTextureBack" as our colour attachement #1
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, renderTextureBack, 0);

	// Set the list of render Targets.
	GLenum renderTargets[2] = { GL_COLOR_ATTACHMENT0 , GL_COLOR_ATTACHMENT1 };
	glDrawBuffers(2, renderTargets); // "2" is the size of renderTargets

	// Something may have gone wrong during the process, depending on the capabilities of the GPU. To check
	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		cout << "Framebuffer problem" << endl;
	
	/* For Second Pass we are to draw a simple quad that fills the screen */
	//Second Pass Setup
	//build and compile our shader program
	Shader raycastDiffuseShader("raycastDiffuse.vert", "raycastDiffuse.frag");
	Shader raycastShader("raycast.vert", "raycast.frag");

	Shader *currentRaycastShader = &raycastShader;

	//setup vertex data (and buffer(s)) and configure vertex attributes
	//quad vertices
	float quadVertices[] = {
		-1.0f, -1.0f,
		1.0f, -1.0f,
		-1.0f,  1.0f,
		-1.0f,  1.0f,
		1.0f, -1.0f,
		1.0f,  1.0f,
	};

	GLuint quadVAO, quadVBO;

	glGenVertexArrays(1, &quadVAO);
	glGenBuffers(1, &quadVBO);

	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s)
	glBindVertexArray(quadVAO);

	glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);
	glEnableVertexAttribArray(0);


	//load and create a texture
	//GLuint renderTextureFront, renderTextureBack;

	//glGenTextures(1, &renderTextureFront);
	//loadAndCreateTexture(renderTextureFront, "renderTextureFront.bmp");

	//glGenTextures(1, &renderTextureBack);
	//loadAndCreateTexture(renderTextureBack, "renderTextureBack.bmp");

	
	int max_dim = max(XDIM, max(YDIM, ZDIM));

	float step_length = 1.0f / max_dim;

	//render loop
	while (!glfwWindowShouldClose(window))
	{
		//per-frame time logic
		float currentFrame = glfwGetTime();
		deltaTime = currentFrame - lastFrame;
		lastFrame = currentFrame;

		//input
		processInput(window);

		glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)SCR_WIDTH / (float)SCR_HEIGHT, 0.1f, 100.0f);

		//First Pass
		//Render to our framebuffer
		//Rendering to the texture is straightforward. Simply bind your framebuffer, and draw your scene as usual
		// Render to our framebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, firstPassFrameBuffer);
		glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		firstPassShader.use();
		//Setup Vertex Attributes
		glBindVertexArray(cubeVAO);
		//Setup Uniforms
		glm::mat4 view = camera.GetViewMatrix();
		glm::mat4 model = mat4_cast(cubeTransform.rotQuat);
		firstPassShader.setMat4("MVP", projection * view * model);

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE);

		//Draw Scene
		glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);

		//Second Pass
		//Render to display
		//Render to the screen, to render to the screen. Use 0 as the second parameter of glBindFramebuffer
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
		//glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_INT, 0);


		//glEnable(GL_CULL_FACE);
		//glCullFace(GL_BACK);

		//glDisable(GL_BLEND);

		if (currentShaderIndex == 1)
		{
			currentRaycastShader = &raycastShader;
		}

		if (currentShaderIndex == 2)
		{
			currentRaycastShader = &raycastDiffuseShader;
		}
		
		//Full Screen Quad
		currentRaycastShader->use();
		//Setup Vertex Attributes
		glBindVertexArray(quadVAO);
		//Setup Uniforms

		currentRaycastShader->setFloat("step_length", step_length);

		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, renderTextureFront);
	
		glUniform1i(glGetUniformLocation(currentRaycastShader->ID, "front_face"), 0);

		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, renderTextureBack);

		glUniform1i(glGetUniformLocation(currentRaycastShader->ID, "back_face"), 1);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_3D, volumeTex);
		glUniform1i(glGetUniformLocation(currentRaycastShader->ID, "volumeTex"), 2);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_1D, transferFuncTexture);
		glUniform1i(glGetUniformLocation(currentRaycastShader->ID, "transferFuncTex"), 3);

		if (currentShaderIndex == 2)
		{
			glActiveTexture(GL_TEXTURE4);
			glBindTexture(GL_TEXTURE_3D, gradientTexture);
			glUniform1i(glGetUniformLocation(currentRaycastShader->ID, "gradientTex"), 4);
		}

		//Draw Scene
		glDrawArrays(GL_TRIANGLES, 0, 6);

		//glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
		glfwSwapBuffers(window);
		glfwPollEvents();

		calcFPS(window, 1.0, windowTitle);
	}
	
	//optional: de-allocate all resources once they've outlived their purpose:

	//glfw: terminate, clearing all previously allocated GLFW resources
	glfwTerminate();

	return 0;
}

bool createWindowAndRC(string windowTitle, GLFWwindow **window)
{
	//GLFW: initialize and configure
	glfwInit();
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	//GLFW: window creation
	*window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, windowTitle.c_str(), NULL, NULL);
	if (*window == NULL)
	{
		cout << "Failed to create GLFW window" << endl;
		glfwTerminate();
		return false;
	}

	glfwMakeContextCurrent(*window);

	//Limit the frameRate to 60fps.
	glfwSwapInterval(1);

	//glad: load all OpenGL function pointers
	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		cout << "Failed to initialize GLAD" << endl;
		return false;
	}

	return true;
}

//process all inputs: query GLFW whether relevant keys are pressed/released this frame and react accordingly
void processInput(GLFWwindow *window)
{
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
		glfwSetWindowShouldClose(window, true);

	if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		currentShaderIndex = 1;

	if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		currentShaderIndex = 2;
}

//glfw: whenever the window size changed (by OS or user resize) this callback function executes
void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
	//make sure the viewport matches the new window dimensions
	glViewport(0, 0, width, height);
}

double calcFPS(GLFWwindow *window, double theTimeInterval, string theWindowTitle)
{
	// Static values which only get initialised the first time the function runs
	static double t0Value = glfwGetTime(); // Set the initial time to now
	static int    fpsFrameCount = 0;             // Set the initial FPS frame count to 0
	static double fps = 0.0;           // Set the initial FPS value to 0.0

	// Get the current time in seconds since the program started (non-static, so executed every time)
	double currentTime = glfwGetTime();

	// Ensure the time interval between FPS checks is sane (low cap = 0.1s, high-cap = 10.0s)
	// Negative numbers are invalid, 10 fps checks per second at most, 1 every 10 secs at least.
	if (theTimeInterval < 0.1)
	{
		theTimeInterval = 0.1;
	}
	if (theTimeInterval > 10.0)
	{
		theTimeInterval = 10.0;
	}

	// Calculate and display the FPS every specified time interval
	if ((currentTime - t0Value) > theTimeInterval)
	{
		// Calculate the FPS as the number of frames divided by the interval in seconds
		fps = (double)fpsFrameCount / (currentTime - t0Value);

		// If the user specified a window title to append the FPS value to...
		if (theWindowTitle != "NONE")
		{
			// Convert the fps value into a string using an output stringstream
			std::ostringstream stream;
			stream << fps;
			std::string fpsString = stream.str();

			// Append the FPS value to the window title details
			theWindowTitle += " | FPS: " + fpsString;

			// Convert the new window title to a c_str and set it
			const char* pszConstString = theWindowTitle.c_str();
			glfwSetWindowTitle(window, pszConstString);
		}
		else // If the user didn't specify a window to append the FPS to then output the FPS to the console
		{
			std::cout << "FPS: " << fps << std::endl;
		}

		// Reset the FPS frame counter and set the initial time to be now
		fpsFrameCount = 0;
		t0Value = glfwGetTime();
	}
	else // FPS calculation time interval hasn't elapsed yet? Simply increment the FPS frame counter
	{
		fpsFrameCount++;
	}

	// Return the current FPS - doesn't have to be used if you don't want it!
	return fps;
}


bool LoadVolume(string volume_file)
{
	ifstream infile(volume_file.c_str(), ios_base::binary);

	if (infile.good())
	{
		//read the volume data file
		volumeData.pData = new GLubyte[XDIM*YDIM*ZDIM];
		infile.read(reinterpret_cast<char*>(volumeData.pData), XDIM*YDIM*ZDIM * sizeof(GLubyte));
		infile.close();

		volumeData.scalars.resize(XDIM*YDIM*ZDIM);
		volumeData.gradients.resize(XDIM*YDIM*ZDIM);

		for (int i = 0; i < volumeData.scalars.size(); i++)
		{
			volumeData.scalars[i] = (float)volumeData.pData[i] / 255;
		}
		return true;
	}
	return false;
}

//function that load a volume from the given raw data file and 
//generates an OpenGL 3D texture from it
GLuint GenerateVolumeTexture()
{
	GLuint textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_3D, textureID);

	// set the texture parameters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);

	//set the mipmap levels (base and max)
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_BASE_LEVEL, 0);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAX_LEVEL, 4);

	//allocate data with internal format and foramt as (GL_RED)	
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RED, XDIM, YDIM, ZDIM, 0, GL_RED, GL_UNSIGNED_BYTE, volumeData.pData);
	//GL_CHECK_ERRORS

	//generate mipmaps
	glGenerateMipmap(GL_TEXTURE_3D);

	return textureID;
}

// Generates gradients using a central differences scheme
void generateGradients(int sampleSize)
{
	int n = sampleSize;
	glm::vec3 normal = glm::vec3(0.0f);
	glm::vec3 s1, s2;

	int index = 0;
	for (int z = 0; z < ZDIM; z++)
	{
		for (int y = 0; y < YDIM; y++)
		{
			for (int x = 0; x < XDIM; x++)
			{
				s1.x = sampleVolume(x - n, y, z);
				s2.x = sampleVolume(x + n, y, z);
				s1.y = sampleVolume(x, y - n, z);
				s2.y = sampleVolume(x, y + n, z);
				s1.z = sampleVolume(x, y, z - n);
				s2.z = sampleVolume(x, y, z + n);

				volumeData.gradients[index++] = glm::normalize(s2 - s1);
				if (isnan(volumeData.gradients[index - 1].x))
				{
					volumeData.gradients[index - 1] = glm::vec3(0.0f);
				}
			}
		}
	}

	filterNxNxN(3);

	//Now save the gradient
	ofstream fout;
	fout.open(gradientFileName, ios_base::binary);

	glm::vec3 *temp = &(volumeData.gradients[0]);

	fout.write(reinterpret_cast<char*>(temp), XDIM*YDIM*ZDIM *  sizeof(glm::vec3));
	fout.close();
	cout << "Gradients saved into file\n";
}

void readGradientsFromFile()
{
	//Load the gradients
	ifstream fin;
	fin.open(gradientFileName, ios_base::binary);
	
	glm::vec3 *temp = new glm::vec3[XDIM*YDIM*ZDIM];
	
	fin.read(reinterpret_cast<char*>(temp), XDIM*YDIM*ZDIM * sizeof(glm::vec3));
	fin.close();

	volumeData.gradients = vector<glm::vec3>(temp, temp + XDIM * YDIM*ZDIM);

	cout << "Gradients loaded from file\n";
}

bool fileExists(string fileName)
{
	ifstream infile(fileName.c_str());

	if (infile.good())
	{
		return true;
	}
	else
	{
		return false;
	}
}

float sampleVolume(int x, int y, int z)
{
	x = (int)clip(x, 0, XDIM - 1);
	y = (int)clip(y, 0, YDIM - 1);
	z = (int)clip(z, 0, ZDIM - 1);
	return volumeData.scalars[x + (y * XDIM) + (z * XDIM * YDIM)];
}

//filter the gradients with an NxNxN box filter
//Should be an odd number of samples. 3 used by default.
void filterNxNxN(int n)
{
	int index = 0;
	for (int z = 0; z < ZDIM; z++)
	{
		for (int y = 0; y < YDIM; y++)
		{
			for (int x = 0; x < XDIM; x++)
			{
				volumeData.gradients[index++] = sampleNxNxN(x, y, z, n);
			}
		}
	}
}

glm::vec3 sampleNxNxN(int x, int y, int z, int n)
{
	n = (n - 1) / 2;
	glm::vec3 avg = glm::vec3(0.0f);
	int num = 0;

	for (int k = z - n; k <= z + n; k++)
	{
		for (int j = y - n; j <= y + n; j++)
		{
			for (int i = x - n; i <= x + n; i++)
			{
				if (isInBounds(i, j, k))
				{
					avg += sampleGradients(i, j, k);
					num++;
				}
			}
		}
	}
	avg /= (float)num;

	if (avg != glm::vec3(0.0f))
	{
		glm::normalize(avg);
	}

	return avg;
}

bool isInBounds(int x, int y, int z)
{
	return ((x >= 0 && x < XDIM) && (y >= 0 && y < YDIM) && (z >= 0 && z < ZDIM));
}


GLuint loadGradientTexture()
{
	//write to texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_3D, textureID);

	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	//set texture filtering paramaters
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	
	//glGenerateMipmap(GL_TEXTURE_2D);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, XDIM, YDIM, ZDIM, 0, GL_RGB, GL_FLOAT, &volumeData.gradients[0].x);

	return textureID;
}

glm::vec3 sampleGradients(int x, int y, int z)
{
	return volumeData.gradients[x + (y * XDIM) + (z * XDIM * YDIM)];
}

int clip(int n, int lower, int upper)
{
	return std::max(lower, std::min(n, upper));
}

void saveTextureToBMP(GLuint &textureID, string outputFileName)
{
	glBindTexture(GL_TEXTURE_2D, textureID);
	unsigned char* imageData = (unsigned char *)malloc((int)(SCR_WIDTH * SCR_HEIGHT * (3)));
	//glReadPixels(0, 0, SCR_WIDTH, SCR_WIDTH, GL_RGB, GL_UNSIGNED_BYTE, imageData);
	glGetTexImage(GL_TEXTURE_2D, 0, GL_RGB, GL_UNSIGNED_BYTE, imageData);
	stbi_write_bmp(outputFileName.c_str(), SCR_WIDTH, SCR_HEIGHT, 3, imageData);
	delete imageData;
}

void loadAndCreateTexture(GLuint &texture, string fileName)
{
	//texture 
	glBindTexture(GL_TEXTURE_2D, texture);
	//set the texture wrapping paramaters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	//set texture filtering paramaters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//load image, create texture and generate mipmaps
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true); // tell stb_image.h to flip loaded texture's on the y-axis.
	unsigned char *data = stbi_load(fileName.c_str(), &width, &height, &nrChannels, 0);
	if (data)
	{
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		cout << "Failed to load texture" << endl;
	}
	stbi_image_free(data);
	glBindTexture(GL_TEXTURE_2D, 0);
}

//whenever the mouse scroll wheel scrolls, this callback is called
void scroll_callback(GLFWwindow *window, double xoffset, double yoffset)
{
	camera.ProcessMouseScroll(yoffset);
}

GLuint setUpEmptyTexture()
{
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_2D, textureID);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
	//set texture filtering paramaters
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glGenerateMipmap(GL_TEXTURE_2D);
	// Give an empty image to OpenGL ( the last "0" )
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
	return textureID;
}


GLuint computeTransferFunction(vector<TransferFunctionControlPoint> colorKnots, vector<TransferFunctionControlPoint> alphaKnots)
{
	glm::vec4 transferFunc[256];

	vector<CubicSplineSegment> colorCubicSpline = CalculateCubicSpline(colorKnots);
	vector<CubicSplineSegment> alphaCubicSpline = CalculateCubicSpline(alphaKnots);

	int numTF = 0; //Each isoVal from 0 to 255 will be mapped to a color and alpha using transfer func

	for (int i = 0; i < colorKnots.size() - 1; i++)
	{
		int steps = colorKnots[i + 1].isoValue - colorKnots[i].isoValue;

		for (int j = 0; j < steps; j++)
		{
			float k = (float)j / (float)(steps - 1);

			transferFunc[numTF++] = colorCubicSpline[i].GetPointOnSpline(k);
		}
	}

	numTF = 0; //Each isoVal from 0 to 255 will be mapped to a color and alpha using transfer func

	for (int i = 0; i < alphaKnots.size() - 1; i++)
	{
		int steps = alphaKnots[i + 1].isoValue - alphaKnots[i].isoValue;

		for (int j = 0; j < steps; j++)
		{
			float k = (float)j / (float)(steps - 1);

			transferFunc[numTF++].w = alphaCubicSpline[i].GetPointOnSpline(k).w;
		}
	}

	//write to texture
	GLuint textureID;
	glGenTextures(1, &textureID);

	// "Bind" the newly created texture : all future texture functions will modify this texture
	glBindTexture(GL_TEXTURE_1D, textureID);

	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
	//set texture filtering paramaters
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glGenerateMipmap(GL_TEXTURE_2D);
	glTexImage1D(GL_TEXTURE_1D, 0, GL_RGBA, 256, 0, GL_RGBA, GL_FLOAT, &transferFunc[0].x);

	return textureID;
}


void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		if (action == GLFW_PRESS)
		{
			double xpos, ypos;
			glfwGetCursorPos(window, &xpos, &ypos);
			trackball.mouseButtonCallback(true, xpos, ypos);
			mouseBtnPressed = true;
		}
		if (action == GLFW_RELEASE)
		{
			trackball.mouseButtonCallback(false, 0, 0);
			mouseBtnPressed = false;
		}
	}
}

void cursor_position_callback(GLFWwindow * window, double xpos, double ypos)
{
	trackball.cursorCallback(window, xpos, ypos);

	if (mouseBtnPressed)
	{
		bool flag = false;
		glm::quat newRotation = trackball.createWorldRotationQuat(camera.GetViewMatrix(), deltaTime, flag);
		//glm::mat4 newRotation = trackball.createViewRotationMatrix(deltaTime);
		if (!flag)
			cubeTransform.rotQuat = newRotation * cubeTransform.rotQuat;
	}
}

ostream& operator<<(ostream& out, const glm::vec3 &v)
{
	out << v.x << " " << v.y << " " << v.z;
	return out;
}

istream& operator>>(istream& in, glm::vec3 &v)
{
	in >> v.x >> v.y >> v.z;
	return in;
}