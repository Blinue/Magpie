// GTU-v050
// 移植自 https://github.com/libretro/common-shaders/tree/master/crt/shaders/gtu-v050

////////////////////////////////////////////////////////
// GTU version 0.50
// Author: aliaspider - aliaspider@gmail.com
// License: GPLv3
////////////////////////////////////////////////////////


//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!VALUE INPUT_WIDTH
float inputWidth;

//!CONSTANT
//!VALUE INPUT_HEIGHT
float inputHeight;

//!CONSTANT
//!VALUE OUTPUT_WIDTH
float outputWidth;

//!CONSTANT
//!VALUE OUTPUT_HEIGHT
float outputHeight;

//!CONSTANT
//!DEFAULT 0
//!MIN 0
//!MAX 1
int compositeConnection;

//!CONSTANT
//!DEFAULT 0
//!MIN 0
//!MAX 1
int noScanlines;

//!CONSTANT
//!DEFAULT 256
//!MIN 16
int signalResolution;

//!CONSTANT
//!DEFAULT 83
//!MIN 1
int signalResolutionI;

//!CONSTANT
//!DEFAULT 25
//!MIN 1
int signalResolutionQ;

//!CONSTANT
//!DEFAULT 250
//!MIN 20
int tvVerticalResolution;

//!CONSTANT
//!DEFAULT 0.07
//!MIN -0.3
//!MAX 0.3
float blackLevel;

//!CONSTANT
//!DEFAULT 1
//!MIN 0
//!MAX 2
float contrast;


//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex1;

//!TEXTURE
//!WIDTH OUTPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex2;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!BIND INPUT
//!SAVE tex1

#define RGB_to_YIQ   transpose(float3x3( 0.299 , 0.595716 , 0.211456 , 0.587 , -0.274453 , -0.522591 , 0.114 , -0.321263 , 0.311135 ))

float4 Pass1(float2 pos) {
	float4 c = INPUT.Sample(sam, pos);
	if (compositeConnection)
		c.rgb = mul(RGB_to_YIQ, c.rgb);
	return c;
}

//!PASS 2
//!BIND tex1
//!SAVE tex2

#define YIQ_to_RGB   transpose(float3x3( 1.0 , 1.0  , 1.0 , 0.9563 , -0.2721 , -1.1070 , 0.6210 , -0.6474 , 1.7046 ))
#define pi        3.14159265358

float d(float x, float b) {
	return pi * b * min(abs(x) + 0.5, 1.0 / b);
}

float e(float x, float b) {
	return (pi * b * min(max(abs(x) - 0.5, -1.0 / b), 1.0 / b));
}

float STU(float x, float b) {
	return ((d(x, b) + sin(d(x, b)) - e(x, b) - sin(e(x, b))) / (2.0 * pi));
}

float4 Pass2(float2 pos) {
	float offset = frac((pos.x * inputWidth) - 0.5);
	float3   tempColor = 0;
	float    X;
	float3   c;
	float range;
	if (compositeConnection)
		range = ceil(0.5 + inputWidth / min(min(signalResolution, signalResolutionI), signalResolutionQ));
	else
		range = ceil(0.5 + inputWidth / signalResolution);

	float i;
	if (compositeConnection) {
		for (i = -range; i < range + 2.0; i++) {
			X = (offset - (i));
			c = tex1.Sample(sam, float2(pos.x - X / inputWidth, pos.y)).rgb;
			tempColor += float3((c.x * STU(X, (signalResolution / inputWidth))), (c.y * STU(X, (signalResolutionI / inputWidth))), (c.z * STU(X, (signalResolutionQ / inputWidth))));
		}
	} else {
		for (i = -range; i < range + 2.0; i++) {
			X = (offset - (i));
			c = tex1.Sample(sam, float2(pos.x - X / inputWidth, pos.y)).rgb;
			tempColor += (c * STU(X, (signalResolution / inputWidth)));
		}
	}
	if (compositeConnection)
		tempColor = clamp(mul(YIQ_to_RGB, tempColor), 0.0, 1.0);
	else
		tempColor = clamp(tempColor, 0.0, 1.0);

	return float4(tempColor, 1.0);
}


//!PASS 3
//!BIND tex2

#define pi        3.14159265358
#define normalGauss(x) ((exp(-(x)*(x)*0.5))/sqrt(2.0*pi))

float normalGaussIntegral(float x) {
	float a1 = 0.4361836;
	float a2 = -0.1201676;
	float a3 = 0.9372980;
	float p = 0.3326700;
	float t = 1.0 / (1.0 + p * abs(x));
	return (0.5 - normalGauss(x) * (t * (a1 + t * (a2 + a3 * t)))) * sign(x);
}

float3 scanlines(float x, float3 c) {
	float temp = sqrt(2 * pi) * (tvVerticalResolution / inputHeight);

	float rrr = 0.5 * (inputHeight / outputHeight);
	float x1 = (x + rrr) * temp;
	float x2 = (x - rrr) * temp;
	c.r = (c.r * (normalGaussIntegral(x1) - normalGaussIntegral(x2)));
	c.g = (c.g * (normalGaussIntegral(x1) - normalGaussIntegral(x2)));
	c.b = (c.b * (normalGaussIntegral(x1) - normalGaussIntegral(x2)));
	c *= (outputHeight / inputHeight);
	return c;
}

#define Y(j) (offset.y-(j))
#define SOURCE(j) float2(pos.x,pos.y - Y(j)/inputHeight)
#define C(j) (COMPAT_Sample(decal, SOURCE(j)).xyz)
#define VAL(j) (C(j)*STU(Y(j),(tvVerticalResolution/inputHeight)))
#define VAL_scanlines(j) (scanlines(Y(j),C(j)))

float d(float x, float b) {
	return pi * b * min(abs(x) + 0.5, 1.0 / b);
}

float e(float x, float b) {
	return (pi * b * min(max(abs(x) - 0.5, -1.0 / b), 1.0 / b));
}

float STU(float x, float b) {
	return ((d(x, b) + sin(d(x, b)) - e(x, b) - sin(e(x, b))) / (2.0 * pi));
}

float4 Pass3(float2 pos) {
	float2   offset = frac((pos * float2(outputWidth, inputHeight)) - 0.5);
	float3   tempColor = 0;
	float3	Cj;

	float range = ceil(0.5 + inputHeight / tvVerticalResolution);

	float i;

	if (noScanlines) {
		for (i = -range; i < range + 2.0; i++) {
			Cj = tex2.Sample(sam, float2(pos.x, pos.y - (offset.y - (i)) / inputHeight)).xyz;
			tempColor += Cj * STU(offset.y - (i), (tvVerticalResolution / inputHeight));
		}
	} else {
		for (i = -range; i < range + 2.0; i++) {
			Cj = tex2.Sample(sam, float2(pos.x, pos.y - (offset.y - (i)) / inputHeight)).xyz;
			tempColor += scanlines(offset.y - (i), Cj);
		}
	}

	tempColor -= blackLevel;
	tempColor *= contrast / (1.0 - blackLevel);
	return float4(tempColor, 1.0);
}