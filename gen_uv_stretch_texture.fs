#version 430 core

in Pipe{
	vec3 FragPos;
}fsInput;

out vec4 color;

vec2 stretch(vec3 world_position){
	vec3 derivu = dFdx(world_position);
	vec3 derivv = dFdy(world_position);
	return vec2(1.0/length(derivu), 1.0/length(derivv))*0.01;
}

void main(){
	vec2 suv = stretch(fsInput.FragPos);
	color = vec4(suv.x,suv.y,0.0f,1.0f);
}