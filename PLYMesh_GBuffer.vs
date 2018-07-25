#version 430 core
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 normal;
layout (location = 2) in vec2 texcoord;

layout (location = 3) in vec3 vcolor;

uniform mat4 model;
uniform mat4 projection_view;

out Pipe{
	vec3 Normal;
	vec3 FragPos;
	vec3 FragColor;
	vec2 TexCoord;
}vsOutput;

void main(){
	  //write to pipe
	  vec4 inworld = model * vec4(position,1.0f);
	  vsOutput.FragPos = inworld.xyz;
	  
	  mat3 normalMatrix = transpose(inverse(mat3(model)));
	  vsOutput.Normal = normalMatrix * normal;
	  
	  vsOutput.TexCoord = texcoord;

	  //calculate glPostion
	  gl_Position = projection_view * inworld;

	  //vec4 proj = projection_view * inworld;
	  //gl_Position = vec4(texcoord.x, texcoord.y, proj.z/proj.w*0.5+0.5,1.0);

	  vsOutput.FragColor = vcolor;
}