//!MAGPIE EFFECT
//!VERSION 2


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;


//!PASS 1
//!STYLE PS
//!IN INPUT

float4 Main(uint2 pos) {
	return INPUT.SampleLevel(sam, (pos + 0.5f) * GetOutputPt(), 0);
}
