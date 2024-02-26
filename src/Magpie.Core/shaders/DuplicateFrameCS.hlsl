// 无需同步
RWBuffer<uint> result : register(u0);

Texture2D tex1 : register(t0);
Texture2D tex2 : register(t1);

SamplerState sam : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
	if (result[0]) {
		return;
	}
	
	const uint2 gxy = (gid.xy << 4) + (tid.xy << 1);
	
	// 不知为何这比通过 cbuffer 传入更快
	uint width, height;
	tex1.GetDimensions(width, height);
	const float2 pos = (gxy + 1) / float2(width, height);
	
	if (any(tex1.GatherRed(sam, pos) != tex2.GatherRed(sam, pos))) {
		result[0] = 1u;
		return;
	}
	
	if (any(tex1.GatherGreen(sam, pos) != tex2.GatherGreen(sam, pos))) {
		result[0] = 1u;
		return;
	}
	
	if (any(tex1.GatherBlue(sam, pos) != tex2.GatherBlue(sam, pos))) {
		result[0] = 1u;
	}
}
