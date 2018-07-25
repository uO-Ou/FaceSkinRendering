#version 430 core
out vec4 color;

in Pipe{
   vec3 Normal;
   vec3 FragPos;
   vec3 FragColor;
   vec2 TexCoord;
}fsInput;

void main(){
	color = vec4(fsInput.FragColor,1.0f);
}