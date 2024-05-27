struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};

cbuffer COLOR_FILTER : register(b4)
{
    float hueShift; // F‘Š’²®
    float saturation; // Ê“x’²®
    float brightness; // –¾“x’²®
    float dummy;
};

#include "filter_functions.hlsli"