#version 430 core
out vec4 color;

in Pipe{
	vec3 FragPos;
	vec3 LightSpaceProjPos;
	vec2 TexCoord;
	mat3 TBN;
}fsInput;

//material
uniform struct Material{
	uint flags;              //0:no texture,1:ambientTexture,2:diffuseTexture,4:normal map 8 shadow map
	vec3 diffuse, ambient;
	sampler2D shadowTexture;
	sampler2D normalTexture;
	sampler2D diffuseTexture;
	sampler2D ambientTexture;
}material;

//lights
#define MAX_POINT_LIGHT_NUMBER 6
#define MAX_DIRECTIONAL_LIGHT_NUMBER 2
uniform struct PointLight{
    vec3 position;
    vec3 intensity;
    vec3 attenuation;
}pointLights [MAX_POINT_LIGHT_NUMBER];

uniform struct DirectionalLight{
	vec3 direction;
	vec3 intensity;
}directionaLights [MAX_DIRECTIONAL_LIGHT_NUMBER];

uniform int pointLightNumber, directionaLightNumber;

uniform vec3 cameraPosition;

uniform float irrmix;

const float ShadowBias = 1e-2;
const float SpecularStrength = 0.2f;
const float Gamma = 2.2f;
const float M = 2.0f;

const int PCFD = 5;

float shadow_factor(){
	if((material.flags&8u)>0){

		ivec2 tdim = textureSize(material.shadowTexture, 0);
		float stepx = 1.0 / tdim.x;
		float stepy = 1.0 / tdim.y;

		float ticker = 0;
		for(int x=-PCFD/2;x<=PCFD/2;++x) for(int y=-PCFD/2;y<=PCFD/2;++y){
			float newx = fsInput.LightSpaceProjPos.x + stepx * x;
			float newy = fsInput.LightSpaceProjPos.y + stepy * y;
			float newd = texture(material.shadowTexture, vec2(newx,newy)).x;

			if(fsInput.LightSpaceProjPos.z<newd+ShadowBias){
				++ticker;
			}
		}
		return ticker / (PCFD*PCFD);

		//float nearz = texture(material.shadowTexture, vec2(fsInput.LightSpaceProjPos.x, fsInput.LightSpaceProjPos.y)).x;
		//if(fsInput.LightSpaceProjPos.z > nearz + ShadowBias){
		//	return 0;
		//} else return 1;
	} else return 1;
}

void main(){
	vec3 N = texture(material.normalTexture, fsInput.TexCoord).bgr;
	N = N * 2.0 - 1.0;
	N = normalize(fsInput.TBN * N);
	
	vec3 Diffuse = vec3(0,0,0), Specular = vec3(0,0,0);

	//deal directional light0, for shadow
	float shafact = shadow_factor();
	//float shafact = 1.0;
	if(shafact > 0){
		// Diffuse
		Diffuse += (max(dot(N, -directionaLights[0].direction), 0.0) * directionaLights[0].intensity * shafact);
		
		// Specular
		vec3 H = normalize(cameraPosition - fsInput.FragPos - directionaLights[0].direction);
        Specular += SpecularStrength * pow(max(dot(H, N), 0.0), M) * directionaLights[0].intensity * shafact;
	}

	//loop directional lights
    for(int i = 1; i < directionaLightNumber; ++i){
		// Diffuse
		Diffuse += (max(dot(N, -directionaLights[i].direction), 0.0) * directionaLights[i].intensity);
		
		// Specular
		vec3 H = normalize(cameraPosition - fsInput.FragPos - directionaLights[i].direction);
        Specular += SpecularStrength * pow(max(dot(H, N), 0.0), M) * directionaLights[i].intensity;
	}

    //loop point lights
    for(int i = 0;i < pointLightNumber; ++i){
         vec3 L = pointLights[i].position - fsInput.FragPos;
         float distance = length(L);    L = L * (1.0/distance);
         float attenuation = 1.0 / dot(pointLights[i].attenuation,vec3(1,distance,distance*distance));

         //Diffuse
         Diffuse += max(dot(L,N),0.0)*pointLights[i].intensity*attenuation;

         // Specular
         vec3 H = normalize(cameraPosition - fsInput.FragPos + pointLights[i].position-fsInput.FragPos);
         Specular += SpecularStrength * pow(max(dot(H,N),0.0),M) * pointLights[i].intensity*attenuation;
    }	

	vec3 albedo = material.diffuse;
	if((material.flags&2u)>0)
		albedo = pow(texture(material.diffuseTexture, fsInput.TexCoord).bgr,vec3(Gamma)) * material.diffuse;
	
	Diffuse = Diffuse * pow(albedo, vec3(irrmix));

	color = vec4(Diffuse.r, Diffuse.g, Diffuse.b, Specular.x);
	//color = vec4(1, 1, 1, 1);
}


//color = vec4((Ambient+Diffuse+Specular).bgr,1.0f);
//color = vec4(pow(Ambient+Diffuse+Specular,vec3(1.0/Gamma)), 1.0f);
//color = vec4(texture(material.diffuseTexture,fsInput.TexCoord).bgr,1.0f);

//color = vec4(fsInput.TBN[0], 1.0f);
//color = vec4(fsInput.T, 1.0f);

//color = vec4(fsInput.TexCoord.x,fsInput.TexCoord.y,0.0f,1.0f);

//vec3 N = normalize(fsInput.TBN[2]);

// Ambient
// vec3 Ambient = vec3(0, 0, 0);
// if((material.flags&1u)==0) Ambient = material.ambient;
// else Ambient = (texture(material.ambientTexture,fsInput.TexCoord).bgr * material.ambient);