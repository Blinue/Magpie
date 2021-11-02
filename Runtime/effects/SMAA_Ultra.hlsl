//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_WIDTH
float inputWidth;

//!CONSTANT
//!VALUE INPUT_HEIGHT
float inputHeight;

//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16_FLOAT
Texture2D edgesTex;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D blendTex;

//!TEXTURE
//!SOURCE SMAA_AreaTex.dds
Texture2D areaTex;

//!TEXTURE
//!SOURCE SMAA_SearchTex.dds
Texture2D searchTex;

//!SAMPLER
//!FILTER POINT
SamplerState PointSampler;

//!SAMPLER
//!FILTER LINEAR
SamplerState LinearSampler;


//!COMMON

#define SMAA_RT_METRICS float4(inputPtX, inputPtY, inputWidth, inputHeight)
#define SMAA_LINEAR_SAMPLER LinearSampler
#define SMAA_POINT_SAMPLER PointSampler
#define SMAA_PRESET_ULTRA
#include "SMAA.hlsli"

//!PASS 1
//!BIND INPUT
//!SAVE edgesTex

float4 Pass1(float2 pos) {
	return float4(SMAALumaEdgeDetectionPS(pos, INPUT), 0, 1);
}

//!PASS 2
//!BIND edgesTex, areaTex, searchTex
//!SAVE blendTex

float4 Pass2(float2 pos) {
	return SMAABlendingWeightCalculationPS(pos, edgesTex, areaTex, searchTex, 0);
}

//!PASS 3
//!BIND INPUT, blendTex

float4 Pass3(float2 pos) {
	return SMAANeighborhoodBlendingPS(pos, INPUT, blendTex);
}
