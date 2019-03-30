#version 430 core
out vec4 color;

in vec2 texcoord;

uniform sampler2D irrmap;

uniform sampler2D shift_map;

void main(){
    //color = vec4(texture(irrmap, texcoord).xyz,1);

	vec2 off = texture(shift_map, texcoord).xy;
	//vec2 off = (*255-127)*(1.0/960);
	//vec2 newtex = texcoord + (texture(shift_map, texcoord).xy - 0.5);
	//color = texture(irrmap, newtex).xyzw;
	vec2 newt = vec2((off.y*255-127)*(1.0/960),(off.x*255-126.5)*(-1.0/960))+texcoord;
	color = texture(irrmap, newt).xyzw;
	
	if(abs(off.y*255-127)>1e-1||abs(off.x*255-127)>1e-1)
		color = vec4(0,1,0,0);

}