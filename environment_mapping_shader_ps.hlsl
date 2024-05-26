#include "environment_mapping_shader.hlsli"

Texture2D color_map : register(t0);
SamplerState color_sampler_state : register(s0);

Texture2D environment_map : register(t3);

float4 main(VS_OUT pin):SV_TARGET
{
    float4 diffuse_color = color_map.Sample(color_sampler_state, pin.texcoord);
    
    float3 E = normalize(pin.world_position.xyz - camera_position.xyz);
    float3 N = normalize(pin.world_normal.xyz);
    float4 color = diffuse_color;
    color.rgb = CalcSphereEnvironment(environment_map, color_sampler_state, color.rgb, N, E, environment_value);
    
    return color;
}

