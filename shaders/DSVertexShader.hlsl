struct VS_INPUT
{
    float4 pos : SV_POSITION;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 wMat;
    float4x4 vpMat;
    float4x4 lMat;
    float4 lPos;
    float4 camPos;
    float3 dsa;
    uint postP;
};

float4 main(VS_INPUT input)
{
    return mul(lMat * (wMat * vpMat), input.pos);
}