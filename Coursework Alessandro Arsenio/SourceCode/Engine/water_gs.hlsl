struct InputType
{
	float4 Pos : SV_POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;
	float3 position3D : TEXCOORD2;
};

struct GSOutput
{
	float4 Pos: SV_POSITION;
	float2 Tex : TEXCOORD0;
	float3 Norm : NORMAL;
	float3 position3D : TEXCOORD2;
};

cbuffer MatrixBuffer : register(b0)
{
    matrix worldMatrix;
    matrix viewMatrix;
    matrix projectionMatrix;
    float time;
};



[maxvertexcount(3)]
void main(triangle  InputType input[3], inout TriangleStream< GSOutput > TriStream)
{
    GSOutput output;
    // Calculate the face normal
    float3 faceEdgeA = input[1].position3D - input[0].position3D;
    float3 faceEdgeB = input[2].position3D - input[0].position3D;
    float3 faceNormal = normalize(cross(faceEdgeA, faceEdgeB));

    for (uint i = 0; i < 3; i++)
    {
        GSOutput element;
        element.Pos = input[i].Pos;
        element.Tex = input[i].Tex;
        //apply the face normal to every vertex normal
        element.Norm = faceNormal;
        element.position3D = input[i].position3D;
        TriStream.Append(element);

    }
    TriStream.RestartStrip();
}
