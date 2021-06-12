// Terrain pixel shader

Texture2D shaderTextureSand : register(t0);
Texture2D shaderTextureRock : register(t1);
Texture2D shaderTextureGrass : register(t2);
SamplerState SampleType : register(s0);


cbuffer LightBuffer : register(b0)
{
    float4 ambientColor;
    float4 diffuseColor;
    float3 lightPosition;
    float padding;
};

struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
    float3 position3D : TEXCOORD2;
};

////////////////////////////////////////////////////

// Description : HLSL convertion of the array and textureless GLSL 2D/3D/4D simplex 
//               noise functions made by Ian McEwan.
//      Author : Ian McEwan, Ashima Arts.
//  Maintainer : stegu
//     Lastmod : 20201014 (stegu)
//     License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//               Distributed under the MIT License. See LICENSE file.
//               https://github.com/ashima/webgl-noise
//               https://github.com/stegu/webgl-noise
// 
float3 mod289(float3 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float4 mod289(float4 x) {
    return x - floor(x * (1.0 / 289.0)) * 289.0;
}

float4 permute(float4 x) {
    return mod289(((x * 34.0) + 1.0) * x);
}

float4 taylorInvSqrt(float4 r)
{
    return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(float3 v)
{
    const float2  C = float2(1.0 / 6.0, 1.0 / 3.0);
    const float4  D = float4(0.0, 0.5, 1.0, 2.0);

    // First corner
    float3 i = floor(v + dot(v, C.yyy));
    float3 x0 = v - i + dot(i, C.xxx);

    // Other corners
    float3 g = step(x0.yzx, x0.xyz);
    float3 l = 1.0 - g;
    float3 i1 = min(g.xyz, l.zxy);
    float3 i2 = max(g.xyz, l.zxy);

    //   x0 = x0 - 0.0 + 0.0 * C.xxx;
    //   x1 = x0 - i1  + 1.0 * C.xxx;
    //   x2 = x0 - i2  + 2.0 * C.xxx;
    //   x3 = x0 - 1.0 + 3.0 * C.xxx;
    float3 x1 = x0 - i1 + C.xxx;
    float3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
    float3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y

  // Permutations
    i = mod289(i);
    float4 p = permute(permute(permute(
        i.z + float4(0.0, i1.z, i2.z, 1.0))
        + i.y + float4(0.0, i1.y, i2.y, 1.0))
        + i.x + float4(0.0, i1.x, i2.x, 1.0));

    // Gradients: 7x7 points over a square, mapped onto an octahedron.
    // The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
    float n_ = 0.142857142857; // 1.0/7.0
    float3  ns = n_ * D.wyz - D.xzx;

    float4 j = p - 49.0 * floor(p * ns.z * ns.z);  //  mod(p,7*7)

    float4 x_ = floor(j * ns.z);
    float4 y_ = floor(j - 7.0 * x_);    // mod(j,N)

    float4 x = x_ * ns.x + ns.yyyy;
    float4 y = y_ * ns.x + ns.yyyy;
    float4 h = 1.0 - abs(x) - abs(y);

    float4 b0 = float4(x.xy, y.xy);
    float4 b1 = float4(x.zw, y.zw);

    //float4 s0 = float4(lessThan(b0,0.0))*2.0 - 1.0;
    //float4 s1 = float4(lessThan(b1,0.0))*2.0 - 1.0;
    float4 s0 = floor(b0) * 2.0 + 1.0;
    float4 s1 = floor(b1) * 2.0 + 1.0;
    float4 sh = -step(h, float4(0.0, 0.0, 0.0, 0.0));

    float4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
    float4 a1 = b1.xzyw + s1.xzyw * sh.zzww;

    float3 p0 = float3(a0.xy, h.x);
    float3 p1 = float3(a0.zw, h.y);
    float3 p2 = float3(a1.xy, h.z);
    float3 p3 = float3(a1.zw, h.w);

    //Normalise gradients
    float4 norm = taylorInvSqrt(float4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
    p0 *= norm.x;
    p1 *= norm.y;
    p2 *= norm.z;
    p3 *= norm.w;

    // Mix final noise value
    float4 m = max(0.5 - float4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
    m = m * m;
    return 105.0 * dot(m * m, float4(dot(p0, x0), dot(p1, x1),
        dot(p2, x2), dot(p3, x3)));
}

float3 getNormal(float3 p, float eps) {
    float3 n;
    float intensity = 10;
    n.y = snoise(p / intensity);
    n.x = snoise(float3(p.x + eps, p.y, p.z) / intensity) - n.y;
    n.z = snoise(float3(p.x, p.y, p.z + eps) / intensity) - n.y;
    n.y = eps;
    return normalize(n);
}
///////////////////////////////////

float4 main(InputType input) : SV_TARGET
{
    float4	mainTextureColor;
    float4	sandTextureColor;
    float4	grassTextureColor;
    float3	lightDir;
    float	lightIntensity;
    float4	color;
    float3  noisePosition3D = input.position3D;

    // Invert the light direction for calculations.
    lightDir = normalize(input.position3D - lightPosition);

    // Calculate the amount of light on this pixel.
    lightIntensity = saturate(dot(input.normal, -lightDir));

    //creates a seas rigdes effect if the pixel is under the level of water
    if (input.position3D.y < 0)
    {
        //noise calculation
        float noiseScale = 20;
        float noiseIntensity = 2;
        noisePosition3D.y += padding / 2;
        noisePosition3D.z += padding / 2;
        //strech the z component of the position and apply a noise, to recreate sea ridges
        noisePosition3D.z *= 100 + snoise(noisePosition3D) / 10;
        //change the pixel brightness with noise, and multiply again with the brightness to show the sea rigdes only on the bright surfaces
        lightIntensity += (snoise(noisePosition3D / noiseScale)/ noiseIntensity)  *  (lightIntensity/2);
        //reduce light intensity the more the pixel position below the level of water
        lightIntensity += input.position3D.y/80;
    }

    // Determine the final amount of diffuse color based on the diffuse color combined with the light intensity.
    color = ambientColor + (diffuseColor * lightIntensity * 0.8); //adding ambient
    color = saturate(color);

    //define the boundaries where to change texture
    float firstBoundary = -10 + snoise(input.position3D / 10) * 10;
    float secondBoundary = 8 + snoise(input.position3D / 10) * 10;

    // Sample the pixel color from the textures using the samplers at this texture coordinate location.
    mainTextureColor = shaderTextureRock.Sample(SampleType, input.tex);
    sandTextureColor = shaderTextureSand.Sample(SampleType, input.tex);
    grassTextureColor = shaderTextureGrass.Sample(SampleType, input.tex);

    //texture the terrain with the appropiate texture for that position
    float blendDistance = 10;
    if (input.position3D.y < firstBoundary)
    {
        grassTextureColor = 0;
        if (firstBoundary - input.position3D.y < blendDistance)
        {
            sandTextureColor *= (firstBoundary - input.position3D.y)/ blendDistance;
            mainTextureColor *= 1 - ((firstBoundary - input.position3D.y)/ blendDistance);
        }
        else
        {
            mainTextureColor = 0;
        }
    }
    else if (input.position3D.y > secondBoundary)
    {
        sandTextureColor = 0;
        if (input.position3D.y  - secondBoundary < blendDistance)
        {
            grassTextureColor *= (input.position3D.y - secondBoundary) / blendDistance;
            mainTextureColor *=  1 - ((input.position3D.y - secondBoundary) / blendDistance);
        }
        else
        {
            mainTextureColor = 0;
        }
    }
    else
    {
        grassTextureColor = 0;
        sandTextureColor = 0;
    }
    color *= mainTextureColor + sandTextureColor + grassTextureColor;
    return color;
}