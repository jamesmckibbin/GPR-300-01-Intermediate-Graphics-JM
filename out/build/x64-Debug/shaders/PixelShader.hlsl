Texture2D t1 : register(t0);
Texture2D t2 : register(t2);
SamplerState s1 : register(s0);

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float3 worldPos : WORLDPOSITION;
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

float ShadowCalculation(float4 lightSpacePos, float bias)
{
    float3 projCoords = lightSpacePos.xyz / lightSpacePos.w;
    projCoords = mul(projCoords, 0.5f) + 0.5f;
    float closestDepth = t2.Sample(s1, projCoords.xy).r;
    float currentDepth = projCoords.z;
    float shadow = currentDepth - bias > closestDepth ? 1.0f : 0.0f;
    return shadow;
}

float4 main(PS_INPUT input) : SV_TARGET
{ 
    float3 _LightColor = float3(1.0f, 1.0f, 1.0f);
    float3 _AmbientColor = float3(0.3f, 0.4f, 0.46f);
    
    float3 normal = normalize(input.norm);
    float diffuseFactor = mul(0.5f, max(dot(normal, clamp(lPos.xyz, 0.0f, 1.0f)), 0.0f));
    float3 toEye = normalize(camPos.xyz - input.worldPos);
    float3 h = normalize(lPos.xyz + toEye);
    float specularFactor = pow(max(dot(normal, h), 0.0f), 128);
    
    float3 toLight = normalize(lPos.xyz - input.worldPos);
    float bias = max(mul(0.05, (1.0 - dot(normal, toLight))), 0.005);
    float shadow = ShadowCalculation(input.fragPosLightSpace, bias);
    
    float3 lightColor = mul(mul(dsa.x, diffuseFactor) + mul(dsa.y, specularFactor), _LightColor);
    lightColor += mul(_AmbientColor, dsa.z) + (1.0f - shadow);
    float3 objectColor = t1.Sample(s1, input.texCoord).rgb;
    float3 passColor = objectColor * lightColor;
    
    return float4(passColor, 1.0f);
}