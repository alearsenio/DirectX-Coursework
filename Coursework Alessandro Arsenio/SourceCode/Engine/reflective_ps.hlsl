// Light pixel shader
// Calculate diffuse lighting for a single directional light(also texturing)

Texture2D Texture : register(t0);
TextureCube EnvironmentMap : register(t1);
SamplerState SampleType : register(s0);


cbuffer LightBuffer : register(b0)
{
	float4 ambientColor;
	float4 diffuseColor;
	float3 lightPosition;
	float padding;
};

cbuffer cameraBuffer : register(b1)
{
	float3 cameraPosition;
	float cameraPadding;
};

struct InputType
{
	float4 position : SV_POSITION;
	float2 tex : TEXCOORD0;
	float3 normal : NORMAL;
	float3 position3D : TEXCOORD2;
};

float4 main(InputType input) : SV_TARGET
{
	float4	textureColor;
	float4	EnviromentalMapColor;
	float4	blendTextureColor;
	float3	lightDir;
	float	lightIntensity;
	float4	color;

	// Invert the light direction for calculations.
	lightDir = normalize(input.position3D - lightPosition);
	float3 toEye = normalize(cameraPosition - input.position3D);
	// Calculate the amount of light on this pixel.
	lightIntensity = saturate(dot(input.normal, -lightDir));

	// Determine the final amount of diffuse color based on the diffuse color combined with the light intensity.
	color = ambientColor + (diffuseColor * lightIntensity); //adding ambient
	color = saturate(color);

	//calculate the reflection vector to determinate the tex of the enviromental map
	float3 incident = -toEye;
	float3 reflectionVector = reflect(incident, input.normal);
	EnviromentalMapColor = EnvironmentMap.Sample(SampleType, reflectionVector);

	//find the color of the object texture
	textureColor = Texture.Sample(SampleType, input.tex);

	blendTextureColor = textureColor * 0.70f + EnviromentalMapColor * 0.30f;
	color = color * blendTextureColor;
	return color;
}