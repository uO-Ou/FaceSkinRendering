#version 430 core

out vec4 color;

in vec2 texcoord;

uniform sampler2D input_texture;
uniform sampler2D stenc_texture;

uniform int UorV;
uniform float ImgSize;
uniform float GaussWidth;

bool check(vec2 coord){
     return length(texture(stenc_texture,coord).xyz)>0.1;
}

void main(){
    vec4 sum = vec4(0,0,0,0);
    if(check(texcoord)){
        vec4 base_color = texture(input_texture, texcoord);

        float netFilterWidth = 1.0 / ImgSize * GaussWidth;
        float curve[7] = {0.006,0.061,0.242,0.383,0.242,0.061,0.006};
    
        vec2 step;
        if(UorV==1) step = vec2(netFilterWidth, 0.0);
        else step = vec2(0.0,netFilterWidth);
        
        vec2 coords = texcoord - step * 3;
        for(int i=0;i<7;++i){
            if(check(coords)){
                vec4 tap = texture(input_texture, coords);
                sum += tap*curve[i];
            }
            else{
                sum += base_color*curve[i];
            }
            coords += step;
        }
        sum.w = base_color.w;
    }
    color = sum;
}