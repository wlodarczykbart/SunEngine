#include "CameraBuffer.hlsl"
#include "ObjectBuffer.hlsl"

struct GS_In
{
	float4 position_scale : POSITION;
	float4 data : DATA;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 projCoord : PROJCOORD;
	float2 texCoord : TEXCOORD;
	float2 dropCoord : DROPCOORD;
};

cbuffer MaterialBuffer 
{
	float4 AtlasDropCoords[64];
}

[maxvertexcount(4)]
void main(point GS_In gIn[1], inout TriangleStream<PS_In> pStream)
{
	PS_In pIn;

	float4 quadPoints[4];
	quadPoints[0] = float4(+1, -1, 0, 0);
	quadPoints[1] = float4(+1, +1, 0, 0);
	quadPoints[2] = float4(-1, -1, 0, 0);
	quadPoints[3] = float4(-1, +1, 0, 0);	
	
	float4 offset = mul(float4(gIn[0].position_scale.xyz, 1.0), WorldMatrix);
	float scale = gIn[0].position_scale.w;
	
	int atlasIdx = int(gIn[0].data.x);
	float4 atlasCoord = AtlasDropCoords[atlasIdx];
	
	[unroll]
	for(int i = 0; i < 4; i++)
	{
		pIn.texCoord.xy = quadPoints[i].xy * 0.5 + 0.5;
		pIn.texCoord.y = 1.0 - pIn.texCoord.y;

		pIn.dropCoord.x = lerp(atlasCoord.x, atlasCoord.z, pIn.texCoord.x);
		pIn.dropCoord.y = lerp(atlasCoord.y, atlasCoord.w, pIn.texCoord.y);	
		
		float4 position =  mul(quadPoints[i], InvViewMatrix) * scale + offset;	
		pIn.clipPos = mul(mul(position, ViewMatrix), ProjectionMatrix);
		pIn.projCoord = pIn.clipPos;
		
		pStream.Append(pIn);
	}			
}