#include "CameraBuffer.hlsl"
#include "ObjectBuffer.hlsl"
#ifdef SKINNED
#include "SkinnedBoneBuffer.hlsl"
#define WORLD_MATRIX SkinnedWorldMatrix
#else
#define WORLD_MATRIX WorldMatrix
#endif


struct VS_In
{
	float4 position : POSITION;
	float4 texCoord : TEXCOORD;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;	
#ifdef SKINNED
	float4 bones : BONES;
	float4 weights : WEIGHTS;
#endif	
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
#if !defined(DEPTH) || (defined(DEPTH) && defined(ALPHA_TEST))	
	float4 texCoord : TEXCOORD;	
#endif
#ifndef DEPTH	
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
#endif	
};

PS_In main(VS_In vIn)
{
	PS_In pIn;
	
#ifdef SKINNED
	int4 bones = int4(vIn.bones);
	float4 weights = vIn.weights;
	float4x4 SkinnedWorldMatrix = SkinnedBones[bones.x] * weights.x;
	SkinnedWorldMatrix += SkinnedBones[bones.y] * weights.y;
	SkinnedWorldMatrix += SkinnedBones[bones.z] * weights.z;
	SkinnedWorldMatrix += SkinnedBones[bones.w] * weights.w;
	SkinnedWorldMatrix = mul(SkinnedWorldMatrix, WorldMatrix);
#endif	
	
	pIn.clipPos = mul(mul(vIn.position, WORLD_MATRIX), ViewProjectionMatrix);
	
#if !defined(DEPTH) || (defined(DEPTH) && defined(ALPHA_TEST))	
	pIn.texCoord = vIn.texCoord;
#endif
	
#ifndef DEPTH	
	pIn.position = mul(vIn.position, WORLD_MATRIX);
	pIn.normal = mul(float4(vIn.normal.xyz, 0.0), WORLD_MATRIX);
	pIn.tangent = mul(float4(vIn.tangent.xyz, 0.0), WORLD_MATRIX);
	
#if 1
	pIn.position = mul(pIn.position, ViewMatrix);
	pIn.normal = mul(pIn.normal, ViewMatrix);
	pIn.tangent = mul(pIn.tangent, ViewMatrix);
#endif
#endif
	
	return pIn;
}