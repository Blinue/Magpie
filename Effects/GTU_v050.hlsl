// GTU-v050
// 移植自 https://github.com/libretro/common-shaders/tree/master/crt/shaders/gtu-v050

////////////////////////////////////////////////////////
// GTU version 0.50
// Author: aliaspider - aliaspider@gmail.com
// License: GPLv3
////////////////////////////////////////////////////////


//!MAGPIE EFFECT
//!VERSION 2


//!PARAMETER
//!DEFAULT 0
//!MIN 0
//!MAX 1
int compositeConnection;

//!PARAMETER
//!DEFAULT 0
//!MIN 0
//!MAX 1
int noScanlines;

//!PARAMETER
//!DEFAULT 256
//!MIN 16
int signalResolution;

//!PARAMETER
//!DEFAULT 83
//!MIN 1
int signalResolutionI;

//!PARAMETER
//!DEFAULT 25
//!MIN 1
int signalResolutionQ;

//!PARAMETER
//!DEFAULT 250
//!MIN 20
int tvVerticalResolution;

//!PARAMETER
//!DEFAULT 0.07
//!MIN -0.3
//!MAX 0.3
float blackLevel;

//!PARAMETER
//!DEFAULT 1
//!MIN 0
//!MAX 2
float contrast;


//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH OUTPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D tex1;

//!SAMPLER
//!FILTER POINT
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT
//!OUT tex1

#define RGB_to_YIQ   transpose(float3x3( 0.299 , 0.595716 , 0.211456 , 0.587 , -0.274453 , -0.522591 , 0.114 , -0.321263 , 0.311135 ))
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

float4 Pass1(float2 pos) {
	float2 inputSize = GetInputSize();
	float2 inputPt = GetInputPt();

	float offset = frac((pos.x * inputSize.x) - 0.5);
	float3   tempColor = 0;
	float    X;
	float3   c;
	float range;
	float i;

	if (compositeConnection) {
		range = ceil(0.5 + inputSize.x / min(min(signalResolution, signalResolutionI), signalResolutionQ));

		for (i = -range; i < range + 2.0; i++) {
			X = (offset - (i));
			c = INPUT.SampleLevel(sam, float2(pos.x - X * inputPt.x, pos.y), 0).rgb;
			c = mul(RGB_to_YIQ, c);
			tempColor += float3((c.x * STU(X, (signalResolution * inputPt.x))), (c.y * STU(X, (signalResolutionI * inputPt.x))), (c.z * STU(X, (signalResolutionQ * inputPt.x))));
		}

		tempColor = clamp(mul(YIQ_to_RGB, tempColor), 0.0, 1.0);
	} else {
		range = ceil(0.5 + inputSize.x / signalResolution);

		for (i = -range; i < range + 2.0; i++) {
			X = (offset - (i));
			c = INPUT.SampleLevel(sam, float2(pos.x - X * inputPt.x, pos.y), 0).rgb;
			tempColor += (c * STU(X, (signalResolution * inputPt.x)));
		}

		tempColor = clamp(tempColor, 0.0, 1.0);
	}

	return float4(tempColor, 1.0);
}


//!PASS 2
//!STYLE PS
//!IN tex1

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
	float inputHeight = GetInputSize().y;
	float inputPtY = GetInputPt().y;
	float outputHeight = GetOutputSize().y;
	float outputPtY = GetOutputPt().y;

	float temp = sqrt(2 * pi) * (tvVerticalResolution * inputPtY);

	float rrr = 0.5 * (inputHeight  * outputPtY);
	float x1 = (x + rrr) * temp;
	float x2 = (x - rrr) * temp;
	c.r = (c.r * (normalGaussIntegral(x1) - normalGaussIntegral(x2)));
	c.g = (c.g * (normalGaussIntegral(x1) - normalGaussIntegral(x2)));
	c.b = (c.b * (normalGaussIntegral(x1) - normalGaussIntegral(x2)));
	c *= GetScale().y;
	return c;
}


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
	uint2 inputSize = GetInputSize();
	float2 inputPt = GetInputPt();

	float2   offset = frac((pos * uint2(GetOutputSize().x, inputSize.y)) - 0.5);
	float3   tempColor = 0;
	float3	Cj;

	float range = ceil(0.5 + inputSize.y / tvVerticalResolution);

	float i;

	if (noScanlines) {
		for (i = -range; i < range + 2.0; i++) {
			Cj = tex1.SampleLevel(sam, float2(pos.x, pos.y - (offset.y - (i)) * inputPt.y), 0).xyz;
			tempColor += Cj * STU(offset.y - (i), (tvVerticalResolution * inputPt.y));
		}
	} else {
		for (i = -range; i < range + 2.0; i++) {
			Cj = tex1.SampleLevel(sam, float2(pos.x, pos.y - (offset.y - (i)) * inputPt.y), 0).xyz;
			tempColor += scanlines(offset.y - (i), Cj);
		}
	}

	tempColor -= blackLevel;
	tempColor *= contrast / (1.0 - blackLevel);
	return float4(tempColor, 1.0);
}
