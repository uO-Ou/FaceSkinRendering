#version 430 core

out vec4 color;

in vec2 texcoord;

uniform sampler2D input_texture;
uniform sampler2D stretch_map;

uniform int UorV;
uniform float ImgSize;
uniform float GaussWidth;

void main(){
	vec4 base_color = texture(input_texture, texcoord);
	vec2 stuv = texture(stretch_map, texcoord).zy;

	stuv = vec2(1,1);

    vec4 sum = vec4(0,0,0,0);

    float netFilterWidth = 50.0 / ImgSize * GaussWidth;
    float curve[7] = {0.006,0.061,0.242,0.382,0.242,0.061,0.006};
    
    vec2 step;
    if(UorV==1) step = vec2(netFilterWidth * stuv.x, 0.0);
    else step = vec2(0.0, netFilterWidth * stuv.y);
        
    vec2 coords = texcoord - step * 3;
    for(int i=0;i<7;++i){
		vec4 tap = texture(input_texture, coords);
        sum += tap*curve[i];
		coords += step;
    }

    sum.w = base_color.w;
	color = sum;

	//color = vec4(stuv.x,stuv.y,0.0,1.0f);
}