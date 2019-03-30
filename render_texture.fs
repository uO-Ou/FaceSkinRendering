#version 430 core
out vec4 color;

in vec2 texcoord;

uniform sampler2D irrmap;

void main(){
    color = vec4(texture(irrmap, texcoord).xyz,1);
}