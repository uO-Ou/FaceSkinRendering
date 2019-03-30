#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

uniform mat4 model;
uniform mat4 projection_view;

out Pipe{
	vec2 TexCoord;
}vsOutput;

void main(){
	  vec4 inworld = model * vec4(position, 1.0f);
	  gl_Position = projection_view * inworld;
	  vsOutput.TexCoord = texcoord;
}