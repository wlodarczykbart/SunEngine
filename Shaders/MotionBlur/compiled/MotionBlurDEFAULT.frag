#pragma pack_matrix(row_major)

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


#define VEL_TOL (0.001)

[[vk::binding(0, 3)]]
Texture2D Texture : register(t3);
[[vk::binding(1, 1)]]
Texture2D DepthTexture : register(t1);
[[vk::binding(0, 4)]]
SamplerState Sampler : register(s3);

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float2 texCoord : TEXCOORD;
};

#define INV_SIZE SizeData.xy
#define INV_WIDTH SizeData.x
#define INV_HEIGHT SizeData.y
#define NUM_SAMPLES int(SizeData.z)

[[vk::binding(0, 6)]]
cbuffer MaterialBuffer
 : register(b2){
	float4x4 PrevViewProj;
	float4 SizeData;
};

float4 main(PS_In pIn) : SV_TARGET
{
	float2 uv = pIn.texCoord;
	float depth = DepthTexture.Sample(Sampler, uv).r;
		
	float4 ndcPos = float4(uv.x, uv.y, depth, 1.0) * 2.0f - 1.0f;
	
	float4 currPos = mul(ndcPos, InvProjMatrix);
	currPos /= currPos.w;
	currPos = mul(currPos, InvViewMatrix);
	
	float4 prevScreenPos = mul(currPos, PrevViewProj);
	prevScreenPos /= prevScreenPos.w;
	prevScreenPos = prevScreenPos * 0.5 + 0.5;
	
	float2 vel = (uv - prevScreenPos.xy) * 1.5;
	
	float4 color = Texture.Sample(Sampler, uv);
	
	float roundingFactor = min(length(pIn.texCoord * 2.0 - 1.0), 0.8);
	//roundingFactor = 1.0 - (pow(roundingFactor, 1.0));
	//vel *= roundingFactor;
	
	for(int i = 1; i < NUM_SAMPLES; i++)
	{
		float2 offset = vel * (float(i) / float(NUM_SAMPLES - 1) - 0.5);
		color += Texture.Sample(Sampler, uv + offset);
	}
	color /= NUM_SAMPLES;
	
	//color = float4(vel.x, vel.y, 0, 1);
	//color = float4(prevScreenPos.x, prevScreenPos.y, 0.0, 0.0);
	//color = prevScreenPos - ndcPos;
	//color = ndcPos;
	//color = float4(1,1,1,1) * roundingFactor;
	return color;
};


