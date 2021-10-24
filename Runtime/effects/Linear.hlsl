//!MAGPIE EFFECT
//!VERSION 1


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;


//!PASS 1
//!BIND INPUT

float4 Pass1(float2 pos) {
	return INPUT.Sample(sam, pos);
}
