// 推理前的预降采样 / pre-inference downscale.
//
// 让模型在更低的分辨率上运行：例如 4K 窗口先降到 1440p 再由模型放大回去。
// Lets the model run at a lower resolution - e.g. take a 4K window down to 1440p
// and let the model bring it back up. This is what makes a model useful on a
// native-resolution window, where there is otherwise nothing to upscale.
//
// 注意：这不会恢复细节 / Note: this cannot recover detail. The wins are inference
// cost and hitting exact multiples (1080p + 2x = exactly 2160p).
//
// A linear sampler gives bilinear filtering, which is adequate for the modest
// ratios this is used at (4K->1440p is 1.5x). TextureToTensorCS cannot do this
// itself: it uses GatherRed/Green/Blue on a 2x2 grid, which is a 1:1 copy rather
// than a filter, and would alias badly.

Texture2D<float4> src : register(t0);
RWTexture2D<unorm float4> dst : register(u0);

SamplerState sam : register(s0);

[numthreads(8, 8, 1)]
void main(uint3 tid : SV_DispatchThreadID) {
	uint width, height;
	dst.GetDimensions(width, height);

	if (tid.x >= width || tid.y >= height) {
		return;
	}

	// 采样目标像素中心 / sample at the centre of the destination texel
	const float2 uv = (float2(tid.xy) + 0.5f) / float2(width, height);
	dst[tid.xy] = src.SampleLevel(sam, uv, 0);
}
