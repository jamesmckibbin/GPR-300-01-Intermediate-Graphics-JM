struct VS_INPUT
{
    float4 pos : POSITION;
    float3 norm : NORMALS;
    float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 worldPos : WORLDPOSITION;
    float3 norm : NORMALS;
    float2 texCoord : TEXCOORD;
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

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = mul(lMat * mul(wMat, vpMat), float4(input.pos.xyz, 1.0f));
    return output;
}