#version 430 core
out vec4 color;

in vec2 texcoord;

uniform sampler2D normalTexture;
uniform sampler2D positionTexture;
uniform sampler2D materialTexture;

void main(){
     color = vec4(texture(normalTexture,texcoord));
} 