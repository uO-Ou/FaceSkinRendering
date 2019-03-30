#version 430 core
out vec4 color;

in Pipe{
	vec2 TexCoord;
} fsInput;

uniform struct Material{
	uint flags;
	vec3 diffuse, ambient;
	sampler2D diffuseTexture;
	sampler2D ambientTexture;
} material;

uniform sampler2D irrmap0;
uniform sampler2D irrmap1;
uniform sampler2D irrmap2;
uniform sampler2D irrmap3;
uniform sampler2D irrmap4;
uniform sampler2D irrmap5;

uniform float irrmix;

vec3 weights[6] = {
    vec3(0.233,0.455,0.649),
    vec3(0.100,0.336,0.344),
    vec3(0.118,0.198,0.000),
    vec3(0.113,0.007,0.007),
    vec3(0.358,0.004,0.000),
    vec3(0.078,0.000,0.000)
};

float gamma = 2.2;

void main(){

	// Ambient
	vec3 Ambient = material.ambient;
	if((material.flags & 1u)!=0) 
		Ambient *= (pow(texture(material.ambientTexture,fsInput.TexCoord).bgr,vec3(gamma,gamma,gamma)));

	// Diffuse
	vec3 Albeo = material.diffuse;
	if((material.flags & 2u)!=0)  
		Albeo *= (pow(texture(material.diffuseTexture,fsInput.TexCoord).bgr,vec3(gamma,gamma,gamma)));
	
	vec4 base = texture(irrmap0, fsInput.TexCoord);
    vec3 tap0 = base.xyz;
    vec3 tap1 = texture(irrmap1, fsInput.TexCoord).xyz;
    vec3 tap2 = texture(irrmap2, fsInput.TexCoord).xyz;
    vec3 tap3 = texture(irrmap3, fsInput.TexCoord).xyz;
    vec3 tap4 = texture(irrmap4, fsInput.TexCoord).xyz;
    vec3 tap5 = texture(irrmap5, fsInput.TexCoord).xyz;

    vec3 total_weight = weights[0]+weights[1]+weights[2]+weights[3]+weights[4]+weights[5];
    vec3 irr = vec3(0,0,0);
    irr += tap0*weights[0];
    irr += tap1*weights[1];
    irr += tap2*weights[2];
    irr += tap3*weights[3];
    irr += tap4*weights[4];
    irr += tap5*weights[5];
    irr *= (1.0 / total_weight);

	//specular
    vec3 Specular = vec3(base.w,base.w,base.w);

    color = vec4(pow(Ambient+irr*pow(Albeo,vec3(1.0-irrmix)),vec3(1.0/gamma))+Specular,1.0);
	//color = vec4(pow(Ambient+base.xyz*Albeo+Specular,vec3(1.0/gamma)),1.0);
}
