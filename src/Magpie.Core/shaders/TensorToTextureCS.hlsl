Buffer<min16float> tensor : register(t0);
RWTexture2D<min16float4> tex : register(u0);

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
	const uint2 gxy = (gid.xy << 3) + tid.xy;

	uint width, height;
	tex.GetDimensions(width, height);

	const uint planeStride = width * height;
	const uint idx = gxy.y * width + gxy.x;
	min16float3 color = { tensor[idx], tensor[planeStride + idx], tensor[planeStride * 2 + idx] };
	tex[gxy] = min16float4(color, 1);
}
