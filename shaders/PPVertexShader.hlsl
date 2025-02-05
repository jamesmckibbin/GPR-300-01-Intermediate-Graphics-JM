struct VS_INPUT
{
    float4 pos : POSITION;
    float2 texCoord : TEXCOORD;
};

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
};

VS_OUTPUT main(VS_INPUT input)
{
    VS_OUTPUT output;
    output.pos = float4(input.pos.x, input.pos.y, 1.0f, 1.0f);
    output.texCoord = input.texCoord;
    return output;
}