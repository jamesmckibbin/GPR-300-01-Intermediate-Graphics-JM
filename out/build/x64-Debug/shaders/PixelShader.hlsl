Texture2D t1 : register(t0);
Texture2D t2 : register(t2);
SamplerState s1 : register(s0);

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
    float4 lDir;
    float4 camPos;
    float3 dsa;
    uint postP;
};

float ShadowCalculation(float4 lightSpacePos, float bias)
{
    float shadowMapDepth = t2.Sample(s1, lightSpacePos.xy).r;
    float worldDepth = lightSpacePos.z;
    float shadow = worldDepth + bias < shadowMapDepth ? 1.0f : 0.0f;
    return shadow;
}

float4 main(PS_INPUT input) : SV_TARGET
{ 
    float3 _LightColor = float3(1.0f, 1.0f, 1.0f);
    float3 _AmbientColor = float3(0.3f, 0.4f, 0.46f);
    
    float3 normal = normalize(input.norm);
    float diffuseFactor = mul(0.5f, saturate(max(dot(normal, saturate(lDir.xyz)), 0.0f)));
    float3 toEye = normalize(camPos.xyz - input.worldPos.xyz);
    float3 halfway = normalize(saturate(lDir.xyz) + toEye);
    float specularFactor = pow(max(dot(normal, halfway), 0.0f), 128);
    float bias = max(mul(0.05, (1.0 - dot(normal, lDir.xyz))), 0.005);
    float shadow = ShadowCalculation(input.fragPosLightSpace, bias);
    
    float3 lightColor = mul(saturate(mul(dsa.x, diffuseFactor) + mul(dsa.y, specularFactor)), _LightColor);
    lightColor += mul(_AmbientColor + (1.0f - shadow), dsa.z);
    float3 objectColor = t1.Sample(s1, input.texCoord).rgb;
    float3 passColor = saturate(objectColor * lightColor);
    
    return float4(passColor, 1.0f);
}