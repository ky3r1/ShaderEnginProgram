struct VS_OUT
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
    float2 texcoord : TEXCOORD;
};
cbuffer SCENE_CONSTANT_BUFFER : register(b1)
{
    row_major float4x4 view_projection;
    float4 options;
};
cbuffer SCROLL_CONSTANT_BUFFER : register(b2)
{
    float2 scroll_direction;
    float2 scroll_dummy;
};
