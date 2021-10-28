// 基于感知的图像缩小算法
// 移植自 https://gist.github.com/igv/36508af3ffc84410fe39761d6969be10

//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!VALUE OUTPUT_PT_X
float outputPtX;

//!CONSTANT
//!VALUE OUTPUT_PT_Y
float outputPtY;


//!CONSTANT
//!DEFAULT 1
//!MIN 0
//!MAX 2

// 0：Mitchell
// 1：Catrom
// 2：Sharper
int variant;

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
//!FORMAT B8G8R8A8_UNORM
Texture2D scaleTex;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!BIND INPUT
//!SAVE scaleTex

float weight(float x, float B, float C) {
	float ax = abs(x);

	if (ax < 1.0) {
		return (x * x * ((12.0 - 9.0 * B - 6.0 * C) * ax + (-18.0 + 12.0 * B + 6.0 * C)) + (6.0 - 2.0 * B)) / 6.0;
	} else if (ax >= 1.0 && ax < 2.0) {
		return (x * x * ((-B - 6.0 * C) * ax + (6.0 * B + 30.0 * C)) + (-12.0 * B - 48.0 * C) * ax + (8.0 * B + 24.0 * C)) / 6.0;
	} else {
		return 0.0;
	}
}

float4 weight4(float x) {
	float B = 0.0;
	float C = 0.0;

	if (variant == 0) {
		// Mitchell
		B = 1.0 / 3.0;
		C = 1.0 / 3.0;
	} else if (variant == 1) {
		// Catrom
		B = 0.0;
		C = 0.5;
	} else {
		// Sharper
		// Photoshop 使用的参数
		B = 0.0;
		C = 0.75;
	}
	// Hermite: B = 0; C = 0;
	// Spline: B = 1; C = 0;
	// Robidoux: B = 0.3782; C = 0.3109;
	// Robidoux Sharp: B = 0.2620; C = 0.3690;
	// Robidoux Soft: B = 0.6796; C = 0.1602;

	return float4(
		weight(x - 2.0, B, C),
		weight(x - 1.0, B, C),
		weight(x, B, C),
		weight(x + 1.0, B, C)
		);
}

float3 line_run(float ypos, float4 xpos, float4 linetaps) {
	return INPUT.Sample(sam, float2(xpos.r, ypos)).rgb * linetaps.r
		+ INPUT.Sample(sam, float2(xpos.g, ypos)).rgb * linetaps.g
		+ INPUT.Sample(sam, float2(xpos.b, ypos)).rgb * linetaps.b
		+ INPUT.Sample(sam, float2(xpos.a, ypos)).rgb * linetaps.a;
}


float4 Pass1(float2 pos) {
	float2 f = frac(pos / float2(inputPtX, inputPtY) + 0.5);

	float4 linetaps = weight4(1.0 - f.x);
	float4 columntaps = weight4(1.0 - f.y);

	// make sure all taps added together is exactly 1.0, otherwise some (very small) distortion can occur
	linetaps /= linetaps.r + linetaps.g + linetaps.b + linetaps.a;
	columntaps /= columntaps.r + columntaps.g + columntaps.b + columntaps.a;

	// !!!改变当前坐标
	pos -= (f + 1) * float2(inputPtX, inputPtY);

	float4 xpos = float4(pos.x, pos.x + inputPtX, pos.x + 2 * inputPtX, pos.x + 3 * inputPtX);

	// final sum and weight normalization
	return float4(line_run(pos.y, xpos, linetaps) * columntaps.r
		+ line_run(pos.y + inputPtY, xpos, linetaps) * columntaps.g
		+ line_run(pos.y + 2 * inputPtY, xpos, linetaps) * columntaps.b
		+ line_run(pos.y + 3 * inputPtY, xpos, linetaps) * columntaps.a,
		1);
}

//!PASS 2
//!BIND INPUT
//!SAVE L2

#define MN(B,C,x)   (x < 1.0 ? ((2.-1.5*B-(C))*x + (-3.+2.*B+C))*x*x + (1.-(B)/3.) : (((-(B)/6.-(C))*x + (B+5.*C))*x + (-2.*B-8.*C))*x+((4./3.)*B+4.*C))
#define Kernel(x)   MN(0.0, 0.5, abs(x))
#define taps        2.0


float4 Pass2(float2 pos) {
	float baseY = pos.y;

	float low = ceil((pos.y - taps * outputPtY) / inputPtY - 0.5);
	float high = floor((pos.y + taps * outputPtY) / inputPtY - 0.5);

	float W = 0;
	float3 avg = 0;

	for (float k = low; k <= high; k++) {
		pos.y = inputPtY * (k + 0.5);
		float rel = (pos.y - baseY) / outputPtY;
		float w = Kernel(rel);

		float3 t = INPUT.SampleLevel(sam, pos, 0).rgb;
		avg += w * t * t;
		W += w;
	}
	avg /= W;

	return float4(avg, 1);
}


//!PASS 3
//!BIND L2
//!SAVE L2_2

#define MN(B,C,x)   (x < 1.0 ? ((2.-1.5*B-(C))*x + (-3.+2.*B+C))*x*x + (1.-(B)/3.) : (((-(B)/6.-(C))*x + (B+5.*C))*x + (-2.*B-8.*C))*x+((4./3.)*B+4.*C))
#define Kernel(x)   MN(0.0, 0.5, abs(x))
#define taps        2.0


float4 Pass3(float2 pos) {
	float baseX = pos.x;

	float low = ceil((pos.x - taps * outputPtX) / inputPtX - 0.5);
	float high = floor((pos.x + taps * outputPtX) / inputPtX - 0.5);

	float W = 0;
	float3 avg = 0;

	for (float k = low; k <= high; k++) {
		pos.x = inputPtX * (k + 0.5);
		float rel = (pos.x - baseX) / outputPtX;
		float w = Kernel(rel);

		avg += w * L2.SampleLevel(sam, pos, 0).rgb;
		W += w;
	}
	avg /= W;

	return float4(avg, 1);
}


//!PASS 4
//!BIND L2_2, scaleTex
//!SAVE MR

#define sigma_nsq   49. / (255.*255.)
#define locality    2.0

#define Kernel(x)   pow(1.0 / locality, abs(x))
#define taps        3.0

#define Luma(rgb)   ( dot(rgb, float3(0.2126, 0.7152, 0.0722)) )


float3x3 ScaleH(float2 pos) {
	float low = ceil(-0.5 * taps);
	float high = floor(0.5 * taps);

	float W = 0;
	float3x3 avg = 0;
	float baseX = pos.x;

	for (float k = low; k <= high; k++) {
		pos.x = baseX + outputPtX * k;
		float rel = k;
		float w = Kernel(rel);

		float3 L = scaleTex.Sample(sam, pos).rgb;
		avg += w * float3x3(L, L * L, L2_2.Sample(sam, pos).rgb);
		W += w;
	}
	avg /= W;

	return avg;
}

float4 Pass4(float2 pos) {
	float low = ceil(-0.5 * taps);
	float high = floor(0.5 * taps);

	float W = 0.0;
	float3x3 avg = 0;
	float baseY = pos.y;

	for (float k = low; k <= high; k++) {
		pos.y = baseY + outputPtY * k;
		float rel = k;
		float w = Kernel(rel);

		avg += w * ScaleH(pos);
		W += w;
	}
	avg /= W;

	float Sl = Luma(max(avg[1] - avg[0] * avg[0], 0.)) + sigma_nsq;
	float Sh = Luma(max(avg[2] - avg[0] * avg[0], 0.)) + sigma_nsq;
	return float4(avg[0], sqrt(Sh / Sl));
}


//!PASS 5
//!BIND MR, scaleTex

#define locality    2.0

#define Kernel(x)   pow(1.0 / locality, abs(x))
#define taps        3.0

#define Gamma(x)    ( pow(x, 0.5) )
#define GammaInv(x) ( pow(clamp(x, 0.0, 1.0), 2.0) )

float3x3 ScaleH(float2 pos) {
	float low = ceil(-0.5 * taps);
	float high = floor(0.5 * taps);

	float W = 0;
	float3x3 avg = 0;
	float baseX = pos.x;

	for (float k = low; k <= high; k++) {
		pos.x = baseX + outputPtX * k;
		float rel = k;
		float w = Kernel(rel);

		float4 MRc = MR.Sample(sam, pos);
		float3 M = Gamma(MRc.rgb);
		avg += w * float3x3(MRc.a * M, M, MRc.aaa);
		W += w;
	}
	avg /= W;

	return avg;
}

float4 Pass5(float2 pos) {
	float low = ceil(-0.5 * taps);
	float high = floor(0.5 * taps);

	float W = 0;
	float3x3 avg = 0;
	float2 base = pos;

	for (float k = low; k <= high; k++) {
		pos.y = base.y + outputPtY * k;
		float rel = k;
		float w = Kernel(rel);

		avg += w * ScaleH(pos);
		W += w;
	}
	avg /= W;
	float3 L = scaleTex.Sample(sam, base).rgb;
	return float4(GammaInv(avg[1] + avg[2] * Gamma(L) - avg[0]), 1);
}
