#pragma pack_matrix(row_major)

struct PS_In
{
	float4 clipPos : SV_POSITION;
	float4 worldPos : WORLDPOSITION;
	float4 eyePos : EYEPOSITION;
	float4 eyeNormal : EYENORMAL;
	float4 color : COLOR;
};

struct PS_Out
{
	float4 color : SV_TARGET0;
	float4 viewPos : SV_TARGET1;
	float4 viewNormal : SV_TARGET2;
};

PS_Out main(PS_In pIn)
{
	PS_Out pOut;
	pOut.color = float4(pIn.color.rgb, 1.0);
	pOut.viewPos = pIn.eyePos;
	pOut.viewNormal = pIn.eyeNormal;
	return pOut;
}