#pragma pack_matrix(row_major)

[[vk::binding(0, 7)]]
cbuffer TextureTransformBuffer
 : register(b6){
	float4 TextureTransforms[32];
};

float4 SampleTextureArray(Texture2DArray texArray, SamplerState texArraySampler, float3 texArrayCoord)
{
	float4 transform = TextureTransforms[int(texArrayCoord.z)];
	texArrayCoord.xy = texArrayCoord.xy * transform.xy + transform.zw;
	return texArray.Sample(texArraySampler, texArrayCoord);
}


[[vk::binding(0, 3)]]
Texture2D DropTexture : register(t3);
[[vk::binding(0, 4)]]
SamplerState DropSampler : register(s3);

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 projCoord : PROJCOORD;
	float2 texCoord : TEXCOORD;
	float2 dropCoord : DROPCOORD;
};

float4 main(PS_In pIn) : SV_TARGET
{
	float4 fragColor =  float4(0,0,0,1);

	float2 texCoord = pIn.texCoord;
	float rounding = min(length(texCoord * 2.0 - 1.0), 1.0);
	
	float2 dropCoord = pIn.dropCoord;
	float4 dropColor = DropTexture.Sample(DropSampler, dropCoord * TextureTransforms[3].xy + TextureTransforms[3].zw);
	
	float dropTransparency = dropColor.a;
		
#ifdef SHADER_PASS_DEFAULT
	float4 proj = pIn.projCoord / pIn.projCoord.w;
	proj = proj * 0.5 + 0.5;
	proj.y = 1.0 - proj.y;		
	float2 projCoord = proj.xy;
	
	float dropRefraction = rounding * 0.025;		
	//dropRefraction = (1.0 - dropColor.a) * 0.5;
	
	projCoord += dropRefraction;
	projCoord.x = clamp(projCoord.x, 0.0, 1.0);
	projCoord.y = clamp(projCoord.y, 0.0, 1.0);
	fragColor = SampleScene(projCoord);
	fragColor.rgb = lerp(fragColor.rgb, float3(1,1,1) * 0.75, dropTransparency * 0.1);
	
	//fragColor.rgb = float3(texCoord.x, texCoord.y, 0.0);	
	//fragColor.rgb = float3(dropCoord.x, dropCoord.y, 0.0);
	//fragColor.rgb = dropColor.rgb;
	//fragColor.rgb = float3(dropRefraction, dropRefraction, dropRefraction);
#endif	
	
#ifdef SHADER_PASS_OFFSCREEN
	fragColor.rgb = float3(1,1,1);
#endif	
	
	fragColor.a = dropTransparency;
	return fragColor;
}


