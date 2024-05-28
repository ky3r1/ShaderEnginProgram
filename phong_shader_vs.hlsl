#include "phong_shader.hlsli"

VS_OUT main(float4 position:POSITION,float4 normal:NORMAL,float2 texcoord:TEXCOORD)
{
    VS_OUT vout = (VS_OUT) 0;
    
    float4 world_position = mul(position, world);
    vout.position = mul(world_position, view_projection);
    vout.world_position = world_position;
    //vout.world_normal = normalize(mul(float4(normal.xyz, 0), world));
    vout.normal = normalize(mul(float4(normal.xyz, 0), world));
    vout.binormal = float3(0.0f, 1.0f, 0.001f);//仮の上ベクトル
    vout.binormal = normalize(vout.binormal);
    vout.tangent = normalize(cross(vout.binormal, vout.normal));//外積
    vout.binormal = normalize(cross(vout.normal, vout.tangent));
    vout.texcoord = texcoord;
    ////シャドウマップ用のパラメーター計算
    //{
    //    //ライトから見たNDC座標を算出
    //    float4 wvpPos = mul(position, mul(world, light_view_projection));
    //    //NDC座標からUV座標を算出する
    //    wvpPos /= wvpPos.w;
    //    wvpPos.y = -wvpPos.y;
    //    wvpPos.xy = 0.5f * wvpPos.xy + 0.5f;
    //    vout.shadow_texcoord = wvpPos.xyz;
    //}
    return vout;
}