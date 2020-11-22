#include "CameraBuffer.hlsl"
#include "ObjectBuffer.hlsl"

struct VS_In
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
};

//x=oneOverTotalTerrainResolution,y=TerrainPatchResolution,z=NumTextures
cbuffer MaterialBuffer
{
	float4 TerrainData; 
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 position : POSITION;	
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 texCoords : TEXCOORD;
};

PS_In main(
	VS_In vIn,
	uint vertexIndex : SV_VERTEXID)
{
	PS_In pIn;
	pIn.clipPos = mul(mul(mul(vIn.position, WorldMatrix), ViewMatrix), ProjectionMatrix);
	pIn.position = mul(vIn.position, WorldMatrix);
	
	pIn.normal = mul(vIn.normal, NormalMatrix);
#if 0
	pIn.tangent = mul(vIn.tangent, NormalMatrix).xyz;	
#else
	pIn.tangent = mul(vIn.normal.yxzw * float4(1, -1, 1, 1), NormalMatrix);
#endif

	//float4 tmp = pIn.tangent;
	//pIn.tangent = pIn.normal;
	//pIn.normal = tmp;
	
	pIn.texCoords.xy = vIn.position.xz * TerrainData.x;
	uint patchSize = uint(TerrainData.y);
	uint col = vertexIndex % patchSize;
	uint row = vertexIndex / patchSize;	
	pIn.texCoords.zw = float2(col, row) / (TerrainData.y - 1);
	
	return pIn;
}