#version 430
layout(location = 0) in vec3 pos;
layout(location = 1) in vec3 normal;

uniform samplerBuffer Model_matrix;
uniform mat4 View_matrix;
uniform mat4 Projection_matrix;

vec3 light_color1 = vec3(0.1,0.5,0.0);
vec3 light_color2 = vec3(0.5,0.1,0.0);

flat out vec3 light_color;
out vec3 WorldPos;
out vec3 in_normal;

void main()
{
    vec4 col1 = texelFetch(Model_matrix, gl_InstanceID * 4);
    vec4 col2 = texelFetch(Model_matrix, gl_InstanceID * 4 + 1);
    vec4 col3 = texelFetch(Model_matrix, gl_InstanceID * 4 + 2);
    vec4 col4 = texelFetch(Model_matrix, gl_InstanceID * 4 + 3);
    
	mat4 Model = mat4(col1, col2, col3, col4);
   
    if(mod(gl_InstanceID, 2)  == 0)
	  light_color = light_color1;
	else
	  light_color = light_color2;

	WorldPos = (Model * vec4(pos,1.0)).xyz;
	in_normal = (Model * vec4(normal, 0.0)).xyz;
	gl_Position = Projection_matrix * View_matrix * Model * vec4(pos,1.0);
}
