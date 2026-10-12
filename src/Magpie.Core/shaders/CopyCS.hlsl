#include "Common.hlsli"

Texture2D<float4> inputTex : register(t0);
RWTexture2D<float4> outputTex : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
	uint2 gxy = (gid.xy << 4) + tid.xy;
    outputTex[gxy] = inputTex[gxy];
	
	gxy.x += 8u;
    outputTex[gxy] = inputTex[gxy];
	
	gxy.y += 8u;
    outputTex[gxy] = inputTex[gxy];
	
	gxy.x -= 8u;
    outputTex[gxy] = inputTex[gxy];
}
