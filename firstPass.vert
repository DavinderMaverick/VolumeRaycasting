#version 330 core

layout (location = 0) in vec3 position;
out vec4 clip_position;

uniform mat4 MVP;

void main()
{
	clip_position = /*MVP * */vec4(position,1.0);
    gl_Position = MVP * vec4(position, 1.0);
}