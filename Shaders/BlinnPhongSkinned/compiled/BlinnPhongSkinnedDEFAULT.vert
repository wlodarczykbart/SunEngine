#pragma pack_matrix(row_major)
#define APPLY_LIGHTING 1
#define APPLY_FOG 1

[[vk::binding(0, 0)]]
cbuffer CameraBuffer
 : register(b0){
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjMatrix;
};

[[vk::binding(0, 5)]]
cbuffer ObjectBuffer
 : register(b1){
	float4x4 WorldMatrix;
	float4x4 NormalMatrix;
};

[[vk::binding(0, 8)]]
cbuffer SkinnedBoneBuffer
 : register(b8){
	float4x4 SkinnedBones[256];
};


struct VS_In
{
	float4 position : POSITION;
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float4 texCoord : TEXCOORD;
	float4 bones : BONES;
	float4 weights : WEIGHTS;
};

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 position : POSITION;	
	float4 normal : NORMAL;
	float4 tangent : TANGENT;
	float2 texCoord : TEXCOORD;
};

PS_In main(VS_In vIn)
{
	float4x4 boneMtx = SkinnedBones[int(vIn.bones.x)] * vIn.weights.x;
	boneMtx += SkinnedBones[int(vIn.bones.y)] * vIn.weights.y;
	boneMtx += SkinnedBones[int(vIn.bones.z)] * vIn.weights.z;
	boneMtx += SkinnedBones[int(vIn.bones.w)] * vIn.weights.w;

	PS_In pIn;
	float4 worldPos = mul(mul(vIn.position, boneMtx), WorldMatrix);
	pIn.clipPos = mul(mul(worldPos, ViewMatrix), ProjectionMatrix);
	pIn.position = worldPos;
	pIn.texCoord = vIn.texCoord.xy;
	
	float4 skinnedNormal = normalize(mul(vIn.normal, boneMtx));
	float4 skinnedTangent = normalize(mul(vIn.tangent, boneMtx));
	
	pIn.normal = mul(skinnedNormal, NormalMatrix);
	pIn.tangent = mul(skinnedTangent, NormalMatrix);		
	
	return pIn;
}


