#version 330 core
out vec4 FragColor;

in vec2 Texcoord;

//uniforms
uniform float step_length;

//texture samplers
uniform sampler2D front_face;
uniform sampler2D back_face;

//Actual Volume data
uniform sampler3D volumeTex;
uniform sampler1D transferFuncTex;
uniform sampler2D noiseTex;

// A very simple color transfer function
/*
 Simple transfer functions interpolates between a couple of colours denoting minimum and maximum intensity. 
 Uses exponential decay for the opacity 
*/
vec4 color_transfer(float intensity)
{
    vec3 high = vec3(1.0, 1.0, 1.0);
    vec3 low = vec3(0.0, 0.0, 0.0);
    float alpha = (exp(intensity) - 1.0) / (exp(1.0) - 1.0);
    return vec4(intensity * high + (1.0 - intensity) * low, alpha);
}

void main()
{
    vec3 ray_start = texture(front_face, Texcoord).rgb;
    vec3 ray_stop = texture(back_face, Texcoord).rgb;

    if (ray_start == ray_stop)
	{
		discard;
		return;
    }

    vec3 ray = ray_stop - ray_start;
    float ray_length = length(ray);
    vec3 step_vector = step_length * ray / ray_length;

	//32 X 32 Size of noise Texture
	float random = texture(noiseTex, gl_FragCoord.xy / vec2(32, 32)).x; 
	
	ray_start += step_vector * random;

    vec3 position = ray_start;
	vec4 color = vec4(0.0);

    // Stop when the end of the volume is reached or early ray termination (when alpha gets high enough)
    while (ray_length > 0)
	{

        float isoValue = texture(volumeTex, position).r;

		//Convert the sampled intensity value to color
		//Use Transfer Function
		//vec4 c = color_transfer(intensity);
		//vec4 c = vec4(intensity, intensity, intensity, intensity);
		//c.a *= 0.5f;

		vec4 c = texture(transferFuncTex, isoValue);
		//c.a *= 0.5f;

		vec4 dst = color;
		vec4 src = c;
		//Alpha-blending (Front to back)
		dst.rgb = dst.rgb + (1 - dst.a) * src.a * src.rgb;
		dst.a = dst.a + (1 - dst.a) * src.a;
		
		//break from the loop when alpha gets high enough
		if(dst.a >= 0.99f)
			break;

        ray_length -= step_length;
        position += step_vector;
		color = dst;
    }

	FragColor = color;
}