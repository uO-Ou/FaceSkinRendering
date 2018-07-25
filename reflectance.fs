#version 430 core
out vec4 color;

in vec2 texcoord;

#define NUM_TERMS 128
#define PI_PI 6.2831853
#define PI_5 1.57079633

float fresnelReflectance(vec3 halfDir, vec3 viewDir, float F0){
	float base = 1.0 - dot(halfDir, viewDir);
	float exponential = pow(base, 5.0);
	return exponential + F0*(1.0 - exponential);
}

float PHBeckmann(float nDotH, float m){
	float alpha = acos(nDotH);
	float tanAlpha = tan(alpha);
	float value = exp(-(tanAlpha*tanAlpha) / (m*m)) / (m*m*pow(nDotH, 4.0));
	return value;
}

float brdf_KS(vec3 normal, vec3 lightDir, vec3 viewDir, float roughness, float specPower){
	float result = 0.0;
	float NdotL = dot(normal,lightDir);
	if (NdotL > 0){
		vec3 h = lightDir + viewDir;
		vec3 halfDir = normalize(h);
		float NdotH = dot(normal,halfDir);
		float PH = PHBeckmann(NdotH,roughness);
		const float F0 = 0.028;
		float F = fresnelReflectance(halfDir,viewDir,F0);
		float frSpec = max(PH*F / dot(h, h), 0.0);
		result = frSpec * NdotL * specPower;
	}
	return result;
}

float reflectance(float NdotV, float roughness){
    float sum = 0.0;
    vec3 N = vec3(0.0,0.0,1.0);
    vec3 V = vec3(0.0,sqrt(1.0-NdotV*NdotV),NdotV);
    for(int j=0;j<NUM_TERMS;++j){
        
        float phi = float(j) / float(NUM_TERMS-1) * PI_PI;
        float sin_phi = sin(phi);
        float cos_phi = cos(phi);

        float local_sum = 0.0;
        for(int k=0;k<NUM_TERMS;++k){
            float theta = float(k)/float(NUM_TERMS-1) * PI_5;
            float sin_theta = sin(theta);
            float cos_theta = cos(theta);
            
            vec3 L = vec3(sin_phi*sin_theta,cos_phi*sin_theta,cos_theta);

            local_sum += brdf_KS(N,L,V,roughness,1)*sin_theta;
        }
        sum += local_sum * PI_5 / float(NUM_TERMS);

    }
    return sum * PI_PI / float(NUM_TERMS);
}

void main(){
     //color = vec4(texture(texture,texcoord));
	 
     //color = vec4(texcoord.x,texcoord.y,0.5,1);

     float value = reflectance(texcoord.x,texcoord.y);
     color = vec4(value,value,value,1);
} 