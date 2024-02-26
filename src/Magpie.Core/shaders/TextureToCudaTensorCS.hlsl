Texture2D<min16float> tex : register(t0);
RWBuffer<min16float> result : register(u0);

SamplerState sam : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_GroupThreadID, uint3 gid : SV_GroupID) {
	const uint2 gxy = (gid.xy << 4) + (tid.xy << 1);

	uint width, height;
	tex.GetDimensions(width, height);

	if (gxy.x >= width || gxy.y >= height) {
		return;
	}

	const float2 pos = (gxy + 1) / float2(width, height);

	min16float4 red = tex.GatherRed(sam, pos);
	min16float4 green = tex.GatherGreen(sam, pos);
	min16float4 blue = tex.GatherBlue(sam, pos);

	const uint pixelCount = width * height;
	const uint pixelCount2 = width * height * 2;

	// w z
	// x y
	uint idx = gxy.y * width + gxy.x;

	result[idx] = red.w;
	result[idx + pixelCount] = green.w;
	result[idx + pixelCount2] = blue.w;

	const bool zyValid = gxy.x + 1 < width;
	if (zyValid) {
		result[idx + 1] = red.z;
		result[idx + pixelCount + 1] = green.z;
		result[idx + pixelCount2 + 1] = blue.z;
	}

	idx += width;

	if (gxy.y + 1 < height) {
		result[idx] = red.x;
		result[idx + pixelCount] = green.x;
		result[idx + pixelCount2] = blue.x;

		if (zyValid) {
			result[idx + 1] = red.y;
			result[idx + pixelCount + 1] = green.y;
			result[idx + pixelCount2 + 1] = blue.y;
		}
	}
}
