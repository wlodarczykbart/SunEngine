#define VIEWPORT CameraData[0]
#define CAMERA_NEAR_Z CameraData[1].x
#define CAMERA_FAR_Z CameraData[1].y
#define SCREEN_SIZE VIEWPORT.zw

cbuffer CameraBuffer
{
	float4x4 ViewMatrix;
	float4x4 ProjectionMatrix;
	float4x4 ViewProjectionMatrix;
	float4x4 InvViewMatrix;
	float4x4 InvProjectionMatrix;
	float4x4 InvViewProjectionMatrix;
	float4x4 CameraData;
};

float2 GetScreenTexCoord(float2 screenPosition)
{
	return (screenPosition - VIEWPORT.xy) / VIEWPORT.zw;
}

float GetEyeDepth(Texture2D tex, int2 texCoord)
{
    //proj.z = eye.x * 0 + eye.y * 0 + eye.z * proj22 + proj32
    //proj.z = eye.z * proj22 + proj32
	//
    //proj.w = eye.x * 0 + eye.y * 0 + eye.z * proj23(should be -1/1 for perspective projection) + 0
    //proj.w = +-eye.z
    //depth = (eye.z * proj22 + proj32) / +-eye.z
    //depth * +-eye.z = eye.z * proj22 + proj32
    //depth * +-eye.z - eye.z * proj22 = proj32
    //+-eye.z(depth + proj22) = proj32
    //+-eye.z = proj32 / depth + proj22 

    float depth = tex.Load(int3(texCoord, 0)).r;
    return ProjectionMatrix[2][3] * ProjectionMatrix[3][2] / (depth + ProjectionMatrix[2][2]);
}