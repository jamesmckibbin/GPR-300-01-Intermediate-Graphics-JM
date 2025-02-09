Texture2D t1 : register(t1);
SamplerState s1 : register(s0);

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    return float4(t1.Sample(s1, input.texCoord).rgb, 1.0f);
}