#include "CameraBuffer.hlsl"
#include "ObjectBuffer.hlsl"

Texture2D HeightMap;

struct VS_In
{
	float4 position : POSITION;
};

#define TERRAIN_RESOLUTION (513.0)

//x=oneOverTotalTerrainResolution,y=TerrainPatchResolution,z=NumTextures
cbuffer MaterialBuffer
{
	float4 TerrainData; 
	float4 TerrainBox;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 position : POSITION;
	float4 texCoord : TEXCOORD;
	float4 debugColor : COLOR;
};

PS_In main(VS_In vIn)
{
	float4 position = mul(float4(vIn.position.x, 0.0, vIn.position.y, 1.0), WorldMatrix);	
	
	float halfRes = TERRAIN_RESOLUTION * 0.5;
	
	position.x = clamp(position.x, -halfRes, +halfRes);
	position.z = clamp(position.z, -halfRes, +halfRes);
	
	float u = (position.x + halfRes) / TERRAIN_RESOLUTION;
	float v = (position.z + halfRes) / TERRAIN_RESOLUTION;
	
	int2 iCoords = int2(float2(u, v) * (TERRAIN_RESOLUTION - 1.0));
	position.y = HeightMap[iCoords].x;	
	//position.y = 0.0;
	float3 normal = float3(0, 1, 0);	

	PS_In pIn;
	pIn.clipPos = mul(mul(position, ViewMatrix), ProjectionMatrix);
	pIn.position = position;
	pIn.texCoord = float4(u, v, 0.0, 0.0);
	pIn.debugColor = float4(vIn.position.w, 1.0 - vIn.position.w, 0, 0);// float4(u, v, texIndices);\
	
	if(u >= 0.0)
	{
		pIn.debugColor.xy = float2(1,0);
	}
	else
	{
		pIn.debugColor.xy = float2(0,1);
	}
	
	pIn.debugColor.xy = float2(u, v);
	
	
	return pIn;
}