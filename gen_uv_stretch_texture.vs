#version 430 core

layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

out Pipe{
	vec3 FragPos;
}vsOutput;

uniform mat4 model;
uniform mat4 projection_view;

void main(){
	vec4 inworld = model * vec4(position,1.0f);
	vsOutput.FragPos = inworld.xyz;

	vec4 proj = projection_view * inworld;
	gl_Position = vec4((texcoord.x-0.5)*2, (texcoord.y-0.5)*2, proj.z/proj.w, 1.0f);
}