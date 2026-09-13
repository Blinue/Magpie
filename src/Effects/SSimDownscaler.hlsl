// 基于感知的图像缩小算法
// 移植自 https://gist.github.com/igv/36508af3ffc84410fe39761d6969be10
// 原始文件使用了大量 mpv 的“特性”，因此可能存在移植错误。如果你熟悉 mpv hook，请帮助我们改进


//!MAGPIE EFFECT
//!VERSION 4

//!PARAMETER
//!LABEL Oversharp
//!DEFAULT 1
//!MIN 1
//!MAX 3
//!STEP 0.1
float oversharp;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
Texture2D OUTPUT;

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
//!DESC CatumllRom
//!STYLE PS
//!IN INPUT
//!OUT POSTKERNEL

// 模拟 mpv 的内置缩放 (CatmullRom)

float4 weight4(float x) {
	// Sharper version.  May look better in some cases. B=0, C=0.75
	return float4(
		((-0.75 * x + 1.5) * x - 0.75) * x,
		(1.25 * x - 2.25) * x * x + 1.0,
		((-1.25 * x + 1.5) * x + 0.75) * x,
		(0.75 * x - 0.75) * x * x
	);
}

float4 Pass1(float2 pos) {
	const float2 inputPt = GetInputPt();
	const float2 inputSize = GetInputSize();

	pos *= inputSize;
	float2 pos1 = floor(pos - 0.5) + 0.5;
	float2 f = pos - pos1;

	float4 rowtaps = weight4(f.x);
	float4 coltaps = weight4(f.y);

	float2 uv1 = pos1 * inputPt;
	float2 uv0 = uv1 - inputPt;
	float2 uv2 = uv1 + inputPt;
	float2 uv3 = uv2 + inputPt;

	float u_weight_sum = rowtaps.y + rowtaps.z;
	float u_middle_offset = rowtaps.z * inputPt.x / u_weight_sum;
	float u_middle = uv1.x + u_middle_offset;

	float v_weight_sum = coltaps.y + coltaps.z;
	float v_middle_offset = coltaps.z * inputPt.y / v_weight_sum;
	float v_middle = uv1.y + v_middle_offset;

	int2 coord_top_left = int2(max(uv0 * inputSize, 0.5));
	int2 coord_bottom_right = int2(min(uv3 * inputSize, inputSize - 0.5));

	float3 top = INPUT.Load(int3(coord_top_left, 0)).rgb * rowtaps.x;
	top += INPUT.SampleLevel(sam1, float2(u_middle, uv0.y), 0).rgb * u_weight_sum;
	top += INPUT.Load(int3(coord_bottom_right.x, coord_top_left.y, 0)).rgb * rowtaps.w;
	float3 total = top * coltaps.x;

	float3 middle = INPUT.SampleLevel(sam1, float2(uv0.x, v_middle), 0).rgb * rowtaps.x;
	middle += INPUT.SampleLevel(sam1, float2(u_middle, v_middle), 0).rgb * u_weight_sum;
	middle += INPUT.SampleLevel(sam1, float2(uv3.x, v_middle), 0).rgb * rowtaps.w;
	total += middle * v_weight_sum;

	float3 bottom = INPUT.Load(int3(coord_top_left.x, coord_bottom_right.y, 0)).rgb * rowtaps.x;
	bottom += INPUT.SampleLevel(sam1, float2(u_middle, uv3.y), 0).rgb * u_weight_sum;
	bottom += INPUT.Load(int3(coord_bottom_right, 0)).rgb * rowtaps.w;
	total += bottom * coltaps.w;

	return float4(total, 1);
}

//!PASS 2
//!DESC L2 pass 1
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
//!DESC L2 pass 2
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
//!DESC mean & R
//!IN L2_2, POSTKERNEL
//!OUT MR
//!BLOCK_SIZE 16
//!NUM_THREADS 64

#define sigma_nsq   10. / (255.*255.)
#define locality    2.0

#define Kernel(x)   pow(1.0 / locality, abs(x))
// taps 需为奇数
#define taps        3

#define Luma(rgb)   ( dot(rgb, float3(0.2126, 0.7152, 0.0722)) )

void Pass4(uint2 blockStart, uint3 threadId) {
	uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	uint2 outputSize = GetOutputSize();
	if (gxy.x >= outputSize.x || gxy.y >= outputSize.y) {
		return;
	}

	float2 outputPt = GetOutputPt();
	uint i, j;

	float3 src1[taps + 1][taps + 1];
	float3 src2[taps + 1][taps + 1];
	[unroll]
	for (i = 0; i < taps; i += 2) {
		[unroll]
		for (j = 0; j < taps; j += 2) {
			const float2 tpos = (int2(gxy + uint2(i, j)) - taps / 2 + 1) * outputPt;
			float4 sr = POSTKERNEL.GatherRed(sam, tpos);
			float4 sg = POSTKERNEL.GatherGreen(sam, tpos);
			float4 sb = POSTKERNEL.GatherBlue(sam, tpos);

			// w z
			// x y
			src1[i][j] = float3(sr.w, sg.w, sb.w);
			src1[i][j + 1] = float3(sr.x, sg.x, sb.x);
			src1[i + 1][j] = float3(sr.z, sg.z, sb.z);
			src1[i + 1][j + 1] = float3(sr.y, sg.y, sb.y);

			sr = L2_2.GatherRed(sam, tpos);
			sg = L2_2.GatherGreen(sam, tpos);
			sb = L2_2.GatherBlue(sam, tpos);

			src2[i][j] = float3(sr.w, sg.w, sb.w);
			src2[i][j + 1] = float3(sr.x, sg.x, sb.x);
			src2[i + 1][j] = float3(sr.z, sg.z, sb.z);
			src2[i + 1][j + 1] = float3(sr.y, sg.y, sb.y);
		}
	}

	float kernels[taps];
	[unroll]
	for (i = 0; i < taps; ++i) {
		kernels[i] = Kernel((int)i - taps / 2);
	}

	[unroll]
	for (i = 0; i <= 1; ++i) {
		[unroll]
		for (j = 0; j <= 1; ++j) {
			uint2 destPos = gxy + uint2(i, j);
			
			float W = 0.0;
			float3x3 avg = 0;

			[unroll]
			for (int i1 = 0; i1 < taps; ++i1) {
				float W1 = 0;
				float3x3 avg1 = 0;

				[unroll]
				for (int j1 = 0; j1 < taps; ++j1) {
					float3 L = src1[j1 + i][i1 + j];
					avg1 += kernels[j1] * float3x3(L, L * L, src2[j1 + i][i1 + j]);
					W1 += kernels[j1];
				}
				avg1 /= W1;

				avg += kernels[i1] * avg1;
				W += kernels[i1];
			}
			avg /= W;

			float Sl = Luma(max(avg[1] - avg[0] * avg[0], 0.));
			float Sh = Luma(max(avg[2] - avg[0] * avg[0], 0.));
			MR[destPos] = float4(avg[0], lerp(sqrt((Sh + sigma_nsq) / (Sl + sigma_nsq)) * oversharp, clamp(Sh / Sl, 0., 1.), int(Sl > Sh)));
		}
	}
}

//!PASS 5
//!DESC final pass
//!IN MR, POSTKERNEL
//!OUT OUTPUT
//!BLOCK_SIZE 16
//!NUM_THREADS 64

#define locality    2.0f

#define Kernel(x)   pow(1.0f / locality, abs(x))
// taps 需为奇数
#define taps        3

void Pass5(uint2 blockStart, uint3 threadId) {
	const uint2 gxy = (Rmp8x8(threadId.x) << 1) + blockStart;
	
	const uint2 outputSize = GetOutputSize();
	if (gxy.x >= outputSize.x || gxy.y >= outputSize.y) {
		return;
	}

	float2 outputPt = GetOutputPt();
	uint i, j;

	float4 src1[taps + 1][taps + 1];
	[unroll]
	for (i = 0; i < taps; i += 2) {
		[unroll]
		for (j = 0; j < taps; j += 2) {
			const float2 tpos = (int2(gxy + uint2(i, j)) - taps / 2 + 1) * outputPt;
			const float4 sr = MR.GatherRed(sam, tpos);
			const float4 sg = MR.GatherGreen(sam, tpos);
			const float4 sb = MR.GatherBlue(sam, tpos);
			const float4 sa = MR.GatherAlpha(sam, tpos);

			// w z
			// x y
			src1[i][j] = float4(sr.w, sg.w, sb.w, sa.w);
			src1[i][j + 1] = float4(sr.x, sg.x, sb.x, sa.x);
			src1[i + 1][j] = float4(sr.z, sg.z, sb.z, sa.z);
			src1[i + 1][j + 1] = float4(sr.y, sg.y, sb.y, sa.y);
		}
	}

	float3 src2[2][2];
	const float2 tpos = (gxy + 1) * outputPt;
	const float4 sr = POSTKERNEL.GatherRed(sam, tpos);
	const float4 sg = POSTKERNEL.GatherGreen(sam, tpos);
	const float4 sb = POSTKERNEL.GatherBlue(sam, tpos);

	// w z
	// x y
	src2[0][0] = float3(sr.w, sg.w, sb.w);
	src2[0][1] = float3(sr.x, sg.x, sb.x);
	src2[1][0] = float3(sr.z, sg.z, sb.z);
	src2[1][1] = float3(sr.y, sg.y, sb.y);

	float kernels[taps];
	[unroll]
	for (i = 0; i < taps; ++i) {
		kernels[i] = Kernel((int)i - taps / 2);
	}

	[unroll]
	for (i = 0; i <= 1; ++i) {
		[unroll]
		for (j = 0; j <= 1; ++j) {
			uint2 destPos = gxy + uint2(i, j);

			float W = 0;
			float3x3 avg = 0;

			[unroll]
			for (int i1 = 0; i1 < taps; ++i1) {
				float W1 = 0;
				float3x3 avg1 = 0;

				[unroll]
				for (int j1 = 0; j1 < taps; ++j1) {
					float4 MRc = src1[j1 + i][i1 + j];
					avg1 += kernels[j1] * float3x3(MRc.a * MRc.rgb, MRc.rgb, MRc.aaa);
					W1 += kernels[j1];
				}
				avg1 /= W1;

				avg += kernels[i1] * avg1;
				W += kernels[i1];
			}
			avg /= W;

			OUTPUT[destPos] = float4(avg[1] + avg[2] * src2[i][j] - avg[0], 1);
		}
	}
}
