#version 430 core
out vec4 color;

in vec2 texcoord;

uniform sampler2D picture;

void main(){
    color = vec4(texture(picture,texcoord).xyz,1);
}