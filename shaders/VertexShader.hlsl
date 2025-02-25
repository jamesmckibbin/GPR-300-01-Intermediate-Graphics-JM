struct VS_INPUT
{
    float4 pos : POSITION;
    float3 norm : NORMALS;
    float2 texCoord : TEXCOORD;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 worldPos : WORLDPOSITION;
    float4 fragPosLightSpace : LIGHTSPACE;
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

PS_INPUT main(VS_INPUT input)
{
    PS_INPUT output;
    output.worldPos = mul(wMat, input.pos);
    output.fragPosLightSpace = mul(input.pos, mul(wMat, lMat));
    output.pos = mul(input.pos, mul(wMat, vpMat));
    output.norm = mul(transpose((float3x3)wMat), input.norm);
    output.texCoord = input.texCoord;
    return output;
}