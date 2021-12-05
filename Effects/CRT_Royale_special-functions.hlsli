#ifndef SPECIAL_FUNCTIONS_H
#define SPECIAL_FUNCTIONS_H

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

//  This file implements the following mathematical special functions:
//  1.) erf() = 2/sqrt(pi) * indefinite_integral(e**(-x**2))
//  2.) gamma(s), a real-numbered extension of the integer factorial function
//  It also implements normalized_ligamma(s, z), a normalized lower incomplete
//  gamma function for s < 0.5 only.  Both gamma() and normalized_ligamma() can
//  be called with an _impl suffix to use an implementation version with a few
//  extra precomputed parameters (which may be useful for the caller to reuse).
//  See below for details.
//
//  Design Rationale:
//  Pretty much every line of code in this file is duplicated four times for
//  different input types (float4/float3/float2/float).  This is unfortunate,
//  but Cg doesn't allow function templates.  Macros would be far less verbose,
//  but they would make the code harder to document and read.  I don't expect
//  these functions will require a whole lot of maintenance changes unless
//  someone ever has need for more robust incomplete gamma functions, so code
//  duplication seems to be the lesser evil in this case.


///////////////////////////  GAUSSIAN ERROR FUNCTION  //////////////////////////

float3 erf6(const float3 x)
{
	//  Float3 version:
	const float3 sign_x = sign(x);
	const float3 t = 1/(1 + 0.47047*abs(x));
	const float3 result = 1 - t*(0.3480242 + t*(-0.0958798 + t*0.7478556))*
		exp(-(x*x));
	return result * sign_x;
}


float3 erf(const float3 x)
{
	//  Float3 version:
	#ifdef ERF_FAST_APPROXIMATION
		return erft(x);
	#else
		return erf6(x);
	#endif
}


///////////////////////////  COMPLETE GAMMA FUNCTION  //////////////////////////

float3 gamma_impl(const float3 s, const float3 s_inv)
{
	//  Float3 version:
	static const float3 g = 1.12906830989;
	static const float3 c0 = 0.8109119309638332633713423362694399653724431;
	static const float3 c1 = 0.4808354605142681877121661197951496120000040;
	static const float3 e = 2.71828182845904523536028747135266249775724709;
	const float3 sph = s + 0.5;
	const float3 lanczos_sum = c0 + c1/(s + 1.0);
	const float3 base = (sph + g)/e;
	return (pow(base, sph) * lanczos_sum) * s_inv;
}

////////////////  INCOMPLETE GAMMA FUNCTIONS (RESTRICTED INPUT)  ///////////////

//  Lower incomplete gamma function for small s and z (implementation):
float3 ligamma_small_z_impl(const float3 s, const float3 z, const float3 s_inv)
{
	//  Float3 version:
	const float3 scale = pow(z, s);
	float3 sum = s_inv;
	const float3 z_sq = z*z;
	const float3 denom1 = s + 1.0;
	const float3 denom2 = 2.0*s + 4.0;
	const float3 denom3 = 6.0*s + 18.0;
	sum -= z/denom1;
	sum += z_sq/denom2;
	sum -= z * z_sq/denom3;
	return scale * sum;
}

//  Upper incomplete gamma function for small s and large z (implementation):
float3 uigamma_large_z_impl(const float3 s, const float3 z)
{
	//  Float3 version:
	const float3 numerator = pow(z, s) * exp(-z);
	float3 denom = 7.0 + z - s;
	denom = 5.0 + z - s + (3.0*s - 9.0)/denom;
	denom = 3.0 + z - s + (2.0*s - 4.0)/denom;
	denom = 1.0 + z - s + (s - 1.0)/denom;
	return numerator / denom;
}

//  Normalized lower incomplete gamma function for small s (implementation):
float3 normalized_ligamma_impl(const float3 s, const float3 z,
	const float3 s_inv, const float3 gamma_s_inv)
{
	//  Float3 version:
	static const float3 thresh = 0.775075;
	const bool3 z_is_large = z > thresh;
	const float3 large_z = 1.0 - uigamma_large_z_impl(s, z) * gamma_s_inv;
	const float3 small_z = ligamma_small_z_impl(s, z, s_inv) * gamma_s_inv;
	return large_z * float3(z_is_large) + small_z * float3(!z_is_large);
}


#endif  //  SPECIAL_FUNCTIONS_H
