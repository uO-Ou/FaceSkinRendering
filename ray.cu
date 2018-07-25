#include <optix.h>
#include <optixu/optixu_math_namespace.h>
using namespace optix;

struct PerRayData_shadow{
	float distance;
};

rtDeclareVariable(int, WinWidth, , );
rtDeclareVariable(int, WinHeight, , );

/**************for ray-casting*************/
rtDeclareVariable(float, nearp, , );
rtDeclareVariable(float2, canvaSize, , );
rtDeclareVariable(float3, camera_x, , );
rtDeclareVariable(float3, camera_y, , );
rtDeclareVariable(float3, camera_z, , );
/******************************************/

//lights buffer
rtBuffer<float, 1> lights_buffer;
rtBuffer<unsigned int, 1> sm_buffer;
rtBuffer<uchar4, 2> output_buffer_uchar4;
rtBuffer<float4, 2> output_buffer_float4;

rtDeclareVariable(float, diff_mix, , );
rtDeclareVariable(float, face_roughness,,);

//gbuffer textures
rtTextureSampler<float4, 2>  position_texture;
rtTextureSampler<float4, 2>  normal_texture;
rtTextureSampler<float4, 2>  diffuse_texture;
rtTextureSampler<float4, 2>  specular_texture;
rtTextureSampler<float4, 2>  reflectance_texture;

rtDeclareVariable(uint, light_cnt, , );
rtDeclareVariable(uint, scr_width, , );
rtDeclareVariable(float3, camera_pos, , );
rtDeclareVariable(float3, bgcolor, , );
rtDeclareVariable(int, option, , );

//others
rtDeclareVariable(uint, shadow_ray_type, , );                    //声明ray的type
rtDeclareVariable(float, scene_epsilon, , );                     //epsilon
rtDeclareVariable(uint2, launch_index, rtLaunchIndex, );         //launch_index  

rtDeclareVariable(rtObject, shadow_casters, , );

rtDeclareVariable(PerRayData_shadow, prd_shadow, rtPayload, );   //声明ray的携带变量
rtDeclareVariable(float, t_hit, rtIntersectionDistance, );       //声明距离变量为t_hit


RT_PROGRAM void any_hit_shadow(){
	prd_shadow.distance = t_hit;
	rtTerminateRay();
}

RT_PROGRAM void rayCasting(){
	float tx = ((0.5f + launch_index.x) / WinWidth - 0.5f) * canvaSize.x;
	float ty = ((0.5f + launch_index.y) / WinHeight - 0.5f) * canvaSize.y;
	float3 direction = camera_x*tx + camera_y*ty + camera_z*nearp;
	
	PerRayData_shadow prd;  prd.distance = -1;
	optix::Ray ray = optix::make_Ray(camera_pos+direction, normalize(direction), shadow_ray_type, scene_epsilon, 10000);
	rtTrace(shadow_casters, ray, prd);
	if (prd.distance > 0)
		output_buffer_uchar4[launch_index] = make_uchar4(0, 255, 0, 255);
}

/*
option: 0, uchar4
		1, float4
*/

__device__ float fresnelReflectance(float3 halfDir, float3 viewDir, float F0){
	float base = 1.0 - dot(halfDir, viewDir);
	float exponential = powf(base, 5.0);
	return exponential + F0*(1.0 - exponential);
}

__device__ float PHBeckmann(float nDotH, float m){
	float alpha = acos(nDotH);
	float tanAlpha = tan(alpha);
	float value = exp(-(tanAlpha*tanAlpha) / (m*m)) / (m*m*powf(nDotH, 4.0));
	return value;
}

__device__ float brdf_KS(float3 normal, float3 lightDir, float3 viewDir, float roughness, float specPower){
	float result = 0.0;
	float NdotL = dot(normal,lightDir);
	if (NdotL > 0){
		float3 h = lightDir + viewDir;
		float3 halfDir = normalize(h);
		float NdotH = dot(normal,halfDir);
		float PH = PHBeckmann(NdotH,roughness);
		const float F0 = 0.028;
		float F = fresnelReflectance(halfDir,viewDir,F0);
		float frSpec = max(PH*F / dot(h, h), 0.0);
		result = frSpec * NdotL * specPower;
	}
	return result;
}

RT_PROGRAM void shading(){
	float3 norm = make_float3(tex2D(normal_texture, launch_index.x, launch_index.y));
	float3 shadeResult = make_float3(0.0f, 0.0f, 0.0f);

	float final_spec = 0.0;
	if (dot(norm, norm)>0.0f) {	//valid pixel
		float3 pos = make_float3(tex2D(position_texture, launch_index.x, launch_index.y));
		float3 mat_diff = make_float3(tex2D(diffuse_texture, launch_index.x, launch_index.y));
		
		for (unsigned int lid = 0u; lid < light_cnt; ++lid){
			float3 light_pos = *((float3 *)(&lights_buffer[lid * 9 + 0]));
			float3 light_col = *((float3 *)(&lights_buffer[lid * 9 + 3]));
			//float3 light_atn = *((float3 *)(&lights_buffer[lid * 9 + 6]));

			float3 L = light_pos - pos;
			if (dot(L, norm)>0.0f){
				//distance to light
				float dist = sqrtf(dot(L, L));
				L /= dist;

				//shadow ray. check if light was blocked
				PerRayData_shadow prd;  prd.distance = -1;
				optix::Ray ray = optix::make_Ray(pos, L, shadow_ray_type, scene_epsilon, dist);
				rtTrace(shadow_casters, ray, prd);

				if (prd.distance > 0){
					
				}
				else{
					float NdotL = max(0.0f, dot(L, norm));
					float reflect_ratio = tex2D(reflectance_texture, WinWidth*NdotL, WinHeight*face_roughness).x;
					
					//diffuse
					shadeResult += light_col * mat_diff * NdotL * (1 - reflect_ratio);

					//specular
					float3 viewDir = normalize(camera_pos - pos);
					float spec = brdf_KS(norm, L, viewDir, face_roughness, 1) * reflect_ratio;

					//shadeResult += make_float3(spec*light_col.x, spec*light_col.y, spec*light_col.z);
					final_spec += spec*light_col.x;
				}
			}
		}
		shadeResult += mat_diff * 0.05f;
	}
	else{
		//shadeResult = make_float3(0.498, 0.235, 0.137);
		shadeResult = bgcolor;
	}
	shadeResult.x = clamp(shadeResult.x, 0.0f, 1.0f);
	shadeResult.y = clamp(shadeResult.y, 0.0f, 1.0f);
	shadeResult.z = clamp(shadeResult.z, 0.0f, 1.0f);
	if (option == 0)
		output_buffer_uchar4[launch_index] = make_uchar4(shadeResult.x * 255, shadeResult.y * 255, shadeResult.z * 255, 255);
	else
		output_buffer_float4[launch_index] = make_float4(shadeResult.x, shadeResult.y, shadeResult.z, final_spec);
}

RT_PROGRAM void exception(){

}
