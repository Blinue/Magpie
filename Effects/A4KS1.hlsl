cbuffer cb : register(b0) {
	float inputPtX;
	float inputPtY;
};

SamplerState sam : register(s0);

Texture2D INPUT : register(t0);
RWTexture2D<float4> OUTPUT : register(u0);


float4 A4KS1(float2 pos) {
	float2 tpos = pos - 0.5f * float2(inputPtX, inputPtY);
	const float4 sr = INPUT.GatherRed(sam, tpos);
	const float4 sg = INPUT.GatherGreen(sam, tpos);
	const float4 sb = INPUT.GatherBlue(sam, tpos);

	float4 result = mul(float3(sr.w, sg.w, sb.w), float3x4(-0.0057322932, 0.12928207, -0.056848746, 0.18680117, -0.0306273, 0.25602463, 0.053723164, 0.20419341, 0.0018709862, 0.022848232, -0.04105527, 0.101690340));
	result += mul(float3(sr.x, sg.x, sb.x), float3x4(0.009471417, -0.12957802, 0.096014425, 0.21836184, 0.00021601951, -0.22997683, 0.23666254, 0.41192335, 0.021762101, 0.0047863554, 0.008233427, 0.108514786));
	result += mul(INPUT.SampleLevel(sam, pos + float2(-inputPtX, inputPtY), 0).rgb, float3x4(-0.01156376, -0.18988979, 0.04614705, -0.044767227, 0.01050636, -0.26426336, 0.23741047, 0.0027636609, -0.027718676, -0.14202335, -0.016650287, -0.06637125));
	result += mul(float3(sr.z, sg.z, sb.z), float3x4(0.057809234, -0.11033858, 0.056533534, -0.06292466, 0.13880666, -0.18710336, 0.2441031, -0.25326246, 0.0032683122, -0.026437074, 0.0023248852, 7.640766e-05));
	result += mul(float3(sr.y, sg.y, sb.y), float3x4(-0.49110603, 0.4429004, -0.44015464, -0.41174838, -0.87738293, 0.7808468, -1.0929365, -0.59699076, -0.18409836, 0.185138, -0.11773224, -0.17097276));
	result += mul(INPUT.SampleLevel(sam, pos + float2(0, inputPtY), 0).rgb, float3x4(0.10580959, -0.055947904, -0.03431237, -0.080236495, 0.14862584, -0.15393938, -0.18872876, -0.3170681, 0.03559387, -0.003990826, 0.021298569, 0.012844483));
	result += mul(INPUT.SampleLevel(sam, pos + float2(inputPtX, -inputPtY), 0).rgb, float3x4(-0.040715586, -0.25781113, 0.08896714, -0.1225879, -0.15790503, -0.54010904, 0.29588607, 0.10401059, 0.003413123, -0.108357325, 0.0112870345, -0.11888622));
	result += mul(INPUT.SampleLevel(sam, pos + float2(inputPtX, 0), 0).rgb, float3x4(0.0049315444, 0.02376202, -0.08224771, 0.121118225, -0.041512914, -0.027994309, -0.585988, -0.069672115, -0.017247835, 0.0056576864, 0.04319012, 0.055003505));
	result += mul(INPUT.SampleLevel(sam, pos + float2(inputPtX, inputPtY), 0).rgb, float3x4(0.37521392, 0.15916082, 0.059708964, 0.19046007, 0.8120325, 0.38343868, 0.3436578, 0.5287958, 0.16570656, 0.06957687, 0.014022592, 0.074799836));
	result += float4(-0.01050964, -0.00939481, 0.17684458, 0.027366742);
	return result;
}

uint ABfe(uint src, uint off, uint bits) {
	uint mask = (1u << bits) - 1;
	return (src >> off) & mask;
}
uint ABfiM(uint src, uint ins, uint bits) {
	uint mask = (1u << bits) - 1;
	return (ins & mask) | (src & (~mask));
}
uint2 ARmp8x8(uint a) {
	return uint2(ABfe(a, 1u, 3u), ABfiM(ABfe(a, 3u, 3u), a, 1u));
}

[numthreads(64, 1, 1)]
void main(uint3 LocalThreadId : SV_GroupThreadID, uint3 WorkGroupId : SV_GroupID, uint3 Dtid : SV_DispatchThreadID) {
	uint2 gxy = ARmp8x8(LocalThreadId.x) + uint2(WorkGroupId.x << 3u, WorkGroupId.y << 3u);
	OUTPUT[gxy] = A4KS1((gxy + 0.5f) * float2(inputPtX, inputPtY));
}
