struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
    float4 world_position : WORLD_POSITION;
};

cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection;
    float4 options;
    float4 camera_position;
};

cbuffer SKYMAP_CONSTANT_BUFFER : register(b7)
{
    row_major float4x4 inverse_view_projection;
};

#include "shading_functions.hlsli"