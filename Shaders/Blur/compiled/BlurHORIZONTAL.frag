#pragma pack_matrix(row_major)

[[vk::binding(0, 3)]]
Texture2D Texture : register(t3);
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
#define KERNEL_SIZE int(SizeData.z)

[[vk::binding(0, 6)]]
cbuffer MaterialBuffer
 : register(b2){
	float4 SizeData;
	float4 BlurKernel[3];
};

float4 ComputeNeighbors(float2 uv, float2 offset)
{
	float4 col = float4(0, 0, 0, 0);
	float2 screenOffset = INV_SIZE;
	
	col += Texture.Sample(Sampler, uv + screenOffset * (offset * +1)) * BlurKernel[0].y;
	col += Texture.Sample(Sampler, uv + screenOffset * (offset * -1)) * BlurKernel[0].y;	
	col += Texture.Sample(Sampler, uv + screenOffset * (offset * +2)) * BlurKernel[0].z;
	col += Texture.Sample(Sampler, uv + screenOffset * (offset * -2)) * BlurKernel[0].z;		
	col += Texture.Sample(Sampler, uv + screenOffset * (offset * +3)) * BlurKernel[0].w;
	col += Texture.Sample(Sampler, uv + screenOffset * (offset * -3)) * BlurKernel[0].w;
	
	if(KERNEL_SIZE > 1)
	{
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * +4)) * BlurKernel[1].x;
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * -4)) * BlurKernel[1].x;	
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * +5)) * BlurKernel[1].y;
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * -5)) * BlurKernel[1].y;		
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * +6)) * BlurKernel[1].z;
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * -6)) * BlurKernel[1].z;		
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * +7)) * BlurKernel[1].w;
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * -7)) * BlurKernel[1].w;
	}	
	
	if(KERNEL_SIZE > 2)
	{
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * +8)) * BlurKernel[2].x;
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * -8)) * BlurKernel[2].x;		
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * +9)) * BlurKernel[2].y;
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * -9)) * BlurKernel[2].y;	
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * +10)) * BlurKernel[2].z;
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * -10)) * BlurKernel[2].z;		
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * +11)) * BlurKernel[2].w;
		col += Texture.Sample(Sampler, uv + screenOffset * (offset * -11)) * BlurKernel[2].w;
	}	
	
	return col;
}

float4 main(PS_In pIn) : SV_TARGET
{
	float2 uv = pIn.texCoord;

	float4 color = float4(0.0, 0.0, 0.0, 0.0);
	

	color += Texture.Sample(Sampler, uv) * BlurKernel[0].x;
	color += ComputeNeighbors(uv, float2(1, 0));		

	//color = Texture.Sample(Sampler, uv);


	
	return color;
};

