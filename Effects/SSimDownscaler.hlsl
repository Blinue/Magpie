// 基于感知的图像缩小算法
// 移植自 https://gist.github.com/igv/36508af3ffc84410fe39761d6969be10
// 原始文件使用了大量 mpv 的“特性”，因此可能存在移植错误。如果你熟悉 mpv hook，请帮助我们改进


//!MAGPIE EFFECT
//!VERSION 2


//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT OUTPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D L2;

//!TEXTURE
//!WIDTH OUTPUT_WIDTH
//!HEIGHT OUTPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D L2_2;

//!TEXTURE
//!WIDTH OUTPUT_WIDTH
//!HEIGHT OUTPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D MR;

//!TEXTURE
//!WIDTH OUTPUT_WIDTH
//!HEIGHT OUTPUT_HEIGHT
//!FORMAT R8G8B8A8_UNORM
Texture2D POSTKERNEL;

//!SAMPLER
//!FILTER POINT
SamplerState sam;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam1;


//!PASS 1
//!STYLE PS
//!IN INPUT
//!OUT POSTKERNEL

// 模拟 mpv 的内置缩放（CatmullRom）
// Samples a texture with Catmull-Rom filtering, using 9 texture fetches instead of 16.
// See http://vec3.ca/bicubic-filtering-in-fewer-taps/ for more details
float4 Pass1(float2 pos) {
	float2 inputSize = GetInputSize();
	float2 inputPt = GetInputPt();

    // We're going to sample a a 4x4 grid of texels surrounding the target UV coordinate. We'll do this by rounding
    // down the sample location to get the exact center of our "starting" texel. The starting texel will be at
    // location [1, 1] in the grid, where [0, 0] is the top left corner.
	float2 samplePos = pos * inputSize;
	float2 texPos1 = floor(samplePos - 0.5f) + 0.5f;

    // Compute the fractional offset from our starting texel to our original sample location, which we'll
    // feed into the Catmull-Rom spline function to get our filter weights.
	float2 f = samplePos - texPos1;

    // Compute the Catmull-Rom weights using the fractional offset that we calculated earlier.
    // These equations are pre-expanded based on our knowledge of where the texels will be located,
    // which lets us avoid having to evaluate a piece-wise function.
	float2 w0 = f * (-0.5f + f * (1.0f - 0.5f * f));
	float2 w1 = 1.0f + f * f * (-2.5f + 1.5f * f);
	float2 w2 = f * (0.5f + f * (2.0f - 1.5f * f));
	float2 w3 = f * f * (-0.5f + 0.5f * f);

    // Work out weighting factors and sampling offsets that will let us use bilinear filtering to
    // simultaneously evaluate the middle 2 samples from the 4x4 grid.
	float2 w12 = w1 + w2;
	float2 offset12 = w2 / (w1 + w2);

    // Compute the final UV coordinates we'll use for sampling the texture
	float2 texPos0 = texPos1 - 1;
	float2 texPos3 = texPos1 + 2;
	float2 texPos12 = texPos1 + offset12;

	texPos0 *= inputPt;
	texPos3 *= inputPt;
	texPos12 *= inputPt;

	float4 result = 0.0f;
	result += INPUT.SampleLevel(sam1, float2(texPos0.x, texPos0.y), 0.0f) * w0.x * w0.y;
	result += INPUT.SampleLevel(sam1, float2(texPos12.x, texPos0.y), 0.0f) * w12.x * w0.y;
	result += INPUT.SampleLevel(sam1, float2(texPos3.x, texPos0.y), 0.0f) * w3.x * w0.y;

	result += INPUT.SampleLevel(sam1, float2(texPos0.x, texPos12.y), 0.0f) * w0.x * w12.y;
	result += INPUT.SampleLevel(sam1, float2(texPos12.x, texPos12.y), 0.0f) * w12.x * w12.y;
	result += INPUT.SampleLevel(sam1, float2(texPos3.x, texPos12.y), 0.0f) * w3.x * w12.y;

	result += INPUT.SampleLevel(sam1, float2(texPos0.x, texPos3.y), 0.0f) * w0.x * w3.y;
	result += INPUT.SampleLevel(sam1, float2(texPos12.x, texPos3.y), 0.0f) * w12.x * w3.y;
	result += INPUT.SampleLevel(sam1, float2(texPos3.x, texPos3.y), 0.0f) * w3.x * w3.y;

	return result;
}

//!PASS 2
//!STYLE PS
//!IN INPUT
//!OUT L2

#define MN(B,C,x)   (x < 1.0 ? ((2.-1.5*B-(C))*x + (-3.+2.*B+C))*x*x + (1.-(B)/3.) : (((-(B)/6.-(C))*x + (B+5.*C))*x + (-2.*B-8.*C))*x+((4./3.)*B+4.*C))
#define Kernel(x)   MN(0.0f, 0.5f, abs(x))
#define taps        2.0f


float4 Pass2(float2 pos) {
	const float inputPtY = GetInputPt().y;
	const uint inputHeight = GetInputSize().y;
	const float outputPtY = GetOutputPt().y;
	const uint outputHeight = GetOutputSize().y;

	const int low = (int)ceil((pos.y - taps * outputPtY) * inputHeight - 0.5f);
	const int high = (int)floor((pos.y + taps * outputPtY) * inputHeight - 0.5f);

	float W = 0;
	float3 avg = 0;
	const float baseY = pos.y;

	for (int k = low; k <= high; k++) {
		pos.y = inputPtY * (k + 0.5f);
		float rel = (pos.y - baseY) * outputHeight;
		float w = Kernel(rel);

		float3 tex = INPUT.SampleLevel(sam, pos, 0).rgb;
		avg += w * tex * tex;
		W += w;
	}
	avg /= W;

	return float4(avg, 1);
}


//!PASS 3
//!STYLE PS
//!IN L2
//!OUT L2_2

#define MN(B,C,x)   (x < 1.0 ? ((2.-1.5*B-(C))*x + (-3.+2.*B+C))*x*x + (1.-(B)/3.) : (((-(B)/6.-(C))*x + (B+5.*C))*x + (-2.*B-8.*C))*x+((4./3.)*B+4.*C))
#define Kernel(x)   MN(0.0, 0.5, abs(x))
#define taps        2.0


float4 Pass3(float2 pos) {
	const float inputPtX = GetInputPt().x;
	const uint inputWidth = GetInputSize().x;
	const float outputPtX = GetOutputPt().x;
	const uint outputWidth = GetOutputSize().x;

	const int low = (int)ceil((pos.x - taps * outputPtX) * inputWidth - 0.5f);
	const int high = (int)floor((pos.x + taps * outputPtX) * inputWidth - 0.5f);

	float W = 0;
	float3 avg = 0;
	const float baseX = pos.x;

	for (int k = low; k <= high; k++) {
		pos.x = inputPtX * (k + 0.5f);
		float rel = (pos.x - baseX) * outputWidth;
		float w = Kernel(rel);

		avg += w * L2.SampleLevel(sam, pos, 0).rgb;
		W += w;
	}
	avg /= W;

	return float4(avg, 1);
}


//!PASS 4
//!STYLE PS
//!IN L2_2, POSTKERNEL
//!OUT MR

#define sigma_nsq   49. / (255.*255.)
#define locality    2.0

#define Kernel(x)   pow(1.0 / locality, abs(x))
#define taps        3.0

#define Luma(rgb)   ( dot(rgb, float3(0.2126, 0.7152, 0.0722)) )


float3x3 ScaleH(float2 pos) {
	const float outputPtX = GetOutputPt().x;
	const int low = (int)ceil(-0.5 * taps);
	const int high = (int)floor(0.5 * taps);

	float W = 0;
	float3x3 avg = 0;
	const float baseX = pos.x;

	[unroll]
	for (int k = low; k <= high; k++) {
		pos.x = baseX + outputPtX * k;
		float w = Kernel(k);

		float3 L = POSTKERNEL.SampleLevel(sam, pos, 0).rgb;
		avg += w * float3x3(L, L * L, L2_2.SampleLevel(sam, pos, 0.0f).rgb);
		W += w;
	}
	avg /= W;

	return avg;
}

float4 Pass4(float2 pos) {
	const float outputPtY = GetOutputPt().y;
	const int low = (int)ceil(-0.5f * taps);
	const int high = (int)floor(0.5f * taps);

	float W = 0.0;
	float3x3 avg = 0;
	float baseY = pos.y;

	[unroll]
	for (int k = low; k <= high; k++) {
		pos.y = baseY + outputPtY * k;
		float w = Kernel(k);

		avg += w * ScaleH(pos);
		W += w;
	}
	avg /= W;

	float Sl = Luma(max(avg[1] - avg[0] * avg[0], 0.)) + sigma_nsq;
	float Sh = Luma(max(avg[2] - avg[0] * avg[0], 0.)) + sigma_nsq;
	return float4(avg[0], sqrt(Sh / Sl));
}


//!PASS 5
//!STYLE PS
//!IN MR, POSTKERNEL

#define locality    2.0f

#define Kernel(x)   pow(1.0f / locality, abs(x))
#define taps        3.0f

float3x3 ScaleH(float2 pos) {
	const float outputPtX = GetOutputPt().x;
	const int low = (int)ceil(-0.5f * taps);
	const int high = (int)floor(0.5f * taps);

	float W = 0;
	float3x3 avg = 0;
	const float baseX = pos.x;

	[unroll]
	for (int k = low; k <= high; k++) {
		pos.x = baseX + outputPtX * k;
		float w = Kernel(k);

		float4 MRc = MR.SampleLevel(sam, pos, 0);
		avg += w * float3x3(MRc.a * MRc.rgb, MRc.rgb, MRc.aaa);
		W += w;
	}
	avg /= W;

	return avg;
}

float4 Pass5(float2 pos) {
	const float outputPtY = GetOutputPt().y;
	const int low = (int)ceil(-0.5f * taps);
	const int high = (int)floor(0.5f * taps);

	float W = 0;
	float3x3 avg = 0;
	const float2 base = pos;

	[unroll]
	for (int k = low; k <= high; k++) {
		pos.y = base.y + outputPtY * k;
		float w = Kernel(k);

		avg += w * ScaleH(pos);
		W += w;
	}
	avg /= W;
	float3 L = POSTKERNEL.SampleLevel(sam, base, 0).rgb;
	return float4(avg[1] + avg[2] * L - avg[0], 1);
}
