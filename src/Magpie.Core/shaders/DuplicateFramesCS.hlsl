RWBuffer<uint> result : register(u0);

Texture2D tex1 : register(t0);
Texture2D tex2 : register(t1);

uint Bfe(uint src, uint off, uint bits) {
	uint mask = (1u << bits) - 1;
	return (src >> off) & mask;
}

uint BfiM(uint src, uint ins, uint bits) {
	uint mask = (1u << bits) - 1;
	return (ins & mask) | (src & (~mask));
}

uint2 Rmp8x8(uint a) {
	return uint2(Bfe(a, 1u, 3u), BfiM(Bfe(a, 3u, 3u), a, 1u));
}

[numthreads(64, 1, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
	if (result[0]) {
		return;
	}
	
	const int3 pos = { (gid.xy << 3u) + Rmp8x8(tid.x) << 2, 0 };
	if (any(tex1.Load(pos).rgb != tex2.Load(pos, 0).rgb)) {
		result[0] = 1u;
	}
}
