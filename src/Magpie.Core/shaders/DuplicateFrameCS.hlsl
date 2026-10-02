cbuffer Constants : register(b0) {
	uint4 dirtyRect;
	float2 texPt;
	uint target;
	uint resultOffset;
};

// 无需同步
RWBuffer<uint> result : register(u0);

Texture2D tex1 : register(t0);
Texture2D tex2 : register(t1);

SamplerState sam : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
	if (result[resultOffset] == target) {
		return;
	}
	
	const uint2 gxy = dirtyRect.xy + (gid.xy << 4) + (tid.xy << 1);
	
#ifndef MP_NO_BOUNDS_CHECKING
	if (gxy.x >= dirtyRect.z || gxy.y >= dirtyRect.w) {
		return;
	}
#endif
	
	const float2 pos = (gxy + 1) * texPt;

#ifdef MP_NO_BOUNDS_CHECKING
	if (any(tex1.GatherRed(sam, pos) != tex2.GatherRed(sam, pos))) {
		result[resultOffset] = target;
		return;
	}
	
	if (any(tex1.GatherGreen(sam, pos) != tex2.GatherGreen(sam, pos))) {
		result[resultOffset] = target;
		return;
	}
	
	if (any(tex1.GatherBlue(sam, pos) != tex2.GatherBlue(sam, pos))) {
		result[resultOffset] = target;
	}
#else
	// w z
	// x y
	float4 mask = 1.0;
	if (gxy.x + 1 >= dirtyRect.z) {
		mask.yz = 0.0;
	}
	if (gxy.y + 1 >= dirtyRect.w) {
		mask.xy = 0.0;
	}
	
	float4 c1 = tex1.GatherRed(sam, pos);
	float4 c2 = tex2.GatherRed(sam, pos);
	// 乘以遮罩过滤掉无效区域
	if (any((c1 != c2) * mask)) {
		result[resultOffset] = target;
		return;
	}
	
	c1 = tex1.GatherGreen(sam, pos);
	c2 = tex2.GatherGreen(sam, pos);
	if (any((c1 != c2) * mask)) {
		result[resultOffset] = target;
		return;
	}
	
	c1 = tex1.GatherBlue(sam, pos);
	c2 = tex2.GatherBlue(sam, pos);
	if (any((c1 != c2) * mask)) {
		result[resultOffset] = target;
		return;
	}
#endif
}
