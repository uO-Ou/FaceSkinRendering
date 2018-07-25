#version 430 core
#extension GL_EXT_gpu_shader4 : enable
#extension GL_ARB_texture_multisample : enable

#define SPP 32

layout (location = 0) out vec4 gPosition;
layout (location = 1) out vec4 gNormal;
layout (location = 2) out vec4 gColor;

in vec2 texcoord;

uniform sampler2DMS position_msaa_texture;
uniform sampler2DMS normal_msaa_texture;
uniform sampler2DMS diffuse_msaa_texture;

void main(){    
    vec3 normal_accumulator = vec3(0.0);
    vec3 diffuse_accumulator = vec3(0.0);
    vec3 position_accumulator = vec3(0.0);

    ivec2 texSize = textureSize(diffuse_msaa_texture);
    for( int i = 0 ; i < SPP ; ++i ){
            normal_accumulator += texelFetch(normal_msaa_texture,ivec2(texcoord*texSize),i).xyz;
            diffuse_accumulator += texelFetch(diffuse_msaa_texture,ivec2(texcoord*texSize),i).xyz;
            position_accumulator += texelFetch(position_msaa_texture,ivec2(texcoord*texSize),i).xyz;
    }

    gNormal = vec4(normalize(normal_accumulator / SPP),1.0f);
    gColor = vec4(diffuse_accumulator / SPP,1.0f);
    gPosition = vec4(position_accumulator / SPP,1.0f);
}