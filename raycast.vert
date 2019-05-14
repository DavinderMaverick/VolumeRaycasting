#version 330 core

//precision highp float;

layout (location = 0) in vec2 PositionNDC;

out vec2 Texcoord;

void main()
{
	// PositionNDC is in normalized device coordinates (NDC).
	// The (x, y) coordinates are in the range [-1, 1].
	
	gl_Position = vec4(PositionNDC, 0.0, 1.0);

	// To generate the texture coordinates for the resulting full screen
	// quad we need to transform the vertex position's coordinates from the
	// [-1, 1] range to the [0, 1] range. This is achieved with a scale and
	// a bias.

	Texcoord = PositionNDC * 0.5 + 0.5; 
}