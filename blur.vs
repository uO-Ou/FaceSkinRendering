#version 430 core
layout (location = 0) in vec3 position;

out vec2 texcoord;

void main(){
   gl_Position = vec4(position,1.0f);
   texcoord = position.xy * 0.5f + 0.5f;
}