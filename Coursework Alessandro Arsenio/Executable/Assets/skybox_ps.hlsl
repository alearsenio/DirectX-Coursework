// Light pixel shader
// Calculate diffuse lighting for a single directional light(also texturing)

TextureCube EnvironmentMap : register(t0);
SamplerState LinearSampler : register(s0);


struct InputType
{
    float4 position : SV_POSITION;
    float2 tex : TEXCOORD0;
    float3 normal : NORMAL;
	float3 position3D : TEXCOORD2;
};

float4 main(InputType input) : SV_TARGET
{ 
    float4	color;

	// Sample the pixel color from the  enviromental map using the sampler at this texture coordinate location.
	color = EnvironmentMap.Sample(LinearSampler, normalize(input.position3D));

    return color;
}