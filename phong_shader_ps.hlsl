#include "phong_shader.hlsli"

Texture2D color_map : register(t0);
SamplerState color_sampler_state : register(s0);
Texture2D normal_map : register(t1);

float4 main(VS_OUT pin):SV_TARGET
{
    float4 color = { 0, 0, 0, 0 };
    
    float4 diffuse_color = color_map.Sample(color_sampler_state, pin.texcoord);
    
    float3 E = normalize(pin.world_position.xyz - camera_position.xyz);
    float3 L = normalize(directional_light_direction.xyz);
    //float3 N = normalize(pin.world_normal.xyz);
    
    float3x3 mat = { normalize(pin.tangent), normalize(pin.binormal), normalize(pin.normal) };
    float3 N = normal_map.Sample(color_sampler_state, pin.texcoord).rgb;
    //ノーマルテクスチャ法線をワールドへ変換
    N = normalize(mul(N * 2.0f - 1.0f, mat));
    
    float3 ambient = ambient_color.rgb * ka.rgb;
    ambient += CalcHemiSphereLight(N, float3(0, 1, 0), sky_color.rgb, ground_color.rgb, hemisphere_weight);
    //float3 directional_diffuse = 0;
    //{
    //    float power = saturate(dot(N, -L));
    //    directional_diffuse = directional_light_color.rgb * power * kd.rgb;
    //}
    
    float3 directional_diffuse = CalcLambert(N, L, directional_light_color.rgb, kd.rgb);
    //float3 directional_diffuse = CalcHalfLambert(N, L, directional_light_color.rgb, kd.rgb);
    
    //float3 directional_specular = 0;
    //{
    //    float3 R = reflect(L, N);
    //    float power = max(dot(-E, R), 0);
    //    power = pow(power, 128);
    //    directional_specular = directional_light_color.rgb * power * ks.rgb;
    //}
    
    float3 directional_specular = CalcPhongSpecluar(N, L, E, directional_light_color.rgb,ks.rgb);
    
    float3 rim_color = CalcRimLight(N, E, L, directional_light_color.rgb);
    
    color = float4(ambient, diffuse_color.a);
    color.rgb += diffuse_color.rgb * directional_diffuse;
    color.rgb += directional_specular;
    color.rgb += rim_color.rgb;
    
    color = CalcFog(color, fog_color, fog_range.xy, length(pin.world_position.xyz - camera_position.xyz));
    
    return color;
}