#ifndef BLUR_FUNCTIONS_H
#define BLUR_FUNCTIONS_H

/////////////////////////////////  MIT LICENSE  ////////////////////////////////

//  Copyright (C) 2014 TroggleMonkey
//
//  Permission is hereby granted, free of charge, to any person obtaining a copy
//  of this software and associated documentation files (the "Software"), to
//  deal in the Software without restriction, including without limitation the
//  rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
//  sell copies of the Software, and to permit persons to whom the Software is
//  furnished to do so, subject to the following conditions:
//
//  The above copyright notice and this permission notice shall be included in
//  all copies or substantial portions of the Software.
//
//  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//  FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
//  IN THE SOFTWARE.

/////////////////////////////////  DESCRIPTION  ////////////////////////////////

//  This file provides reusable one-pass and separable (two-pass) blurs.
//  Requires:   All blurs share these requirements (dxdy requirement is split):
//              1.) All requirements of gamma-management.h must be satisfied!
//              2.) filter_linearN must == "true" in your .cgp preset unless
//                  you're using tex2DblurNresize at 1x scale.
//              3.) mipmap_inputN must == "true" in your .cgp preset if
//                  IN.output_size < IN.video_size.
//              4.) IN.output_size == IN.video_size / pow(2, M), where M is some
//                  positive integer.  tex2Dblur*resize can resize arbitrarily
//                  (and the blur will be done after resizing), but arbitrary
//                  resizes "fail" with other blurs due to the way they mix
//                  static weights with bilinear sample exploitation.
//              5.) In general, dxdy should contain the uv pixel spacing:
//                      dxdy = (IN.video_size/IN.output_size)/IN.texture_size
//              6.) For separable blurs (tex2DblurNresize and tex2DblurNfast),
//                  zero out the dxdy component in the unblurred dimension:
//                      dxdy = float2(dxdy.x, 0.0) or float2(0.0, dxdy.y)
//              Many blurs share these requirements:
//              1.) One-pass blurs require scale_xN == scale_yN or scales > 1.0,
//                  or they will blur more in the lower-scaled dimension.
//              2.) One-pass shared sample blurs require ddx(), ddy(), and
//                  tex2Dlod() to be supported by the current Cg profile, and
//                  the drivers must support high-quality derivatives.
//              3.) One-pass shared sample blurs require:
//                      tex_uv.w == log2(IN.video_size/IN.output_size).y;
//              Non-wrapper blurs share this requirement:
//              1.) sigma is the intended standard deviation of the blur
//              Wrapper blurs share this requirement, which is automatically
//              met (unless OVERRIDE_BLUR_STD_DEVS is #defined; see below):
//              1.) blurN_std_dev must be global static const float values
//                  specifying standard deviations for Nx blurs in units
//                  of destination pixels
//  Optional:   1.) The including file (or an earlier included file) may
//                  optionally #define USE_BINOMIAL_BLUR_STD_DEVS to replace
//                  default standard deviations with those matching a binomial
//                  distribution.  (See below for details/properties.)
//              2.) The including file (or an earlier included file) may
//                  optionally #define OVERRIDE_BLUR_STD_DEVS and override:
//                      static const float blur3_std_dev
//                      static const float blur4_std_dev
//                      static const float blur5_std_dev
//                      static const float blur6_std_dev
//                      static const float blur7_std_dev
//                      static const float blur8_std_dev
//                      static const float blur9_std_dev
//                      static const float blur10_std_dev
//                      static const float blur11_std_dev
//                      static const float blur12_std_dev
//                      static const float blur17_std_dev
//                      static const float blur25_std_dev
//                      static const float blur31_std_dev
//                      static const float blur43_std_dev
//              3.) The including file (or an earlier included file) may
//                  optionally #define OVERRIDE_ERROR_BLURRING and override:
//                      static const float error_blurring
//                  This tuning value helps mitigate weighting errors from one-
//                  pass shared-sample blurs sharing bilinear samples between
//                  fragments.  Values closer to 0.0 have "correct" blurriness
//                  but allow more artifacts, and values closer to 1.0 blur away
//                  artifacts by sampling closer to halfway between texels.
//              UPDATE 6/21/14: The above static constants may now be overridden
//              by non-static uniform constants.  This permits exposing blur
//              standard deviations as runtime GUI shader parameters.  However,
//              using them keeps weights from being statically computed, and the
//              speed hit depends on the blur: On my machine, uniforms kill over
//              53% of the framerate with tex2Dblur12x12shared, but they only
//              drop the framerate by about 18% with tex2Dblur11fast.
//  Quality and Performance Comparisons:
//  For the purposes of the following discussion, "no sRGB" means
//  GAMMA_ENCODE_EVERY_FBO is #defined, and "sRGB" means it isn't.
//  1.) tex2DblurNfast is always faster than tex2DblurNresize.
//  2.) tex2DblurNresize functions are the only ones that can arbitrarily resize
//      well, because they're the only ones that don't exploit bilinear samples.
//      This also means they're the only functions which can be truly gamma-
//      correct without linear (or sRGB FBO) input, but only at 1x scale.
//  3.) One-pass shared sample blurs only have a speed advantage without sRGB.
//      They also have some inaccuracies due to their shared-[bilinear-]sample
//      design, which grow increasingly bothersome for smaller blurs and higher-
//      frequency source images (relative to their resolution).  I had high
//      hopes for them, but their most realistic use case is limited to quickly
//      reblurring an already blurred input at full resolution.  Otherwise:
//      a.) If you're blurring a low-resolution source, you want a better blur.
//      b.) If you're blurring a lower mipmap, you want a better blur.
//      c.) If you're blurring a high-resolution, high-frequency source, you
//          want a better blur.
//  4.) The one-pass blurs without shared samples grow slower for larger blurs,
//      but they're competitive with separable blurs at 5x5 and smaller, and
//      even tex2Dblur7x7 isn't bad if you're wanting to conserve passes.
//  Here are some framerates from a GeForce 8800GTS.  The first pass resizes to
//  viewport size (4x in this test) and linearizes for sRGB codepaths, and the
//  remaining passes perform 6 full blurs.  Mipmapped tests are performed at the
//  same scale, so they just measure the cost of mipmapping each FBO (only every
//  other FBO is mipmapped for separable blurs, to mimic realistic usage).
//  Mipmap      Neither     sRGB+Mipmap sRGB        Function
//  76.0        92.3        131.3       193.7       tex2Dblur3fast
//  63.2        74.4        122.4       175.5       tex2Dblur3resize
//  93.7        121.2       159.3       263.2       tex2Dblur3x3
//  59.7        68.7        115.4       162.1       tex2Dblur3x3resize
//  63.2        74.4        122.4       175.5       tex2Dblur5fast
//  49.3        54.8        100.0       132.7       tex2Dblur5resize
//  59.7        68.7        115.4       162.1       tex2Dblur5x5
//  64.9        77.2        99.1        137.2       tex2Dblur6x6shared
//  55.8        63.7        110.4       151.8       tex2Dblur7fast
//  39.8        43.9        83.9        105.8       tex2Dblur7resize
//  40.0        44.2        83.2        104.9       tex2Dblur7x7
//  56.4        65.5        71.9        87.9        tex2Dblur8x8shared
//  49.3        55.1        99.9        132.5       tex2Dblur9fast
//  33.3        36.2        72.4        88.0        tex2Dblur9resize
//  27.8        29.7        61.3        72.2        tex2Dblur9x9
//  37.2        41.1        52.6        60.2        tex2Dblur10x10shared
//  44.4        49.5        91.3        117.8       tex2Dblur11fast
//  28.8        30.8        63.6        75.4        tex2Dblur11resize
//  33.6        36.5        40.9        45.5        tex2Dblur12x12shared
//  TODO: Fill in benchmarks for new untested blurs.
//                                                  tex2Dblur17fast
//                                                  tex2Dblur25fast
//                                                  tex2Dblur31fast
//                                                  tex2Dblur43fast
//                                                  tex2Dblur3x3resize


/////////////////////////////  SETTINGS MANAGEMENT  ////////////////////////////

//  Set static standard deviations, but allow users to override them with their
//  own constants (even non-static uniforms if they're okay with the speed hit):
#ifndef OVERRIDE_BLUR_STD_DEVS
	//  blurN_std_dev values are specified in terms of dxdy strides.
	#ifdef USE_BINOMIAL_BLUR_STD_DEVS
		//  By request, we can define standard deviations corresponding to a
		//  binomial distribution with p = 0.5 (related to Pascal's triangle).
		//  This distribution works such that blurring multiple times should
		//  have the same result as a single larger blur.  These values are
		//  larger than default for blurs up to 6x and smaller thereafter.
		static const float blur3_std_dev = 0.84931640625;
		static const float blur4_std_dev = 0.84931640625;
		static const float blur5_std_dev = 1.0595703125;
		static const float blur6_std_dev = 1.06591796875;
		static const float blur7_std_dev = 1.17041015625;
		static const float blur8_std_dev = 1.1720703125;
		static const float blur9_std_dev = 1.2259765625;
		static const float blur10_std_dev = 1.21982421875;
		static const float blur11_std_dev = 1.25361328125;
		static const float blur12_std_dev = 1.2423828125;
		static const float blur17_std_dev = 1.27783203125;
		static const float blur25_std_dev = 1.2810546875;
		static const float blur31_std_dev = 1.28125;
		static const float blur43_std_dev = 1.28125;
	#else
		//  The defaults are the largest values that keep the largest unused
		//  blur term on each side <= 1.0/256.0.  (We could get away with more
		//  or be more conservative, but this compromise is pretty reasonable.)
		static const float blur3_std_dev = 0.62666015625;
		static const float blur4_std_dev = 0.66171875;
		static const float blur5_std_dev = 0.9845703125;
		static const float blur6_std_dev = 1.02626953125;
		static const float blur7_std_dev = 1.36103515625;
		static const float blur8_std_dev = 1.4080078125;
		static const float blur9_std_dev = 1.7533203125;
		static const float blur10_std_dev = 1.80478515625;
		static const float blur11_std_dev = 2.15986328125;
		static const float blur12_std_dev = 2.215234375;
		static const float blur17_std_dev = 3.45535583496;
		static const float blur25_std_dev = 5.3409576416;
		static const float blur31_std_dev = 6.86488037109;
		static const float blur43_std_dev = 10.1852050781;
	#endif  //  USE_BINOMIAL_BLUR_STD_DEVS
#endif  //  OVERRIDE_BLUR_STD_DEVS

#ifndef OVERRIDE_ERROR_BLURRING
	//  error_blurring should be in [0.0, 1.0].  Higher values reduce ringing
	//  in shared-sample blurs but increase blurring and feature shifting.
	static const float error_blurring = 0.5;
#endif


//////////////////////////////////  INCLUDES  //////////////////////////////////

//  gamma-management.h relies on pass-specific settings to guide its behavior:
//  FIRST_PASS, LAST_PASS, GAMMA_ENCODE_EVERY_FBO, etc.  See it for details.
#include "CRT_Royale_gamma-management.hlsli"
#include "CRT_Royale_special-functions.hlsli"


///////////////////////////////////  HELPERS  //////////////////////////////////

inline float4 uv2_to_uv4(float2 tex_uv)
{
	//  Make a float2 uv offset safe for adding to float4 tex2Dlod coords:
	return float4(tex_uv, 0.0, 0.0);
}

//  Make a length squared helper macro (for usage with static constants):
#define LENGTH_SQ(vec) (dot(vec, vec))

inline float get_fast_gaussian_weight_sum_inv(const float sigma)
{
	//  We can use the Gaussian integral to calculate the asymptotic weight for
	//  the center pixel.  Since the unnormalized center pixel weight is 1.0,
	//  the normalized weight is the same as the weight sum inverse.  Given a
	//  large enough blur (9+), the asymptotic weight sum is close and faster:
	//      center_weight = 0.5 *
	//          (erf(0.5/(sigma*sqrt(2.0))) - erf(-0.5/(sigma*sqrt(2.0))))
	//      erf(-x) == -erf(x), so we get 0.5 * (2.0 * erf(blah blah)):
	//  However, we can get even faster results with curve-fitting.  These are
	//  also closer than the asymptotic results, because they were constructed
	//  from 64 blurs sizes from [3, 131) and 255 equally-spaced sigmas from
	//  (0, blurN_std_dev), so the results for smaller sigmas are biased toward
	//  smaller blurs.  The max error is 0.0031793913.
	//  Relative FPS: 134.3 with erf, 135.8 with curve-fitting.
	//static const float temp = 0.5/sqrt(2.0);
	//return erf(temp/sigma);
	return min(exp(exp(0.348348412457428/
		(sigma - 0.0860587260734721))), 0.399334576340352/sigma);
}


///////////////////////////  FAST SEPARABLE BLURS  ///////////////////////////

float3 tex2Dblur9fast(Texture2D tex, SamplerState sam, const float2 tex_uv,
	const float2 dxdy, const float sigma)
{
	//  Requires:   Same as tex2Dblur11()
	//  Returns:    A 1D 9x Gaussian blurred texture lookup using 1 nearest
	//              neighbor and 4 linear taps.  It may be mipmapped depending
	//              on settings and dxdy.
	//  First get the texel weights and normalization factor as above.
	const float denom_inv = 0.5/(sigma*sigma);
	const float w0 = 1.0;
	const float w1 = exp(-1.0 * denom_inv);
	const float w2 = exp(-4.0 * denom_inv);
	const float w3 = exp(-9.0 * denom_inv);
	const float w4 = exp(-16.0 * denom_inv);
	const float weight_sum_inv = 1.0 / (w0 + 2.0 * (w1 + w2 + w3 + w4));
	//  Calculate combined weights and linear sample ratios between texel pairs.
	const float w12 = w1 + w2;
	const float w34 = w3 + w4;
	const float w12_ratio = w2/w12;
	const float w34_ratio = w4/w34;
	//  Statically normalize weights, sum weighted samples, and return:
	float3 sum = 0.0;
	sum += w34 * tex2D_linearize(tex, sam, tex_uv - (3.0 + w34_ratio) * dxdy).rgb;
	sum += w12 * tex2D_linearize(tex, sam, tex_uv - (1.0 + w12_ratio) * dxdy).rgb;
	sum += w0 * tex2D_linearize(tex, sam, tex_uv).rgb;
	sum += w12 * tex2D_linearize(tex, sam, tex_uv + (1.0 + w12_ratio) * dxdy).rgb;
	sum += w34 * tex2D_linearize(tex, sam, tex_uv + (3.0 + w34_ratio) * dxdy).rgb;
	return sum * weight_sum_inv;
}


////////////////////////////  HUGE SEPARABLE BLURS  ////////////////////////////

//  Huge separable blurs come only in "fast" versions.
float3 tex2Dblur43fast(Texture2D tex, SamplerState sam, const float2 tex_uv,
	const float2 dxdy, const float sigma)
{
	//  Requires:   Same as tex2Dblur11()
	//  Returns:    A 1D 43x Gaussian blurred texture lookup using 22 linear
	//              taps.  It may be mipmapped depending on settings and dxdy.
	//  First get the texel weights and normalization factor as above.
	const float denom_inv = 0.5/(sigma*sigma);
	const float w0 = 1.0;
	const float w1 = exp(-1.0 * denom_inv);
	const float w2 = exp(-4.0 * denom_inv);
	const float w3 = exp(-9.0 * denom_inv);
	const float w4 = exp(-16.0 * denom_inv);
	const float w5 = exp(-25.0 * denom_inv);
	const float w6 = exp(-36.0 * denom_inv);
	const float w7 = exp(-49.0 * denom_inv);
	const float w8 = exp(-64.0 * denom_inv);
	const float w9 = exp(-81.0 * denom_inv);
	const float w10 = exp(-100.0 * denom_inv);
	const float w11 = exp(-121.0 * denom_inv);
	const float w12 = exp(-144.0 * denom_inv);
	const float w13 = exp(-169.0 * denom_inv);
	const float w14 = exp(-196.0 * denom_inv);
	const float w15 = exp(-225.0 * denom_inv);
	const float w16 = exp(-256.0 * denom_inv);
	const float w17 = exp(-289.0 * denom_inv);
	const float w18 = exp(-324.0 * denom_inv);
	const float w19 = exp(-361.0 * denom_inv);
	const float w20 = exp(-400.0 * denom_inv);
	const float w21 = exp(-441.0 * denom_inv);
	//const float weight_sum_inv = 1.0 /
	//    (w0 + 2.0 * (w1 + w2 + w3 + w4 + w5 + w6 + w7 + w8 + w9 + w10 + w11 +
	//        w12 + w13 + w14 + w15 + w16 + w17 + w18 + w19 + w20 + w21));
	const float weight_sum_inv = get_fast_gaussian_weight_sum_inv(sigma);
	//  Calculate combined weights and linear sample ratios between texel pairs.
	//  The center texel (with weight w0) is used twice, so halve its weight.
	const float w0_1 = w0 * 0.5 + w1;
	const float w2_3 = w2 + w3;
	const float w4_5 = w4 + w5;
	const float w6_7 = w6 + w7;
	const float w8_9 = w8 + w9;
	const float w10_11 = w10 + w11;
	const float w12_13 = w12 + w13;
	const float w14_15 = w14 + w15;
	const float w16_17 = w16 + w17;
	const float w18_19 = w18 + w19;
	const float w20_21 = w20 + w21;
	const float w0_1_ratio = w1/w0_1;
	const float w2_3_ratio = w3/w2_3;
	const float w4_5_ratio = w5/w4_5;
	const float w6_7_ratio = w7/w6_7;
	const float w8_9_ratio = w9/w8_9;
	const float w10_11_ratio = w11/w10_11;
	const float w12_13_ratio = w13/w12_13;
	const float w14_15_ratio = w15/w14_15;
	const float w16_17_ratio = w17/w16_17;
	const float w18_19_ratio = w19/w18_19;
	const float w20_21_ratio = w21/w20_21;
	//  Statically normalize weights, sum weighted samples, and return:
	float3 sum = 0.0;
	sum += w20_21 * tex2D_linearize(tex, sam, tex_uv - (20.0 + w20_21_ratio) * dxdy).rgb;
	sum += w18_19 * tex2D_linearize(tex, sam, tex_uv - (18.0 + w18_19_ratio) * dxdy).rgb;
	sum += w16_17 * tex2D_linearize(tex, sam, tex_uv - (16.0 + w16_17_ratio) * dxdy).rgb;
	sum += w14_15 * tex2D_linearize(tex, sam, tex_uv - (14.0 + w14_15_ratio) * dxdy).rgb;
	sum += w12_13 * tex2D_linearize(tex, sam, tex_uv - (12.0 + w12_13_ratio) * dxdy).rgb;
	sum += w10_11 * tex2D_linearize(tex, sam, tex_uv - (10.0 + w10_11_ratio) * dxdy).rgb;
	sum += w8_9 * tex2D_linearize(tex, sam, tex_uv - (8.0 + w8_9_ratio) * dxdy).rgb;
	sum += w6_7 * tex2D_linearize(tex, sam, tex_uv - (6.0 + w6_7_ratio) * dxdy).rgb;
	sum += w4_5 * tex2D_linearize(tex, sam, tex_uv - (4.0 + w4_5_ratio) * dxdy).rgb;
	sum += w2_3 * tex2D_linearize(tex, sam, tex_uv - (2.0 + w2_3_ratio) * dxdy).rgb;
	sum += w0_1 * tex2D_linearize(tex, sam, tex_uv - w0_1_ratio * dxdy).rgb;
	sum += w0_1 * tex2D_linearize(tex, sam, tex_uv + w0_1_ratio * dxdy).rgb;
	sum += w2_3 * tex2D_linearize(tex, sam, tex_uv + (2.0 + w2_3_ratio) * dxdy).rgb;
	sum += w4_5 * tex2D_linearize(tex, sam, tex_uv + (4.0 + w4_5_ratio) * dxdy).rgb;
	sum += w6_7 * tex2D_linearize(tex, sam, tex_uv + (6.0 + w6_7_ratio) * dxdy).rgb;
	sum += w8_9 * tex2D_linearize(tex, sam, tex_uv + (8.0 + w8_9_ratio) * dxdy).rgb;
	sum += w10_11 * tex2D_linearize(tex, sam, tex_uv + (10.0 + w10_11_ratio) * dxdy).rgb;
	sum += w12_13 * tex2D_linearize(tex, sam, tex_uv + (12.0 + w12_13_ratio) * dxdy).rgb;
	sum += w14_15 * tex2D_linearize(tex, sam, tex_uv + (14.0 + w14_15_ratio) * dxdy).rgb;
	sum += w16_17 * tex2D_linearize(tex, sam, tex_uv + (16.0 + w16_17_ratio) * dxdy).rgb;
	sum += w18_19 * tex2D_linearize(tex, sam, tex_uv + (18.0 + w18_19_ratio) * dxdy).rgb;
	sum += w20_21 * tex2D_linearize(tex, sam, tex_uv + (20.0 + w20_21_ratio) * dxdy).rgb;
	return sum * weight_sum_inv;
}

float3 tex2Dblur31fast(Texture2D tex, SamplerState sam, const float2 tex_uv,
	const float2 dxdy, const float sigma)
{
	//  Requires:   Same as tex2Dblur11()
	//  Returns:    A 1D 31x Gaussian blurred texture lookup using 16 linear
	//              taps.  It may be mipmapped depending on settings and dxdy.
	//  First get the texel weights and normalization factor as above.
	const float denom_inv = 0.5/(sigma*sigma);
	const float w0 = 1.0;
	const float w1 = exp(-1.0 * denom_inv);
	const float w2 = exp(-4.0 * denom_inv);
	const float w3 = exp(-9.0 * denom_inv);
	const float w4 = exp(-16.0 * denom_inv);
	const float w5 = exp(-25.0 * denom_inv);
	const float w6 = exp(-36.0 * denom_inv);
	const float w7 = exp(-49.0 * denom_inv);
	const float w8 = exp(-64.0 * denom_inv);
	const float w9 = exp(-81.0 * denom_inv);
	const float w10 = exp(-100.0 * denom_inv);
	const float w11 = exp(-121.0 * denom_inv);
	const float w12 = exp(-144.0 * denom_inv);
	const float w13 = exp(-169.0 * denom_inv);
	const float w14 = exp(-196.0 * denom_inv);
	const float w15 = exp(-225.0 * denom_inv);
	//const float weight_sum_inv = 1.0 /
	//    (w0 + 2.0 * (w1 + w2 + w3 + w4 + w5 + w6 + w7 + w8 +
	//        w9 + w10 + w11 + w12 + w13 + w14 + w15));
	const float weight_sum_inv = get_fast_gaussian_weight_sum_inv(sigma);
	//  Calculate combined weights and linear sample ratios between texel pairs.
	//  The center texel (with weight w0) is used twice, so halve its weight.
	const float w0_1 = w0 * 0.5 + w1;
	const float w2_3 = w2 + w3;
	const float w4_5 = w4 + w5;
	const float w6_7 = w6 + w7;
	const float w8_9 = w8 + w9;
	const float w10_11 = w10 + w11;
	const float w12_13 = w12 + w13;
	const float w14_15 = w14 + w15;
	const float w0_1_ratio = w1/w0_1;
	const float w2_3_ratio = w3/w2_3;
	const float w4_5_ratio = w5/w4_5;
	const float w6_7_ratio = w7/w6_7;
	const float w8_9_ratio = w9/w8_9;
	const float w10_11_ratio = w11/w10_11;
	const float w12_13_ratio = w13/w12_13;
	const float w14_15_ratio = w15/w14_15;
	//  Statically normalize weights, sum weighted samples, and return:
	float3 sum = 0.0;
	sum += w14_15 * tex2D_linearize(tex, sam, tex_uv - (14.0 + w14_15_ratio) * dxdy).rgb;
	sum += w12_13 * tex2D_linearize(tex, sam, tex_uv - (12.0 + w12_13_ratio) * dxdy).rgb;
	sum += w10_11 * tex2D_linearize(tex, sam, tex_uv - (10.0 + w10_11_ratio) * dxdy).rgb;
	sum += w8_9 * tex2D_linearize(tex, sam, tex_uv - (8.0 + w8_9_ratio) * dxdy).rgb;
	sum += w6_7 * tex2D_linearize(tex, sam, tex_uv - (6.0 + w6_7_ratio) * dxdy).rgb;
	sum += w4_5 * tex2D_linearize(tex, sam, tex_uv - (4.0 + w4_5_ratio) * dxdy).rgb;
	sum += w2_3 * tex2D_linearize(tex, sam, tex_uv - (2.0 + w2_3_ratio) * dxdy).rgb;
	sum += w0_1 * tex2D_linearize(tex, sam, tex_uv - w0_1_ratio * dxdy).rgb;
	sum += w0_1 * tex2D_linearize(tex, sam, tex_uv + w0_1_ratio * dxdy).rgb;
	sum += w2_3 * tex2D_linearize(tex, sam, tex_uv + (2.0 + w2_3_ratio) * dxdy).rgb;
	sum += w4_5 * tex2D_linearize(tex, sam, tex_uv + (4.0 + w4_5_ratio) * dxdy).rgb;
	sum += w6_7 * tex2D_linearize(tex, sam, tex_uv + (6.0 + w6_7_ratio) * dxdy).rgb;
	sum += w8_9 * tex2D_linearize(tex, sam, tex_uv + (8.0 + w8_9_ratio) * dxdy).rgb;
	sum += w10_11 * tex2D_linearize(tex, sam, tex_uv + (10.0 + w10_11_ratio) * dxdy).rgb;
	sum += w12_13 * tex2D_linearize(tex, sam, tex_uv + (12.0 + w12_13_ratio) * dxdy).rgb;
	sum += w14_15 * tex2D_linearize(tex, sam, tex_uv + (14.0 + w14_15_ratio) * dxdy).rgb;
	return sum * weight_sum_inv;
}

float3 tex2Dblur25fast(Texture2D tex, SamplerState sam, const float2 tex_uv,
	const float2 dxdy, const float sigma)
{
	//  Requires:   Same as tex2Dblur11()
	//  Returns:    A 1D 25x Gaussian blurred texture lookup using 1 nearest
	//              neighbor and 12 linear taps.  It may be mipmapped depending
	//              on settings and dxdy.
	//  First get the texel weights and normalization factor as above.
	const float denom_inv = 0.5/(sigma*sigma);
	const float w0 = 1.0;
	const float w1 = exp(-1.0 * denom_inv);
	const float w2 = exp(-4.0 * denom_inv);
	const float w3 = exp(-9.0 * denom_inv);
	const float w4 = exp(-16.0 * denom_inv);
	const float w5 = exp(-25.0 * denom_inv);
	const float w6 = exp(-36.0 * denom_inv);
	const float w7 = exp(-49.0 * denom_inv);
	const float w8 = exp(-64.0 * denom_inv);
	const float w9 = exp(-81.0 * denom_inv);
	const float w10 = exp(-100.0 * denom_inv);
	const float w11 = exp(-121.0 * denom_inv);
	const float w12 = exp(-144.0 * denom_inv);
	//const float weight_sum_inv = 1.0 / (w0 + 2.0 * (
	//    w1 + w2 + w3 + w4 + w5 + w6 + w7 + w8 + w9 + w10 + w11 + w12));
	const float weight_sum_inv = get_fast_gaussian_weight_sum_inv(sigma);
	//  Calculate combined weights and linear sample ratios between texel pairs.
	const float w1_2 = w1 + w2;
	const float w3_4 = w3 + w4;
	const float w5_6 = w5 + w6;
	const float w7_8 = w7 + w8;
	const float w9_10 = w9 + w10;
	const float w11_12 = w11 + w12;
	const float w1_2_ratio = w2/w1_2;
	const float w3_4_ratio = w4/w3_4;
	const float w5_6_ratio = w6/w5_6;
	const float w7_8_ratio = w8/w7_8;
	const float w9_10_ratio = w10/w9_10;
	const float w11_12_ratio = w12/w11_12;
	//  Statically normalize weights, sum weighted samples, and return:
	float3 sum = 0.0;
	sum += w11_12 * tex2D_linearize(tex, sam, tex_uv - (11.0 + w11_12_ratio) * dxdy).rgb;
	sum += w9_10 * tex2D_linearize(tex, sam, tex_uv - (9.0 + w9_10_ratio) * dxdy).rgb;
	sum += w7_8 * tex2D_linearize(tex, sam, tex_uv - (7.0 + w7_8_ratio) * dxdy).rgb;
	sum += w5_6 * tex2D_linearize(tex, sam, tex_uv - (5.0 + w5_6_ratio) * dxdy).rgb;
	sum += w3_4 * tex2D_linearize(tex, sam, tex_uv - (3.0 + w3_4_ratio) * dxdy).rgb;
	sum += w1_2 * tex2D_linearize(tex, sam, tex_uv - (1.0 + w1_2_ratio) * dxdy).rgb;
	sum += w0 * tex2D_linearize(tex, sam, tex_uv).rgb;
	sum += w1_2 * tex2D_linearize(tex, sam, tex_uv + (1.0 + w1_2_ratio) * dxdy).rgb;
	sum += w3_4 * tex2D_linearize(tex, sam, tex_uv + (3.0 + w3_4_ratio) * dxdy).rgb;
	sum += w5_6 * tex2D_linearize(tex, sam, tex_uv + (5.0 + w5_6_ratio) * dxdy).rgb;
	sum += w7_8 * tex2D_linearize(tex, sam, tex_uv + (7.0 + w7_8_ratio) * dxdy).rgb;
	sum += w9_10 * tex2D_linearize(tex, sam, tex_uv + (9.0 + w9_10_ratio) * dxdy).rgb;
	sum += w11_12 * tex2D_linearize(tex, sam, tex_uv + (11.0 + w11_12_ratio) * dxdy).rgb;
	return sum * weight_sum_inv;
}

float3 tex2Dblur17fast(Texture2D tex, SamplerState sam, const float2 tex_uv,
	const float2 dxdy, const float sigma)
{
	//  Requires:   Same as tex2Dblur11()
	//  Returns:    A 1D 17x Gaussian blurred texture lookup using 1 nearest
	//              neighbor and 8 linear taps.  It may be mipmapped depending
	//              on settings and dxdy.
	//  First get the texel weights and normalization factor as above.
	const float denom_inv = 0.5/(sigma*sigma);
	const float w0 = 1.0;
	const float w1 = exp(-1.0 * denom_inv);
	const float w2 = exp(-4.0 * denom_inv);
	const float w3 = exp(-9.0 * denom_inv);
	const float w4 = exp(-16.0 * denom_inv);
	const float w5 = exp(-25.0 * denom_inv);
	const float w6 = exp(-36.0 * denom_inv);
	const float w7 = exp(-49.0 * denom_inv);
	const float w8 = exp(-64.0 * denom_inv);
	//const float weight_sum_inv = 1.0 / (w0 + 2.0 * (
	//    w1 + w2 + w3 + w4 + w5 + w6 + w7 + w8));
	const float weight_sum_inv = get_fast_gaussian_weight_sum_inv(sigma);
	//  Calculate combined weights and linear sample ratios between texel pairs.
	const float w1_2 = w1 + w2;
	const float w3_4 = w3 + w4;
	const float w5_6 = w5 + w6;
	const float w7_8 = w7 + w8;
	const float w1_2_ratio = w2/w1_2;
	const float w3_4_ratio = w4/w3_4;
	const float w5_6_ratio = w6/w5_6;
	const float w7_8_ratio = w8/w7_8;
	//  Statically normalize weights, sum weighted samples, and return:
	float3 sum = 0.0;
	sum += w7_8 * tex2D_linearize(tex, sam, tex_uv - (7.0 + w7_8_ratio) * dxdy).rgb;
	sum += w5_6 * tex2D_linearize(tex, sam, tex_uv - (5.0 + w5_6_ratio) * dxdy).rgb;
	sum += w3_4 * tex2D_linearize(tex, sam, tex_uv - (3.0 + w3_4_ratio) * dxdy).rgb;
	sum += w1_2 * tex2D_linearize(tex, sam, tex_uv - (1.0 + w1_2_ratio) * dxdy).rgb;
	sum += w0 * tex2D_linearize(tex, sam, tex_uv).rgb;
	sum += w1_2 * tex2D_linearize(tex, sam, tex_uv + (1.0 + w1_2_ratio) * dxdy).rgb;
	sum += w3_4 * tex2D_linearize(tex, sam, tex_uv + (3.0 + w3_4_ratio) * dxdy).rgb;
	sum += w5_6 * tex2D_linearize(tex, sam, tex_uv + (5.0 + w5_6_ratio) * dxdy).rgb;
	sum += w7_8 * tex2D_linearize(tex, sam, tex_uv + (7.0 + w7_8_ratio) * dxdy).rgb;
	return sum * weight_sum_inv;
}


////////////////////  ARBITRARILY RESIZABLE ONE-PASS BLURS  ////////////////////

float3 tex2Dblur3x3resize(Texture2D tex, SamplerState sam, const float2 tex_uv,
	const float2 dxdy, const float sigma)
{
	//  Requires:   Global requirements must be met (see file description).
	//  Returns:    A 3x3 Gaussian blurred mipmapped texture lookup of the
	//              resized input.
	//  Description:
	//  This is the only arbitrarily resizable one-pass blur; tex2Dblur5x5resize
	//  would perform like tex2Dblur9x9, MUCH slower than tex2Dblur5resize.
	const float denom_inv = 0.5/(sigma*sigma);
	//  Load each sample.  We need all 3x3 samples.  Quad-pixel communication
	//  won't help either: This should perform like tex2Dblur5x5, but sharing a
	//  4x4 sample field would perform more like tex2Dblur8x8shared (worse).
	const float2 sample4_uv = tex_uv;
	const float2 dx = float2(dxdy.x, 0.0);
	const float2 dy = float2(0.0, dxdy.y);
	const float2 sample1_uv = sample4_uv - dy;
	const float2 sample7_uv = sample4_uv + dy;
	const float3 sample0 = tex2D_linearize(tex, sam, sample1_uv - dx).rgb;
	const float3 sample1 = tex2D_linearize(tex, sam, sample1_uv).rgb;
	const float3 sample2 = tex2D_linearize(tex, sam, sample1_uv + dx).rgb;
	const float3 sample3 = tex2D_linearize(tex, sam, sample4_uv - dx).rgb;
	const float3 sample4 = tex2D_linearize(tex, sam, sample4_uv).rgb;
	const float3 sample5 = tex2D_linearize(tex, sam, sample4_uv + dx).rgb;
	const float3 sample6 = tex2D_linearize(tex, sam, sample7_uv - dx).rgb;
	const float3 sample7 = tex2D_linearize(tex, sam, sample7_uv).rgb;
	const float3 sample8 = tex2D_linearize(tex, sam, sample7_uv + dx).rgb;
	//  Statically compute Gaussian sample weights:
	const float w4 = 1.0;
	const float w1_3_5_7 = exp(-LENGTH_SQ(float2(1.0, 0.0)) * denom_inv);
	const float w0_2_6_8 = exp(-LENGTH_SQ(float2(1.0, 1.0)) * denom_inv);
	const float weight_sum_inv = 1.0/(w4 + 4.0 * (w1_3_5_7 + w0_2_6_8));
	//  Weight and sum the samples:
	const float3 sum = w4 * sample4 +
		w1_3_5_7 * (sample1 + sample3 + sample5 + sample7) +
		w0_2_6_8 * (sample0 + sample2 + sample6 + sample8);
	return sum * weight_sum_inv;
}

///////////////////////  MAX OPTIMAL SIGMA BLUR WRAPPERS  //////////////////////

//  The following blurs are static wrappers around the dynamic blurs above.
//  HOPEFULLY, the compiler will be smart enough to do constant-folding.

//  Resizable separable blurs:
float3 tex2Dblur9fast(Texture2D tex, SamplerState sam, const float2 tex_uv,
	const float2 dxdy)
{
	return tex2Dblur9fast(tex, sam, tex_uv, dxdy, blur9_std_dev);
}

//  Resizable one-pass blurs:
float3 tex2Dblur3x3resize(Texture2D tex, SamplerState sam, const float2 tex_uv,
	const float2 dxdy)
{
	return tex2Dblur3x3resize(tex, sam, tex_uv, dxdy, blur3_std_dev);
}


#endif  //  BLUR_FUNCTIONS_H
