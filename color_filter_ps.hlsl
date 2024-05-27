#include "color_filter.hlsli"

Texture2D color_map : register(t0);
SamplerState color_sampler_state : register(s0);

float4 main(VS_OUT pin) : SV_TARGET
{
    float4 color = color_map.Sample(color_sampler_state, pin.texcoord) * pin.color;
	// RGB > HSV‚É•ÏŠ·
    color.rgb = RGB2HSV(color.rgb);

	// F‘Š’²®
    color.r += hueShift;

	// Ê“x’²®
    color.g *= saturation;

	// –¾“x’²®
    color.b *= brightness;

	// HSV > RGB‚É•ÏŠ·
    color.rgb = HSV2RGB(color.rgb);

    return color;
}
