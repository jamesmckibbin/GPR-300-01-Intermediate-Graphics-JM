struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 worldPos : WORLDPOSITION;
    float3 norm : NORMALS;
    float2 texCoord : TEXCOORD;
};

float4 main(VS_OUTPUT input) : SV_TARGET
{
    return float4(1.0f, 1.0f, 1.0f, 1.0f);
}