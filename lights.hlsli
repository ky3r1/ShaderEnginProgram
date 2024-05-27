//点光源
struct point_lights
{
    float4 position;
    float4 color;
    float range;
    float3 dummy;
};

//スポットライト
struct spot_lights
{
    float4 position;
    float4 direction;
    float4 color;
    float range;
    float innerCorn;
    float outerCorn;
    float dummy;
};