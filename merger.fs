#version 430 core
out vec4 color;

in vec2 texcoord;

uniform sampler2D diff0;
uniform sampler2D diff1;
uniform sampler2D diff2;
uniform sampler2D diff3;
uniform sampler2D diff4;
uniform sampler2D diff5;

vec3 weights[6] = {
    vec3(0.233,0.455,0.649),
    vec3(0.100,0.336,0.344),
    vec3(0.118,0.198,0.000),
    vec3(0.113,0.007,0.007),
    vec3(0.358,0.004,0.000),
    vec3(0.078,0.000,0.000)
};

void main(){
    vec3 tap0 = texture(diff0, texcoord).xyz;
    vec3 tap1 = texture(diff1, texcoord).xyz;
    vec3 tap2 = texture(diff2, texcoord).xyz;
    vec3 tap3 = texture(diff3, texcoord).xyz;
    vec3 tap4 = texture(diff4, texcoord).xyz;
    vec3 tap5 = texture(diff5, texcoord).xyz;

    vec3 total_weight = weights[0]+weights[1]+weights[2]+weights[3]+weights[4]+weights[5];
    vec3 diffuse = vec3(0,0,0);
    diffuse += tap0*weights[0];
    diffuse += tap1*weights[1];
    diffuse += tap2*weights[2];
    diffuse += tap3*weights[3];
    diffuse += tap4*weights[4];
    diffuse += tap5*weights[5];
    diffuse /= total_weight;

    float spec = texture(diff0, texcoord).w*3;
    vec3 specular = vec3(spec,spec,spec);

    color = vec4(diffuse+specular,1.0);
}
