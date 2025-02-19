Texture2D t1 : register(t1);
Texture2D t2 : register(t2);
SamplerState s1 : register(s0);

#define BOX_BLUR float3x3(1, 1, 1, 1, 1, 1, 1, 1, 1) * 0.1111f
#define GAUSSIAN_BLUR float3x3(1, 2, 1, 2, 4, 2, 1, 2, 1) * 0.0625f

struct VS_OUTPUT
{
    float4 pos : SV_POSITION;
    float2 texCoord : TEXCOORD;
    uint postPOption : OPTIONS;
};

// Adapted from the Shadertoy Example in the slides https://www.shadertoy.com/view/fd3Szs
float4 convolute(float2 uv, float3x3 kernel)
{
    float2 screenDim;
    t1.GetDimensions(screenDim.x, screenDim.y);
    float2 texelSize = 1.0f / screenDim;
    float3 blurColor = 0.0f;
    
    float direction[3] = { -1.0f, 0.0f, 1.0f };
    for (int x = 0; x < 3; x++)
    {
        for (int y = 0; y < 3; y++)
        {
            float2 offset = float2(direction[x], direction[y]) * texelSize;
            blurColor += t1.Sample(s1, uv + offset).rgb * kernel[x][y];
        }
    }

    return float4(blurColor, 1.0f);
}

float4 main(VS_OUTPUT input) : SV_TARGET
{
    switch (input.postPOption)
    {
        // Normal
        case 0:
            return t1.Sample(s1, input.texCoord);
            break;
        
        // Shadow Map
        case 1:
            float shadowColor = t2.Sample(s1, input.texCoord).r;
            return float4(float3(shadowColor, shadowColor, shadowColor), 1.0f);
            //break;
        
        // Box Blur
        case 2:
            return convolute(input.texCoord, BOX_BLUR);
            break;
        
        // Gaussian Blur
        case 3:
            return convolute(input.texCoord, GAUSSIAN_BLUR);
            break;
    }
    
    // Normal if nothing is selected
    return float4(t1.Sample(s1, input.texCoord).rgb, 1.0f);
}