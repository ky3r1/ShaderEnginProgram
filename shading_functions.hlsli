//-------------------------
//ランバート拡散反射計算関数
//-------------------------
// N: 法線(正規化済み)
// L: 入射ベクトル(正規化済み)
// C: 入射光(色・強さ)
// K: 反射率
float3 CalcLambert(float3 N,float3 L,float3 C,float3 K)
{
    float power = saturate(dot(N, -L));
    return C * power * K;
}

//-------------------------
//フォンの鏡面反射計算関数
//-------------------------
// N: 法線(正規化済み)
// L: 入射ベクトル(正規化済み)
// E: 視線ベクトル(正規化済み)
// C: 入射光(色・強さ)
// K: 反射率
float3 CalcPhongSpecluar(float3 N,float3 L,float3 E,float3 C,float3 K)
{
    float3 R = reflect(L, N);
    float power = max(dot(-E, R), 0);
    power = pow(power, 128);
    return C * power * K;
}

//-------------------------
//ハーフランバート拡散反射計算関数
//-------------------------
// N: 法線(正規化済み)
// L: 入射ベクトル(正規化済み)
// C: 入射光(色・強さ)
// K: 反射率
float3 CalcHalfLambert(float3 N,float3 L,float3 C,float3 K)
{
    float D = saturate(dot(N, -L) * 0.5f + 0.5f);
    return C * D * K;
}

//-------------------------
//リムライト
//-------------------------
// N: 法線(正規化済み)
// E: 視線ベクトル(正規化済み)
// L: 入射ベクトル(正規化済み)
// C: 入射光(色・強さ)
// RimPower: リムライトの強さ(初期値は要設定)
float CalcRimLight(float3 N,float3 E,float3 L,float3 C, float RimPower=20.0f)
{
    float rim = 1.0f - saturate(dot(N, -E));
    return C * pow(rim, RimPower) * saturate(dot(L, -E));
}

//-------------------------
//ランプシェーディング
//-------------------------
// tex: ランプシェーディング用テクスチャ
// samp: ランプシェーディング用サンプラステート
// N: 法線(正規化済み)
// L: 入射ベクトル(正規化済み)
// C: 入射光(色・強さ)
// K: 反射率
float CalcRampShading(Texture2D tex,SamplerState samp,float3 N,float3 L,float3 C,float3 K)
{
    float D = saturate(dot(N, -L) * 0.5f + 0.5f);
    float Ramp = tex.Sample(samp, float2(D, 0.5f));
    return C * Ramp * K.rgb;
}

//-------------------------
//球体環境マッピング
//-------------------------
// tex: ランプシェーディング用テクスチャ
// samp: ランプシェーディング用サンプラステート
// color: 現在のピクセル色
// N: 法線(正規化済み)
// C: 入射光(色・強さ)
// value: 適応率
float3 CalcSphereEnvironment(Texture2D tex,SamplerState samp,in float3 color,float3 N,float3 C,float value)
{
    float3 R = reflect(C, N);
    float2 texcoord = R.xy * 0.5f + 0.5f;
    return lerp(color.rgb, tex.Sample(samp, texcoord).rgb, value);
}

//-------------------------
//半球ライティング
//-------------------------
// normal: 法線(正規化済み)
// up: 上方向(片方)
// sky_color: 空(上)色
// ground_color: 地面(下)色
// hemisphere_weight: 重み
float3 CalcHemiSphereLight(float3 normal, float3 up, float3 sky_color, float3 ground_color, float4 hemisphere_weight)
{
    float factor = dot(normal, up) * 0.5f + 0.5f;
    return lerp(ground_color, sky_color, factor) * hemisphere_weight.x;
}

//-------------------------
//フォグ
//-------------------------
// color: 現在のピクセルの色
// fog_color: フォグの色
// fog_range: フォグの範囲情報
// eye_lange: 視点からの距離
float4 CalcFog(in float4 color,float4 fog_color,float2 fog_range,float eye_length)
{
    float fogAlpha = saturate((eye_length - fog_range.x) / (fog_range.y - fog_range.x));
    return lerp(color, fog_color, fogAlpha);

}