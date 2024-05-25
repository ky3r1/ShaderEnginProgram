//-------------------------
//�����o�[�g�g�U���ˌv�Z�֐�
//-------------------------
// N: �@��(���K���ς�)
// L: ���˃x�N�g��(���K���ς�)
// C: ���ˌ�(�F�E����)
// K: ���˗�
float3 CalcLambert(float3 N,float3 L,float3 C,float3 K)
{
    float power = saturate(dot(N, -L));
    return C * power * K;
}

//-------------------------
//�t�H���̋��ʔ��ˌv�Z�֐�
//-------------------------
// N: �@��(���K���ς�)
// L: ���˃x�N�g��(���K���ς�)
// E: �����x�N�g��(���K���ς�)
// C: ���ˌ�(�F�E����)
// K: ���˗�
float3 CalcPhongSpecluar(float3 N,float3 L,float3 E,float3 C,float3 K)
{
    float3 R = reflect(L, N);
    float power = max(dot(-E, R), 0);
    power = pow(power, 128);
    return C * power * K;
}

//-------------------------
//�n�[�t�����o�[�g�g�U���ˌv�Z�֐�
//-------------------------
// N: �@��(���K���ς�)
// L: ���˃x�N�g��(���K���ς�)
// C: ���ˌ�(�F�E����)
// K: ���˗�
float3 CalcHalfLambert(float3 N,float3 L,float3 C,float3 K)
{
    float D = saturate(dot(N, -L) * 0.5f + 0.5f);
    return C * D * K;
}

//-------------------------
//�������C�g
//-------------------------
// N: �@��(���K���ς�)
// E: ���˃x�N�g��(���K���ς�)
// L: ���˃x�N�g��(���K���ς�)
// C: ���ˌ�(�F�E����)
// RimPower: �������C�g�̋���(�����l�͗v�ݒ�)
float CalcRimLight(float3 N,float3 E,float3 L,float3 C, float RimPower=20.0f)
{
    float rim = 1.0f - saturate(dot(N, -E));
    return C * pow(rim, RimPower) * saturate(dot(L, -E));
}