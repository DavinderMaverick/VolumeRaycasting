#version 330 core

in vec4 clip_position;

//This means that when writing in the variable “color”, we will actually write in the Render Target 0, 
//which happens to be our texture because DrawBuffers[0] is GL_COLOR_ATTACHMENTi, which is, in our case, renderedTexture.
layout(location = 0) out vec3 start_point;
layout(location = 1) out vec3 end_point;

//out vec4 FragColor;

/*
To store Front, Back Face in the correct texture of the framebuffer, 
according to whether the fragment belongs to a front face or not, 
and perform a conversion from two-unit cube coordinates (range [1,1])[NDC] to texture coordinates (range [0,1]).
*/
void main()
{
	vec3 ndc_position = clip_position.xyz; // / clip_position.w;
    if (gl_FrontFacing) 
	{
        //start_point = 0.5 * (ndc_position + 1.0);
        start_point = ndc_position + 0.5;
		end_point = vec3(0);
    } 
	else 
	{
        start_point = vec3(0);
        //end_point = 0.5 * (ndc_position + 1.0);
		end_point = ndc_position + 0.5;
    }
	//FragColor = vec4(end_point, 1.0); 
}