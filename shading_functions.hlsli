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