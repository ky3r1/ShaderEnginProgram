#include "ramp_shader.hlsli"

Texture2D color_map : register(t0);
SamplerState color_sampler_state : register(s0);
Texture2D ramp_map : register(t2);
SamplerState ramp_sampler_state : register(s2);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 diffuse_color = color_map.Sample(color_sampler_state, pin.texcoord);

    float3 E = normalize(pin.world_position.xyz - camera_position.xyz);
    float3 L = normalize(directional_light_direction.xyz);
    float3 N = normalize(pin.world_normal.xyz);
    float3 ambient = ambient_color.rgb * ka.rgb;
    
    float3 directional_diffuse = CalcRampShading(ramp_map, ramp_sampler_state, N, L, directional_light_color.rgb, kd.rgb);

    float3 directional_specular = CalcPhongSpecluar(N, L, E, directional_light_color.rgb, ks.rgb);

    float4 color = float4(ambient, diffuse_color.a);
    color.rgb += diffuse_color.rgb * directional_diffuse;
    color.rgb += directional_specular;
    return color;
}
