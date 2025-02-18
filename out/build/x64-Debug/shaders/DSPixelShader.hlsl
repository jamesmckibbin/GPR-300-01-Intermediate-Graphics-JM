struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float3 worldPos : WORLDPOSITION;
    float3 norm : NORMALS;
    float2 texCoord : TEXCOORD;
};

void main(VS_OUTPUT input) : SV_TARGET
{
    
}