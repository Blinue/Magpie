RWBuffer<uint> result : register(u0);

Texture2D tex1 : register(t0);
Texture2D tex2 : register(t1);

SamplerState sam : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
	if (result[0]) {
		return;
	}
	
	int3 gxy = { (gid.xy << 3u) + tid.xy, 0 };
	if (any(tex1.Load(gxy).rgb != tex2.Load(gxy, 0).rgb)) {
		result[0] = 1u;
	}
}
