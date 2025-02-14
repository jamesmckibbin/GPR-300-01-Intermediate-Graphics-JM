Texture2D t1 : register(t0);
SamplerState s1 : register(s0);

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

float4 main(VS_OUTPUT input) : SV_TARGET
{ 
    float3 _LightDirection = float3(1.0f, 1.0f, -1.0f);
    float3 _LightColor = float3(1.0f, 1.0f, 1.0f);
    float3 _AmbientColor = float3(0.3f, 0.4f, 0.46f);
    
    float3 normal = normalize(input.norm);
    float diffuseFactor = mul(0.5f, max(dot(normal, _LightDirection), 0.0f));
    float3 toEye = normalize((float3)camPos - input.worldPos);
    float3 h = normalize(_LightDirection + toEye);
    float specularFactor = pow(max(dot(normal, h), 0.0f), 128);
    float3 lightColor = mul(mul(dsa.x, diffuseFactor) + mul(dsa.y, specularFactor), _LightColor);
    lightColor += mul(_AmbientColor, dsa.z);
    float3 objectColor = t1.Sample(s1, input.texCoord).rgb;
    float3 passColor = objectColor * lightColor;
    return float4(passColor, 1.0f);
}