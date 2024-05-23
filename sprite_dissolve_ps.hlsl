#include "sprite_dissolve.hlsli"

Texture2D color_map : register(t0);
SamplerState color_sampler_state : register(s0);
Texture2D mask_texture : register(t1);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = color_map.Sample(color_sampler_state, pin.texcoord) * pin.color;
    float mask_value = mask_texture.Sample(color_sampler_state, pin.texcoord);
    //color.a *= mask_value;
    //stepŠÖ”‚ğ—p‚¢‚Ä“§‰ß’l‚ğ0/1‚É‚·‚é
    float alpha = step(parameters.x, mask_value);
    color.a *= alpha;
    return color;
}