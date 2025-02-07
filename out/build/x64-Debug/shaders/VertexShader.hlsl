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
    float4 camPos;
    float3 dsa;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.worldPos = (float3)mul(wMat, input.pos);
    output.pos = mul(input.pos, mul(wMat, vpMat));
    output.norm = mul(transpose((float3x3)wMat), input.norm);
    output.texCoord = input.texCoord;
    return output;
}