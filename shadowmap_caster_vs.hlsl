#include "static_mesh.hlsli"

float4 main(float4 position:POSITION,float4 normal:NORMAL,float2 texcoord:TEXCOORD):SV_POSITION
{
    return mul(position, mul(world, view_projection));
}