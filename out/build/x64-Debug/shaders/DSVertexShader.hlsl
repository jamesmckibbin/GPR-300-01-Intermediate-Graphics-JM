struct VS_INPUT
{
    float4 pos : POSITION;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
};

cbuffer ConstantBuffer : register(b0)
{
    float4x4 wMat;
    float4x4 vpMat;
    float4x4 lMat;
    float4 lDir;
    float4 camPos;
    float3 dsa;
    uint postP;
};

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = mul(input.pos, mul(wMat, lMat));
    return output;
}