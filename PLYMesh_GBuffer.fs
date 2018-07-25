#version 430 core
layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gColor;

in Pipe{
   vec3 Normal;
   vec3 FragPos;
   vec3 FragColor;
   vec2 TexCoord;
}fsInput;

void main(){
    gPosition = vec4(fsInput.FragPos,1.0f);
    gNormal = vec4(normalize(fsInput.Normal),1.0f);
    gColor = vec4(fsInput.FragColor,1.0);
}