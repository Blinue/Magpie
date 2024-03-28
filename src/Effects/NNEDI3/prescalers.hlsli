// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// Conversion from GLSL to HLSL is done through defines as much as possible to ease synchronization and comparison with upstream
#define ivec2 int2

#define vec2 float2
#define vec3 float3
#define vec4 float4

#define mat4x3 float4x3
#define matrixCompMult(mtx1, mtx2) (mtx1 * mtx2)

#define shared groupshared

#define atan atan2
#define barrier GroupMemoryBarrierWithGroupSync
#define fract frac
#define intBitsToFloat asfloat
#define inversesqrt rsqrt
// mod deals only with positive numbers here and it could be substituted by fmod
#define mod fmod

// lerp handles bools as the third argument differently from mix
float mix(float a, float b, bool c) {
	return c ? b : a;
}

#define MIX_LERP(type1, type3) type1 mix(type1 a, type1 b, type3 c) { return lerp(a, b, c); }
MIX_LERP(float, float)
MIX_LERP(float2, float2)
MIX_LERP(float3, float)
MIX_LERP(float4, float)

#define texture(tex, pos) tex.SampleLevel(sam_##tex, pos, 0.0)

#define OUTPUT_pt float2(GetOutputPt())
#define frag_pos(id) (vec2(id) + vec2(0.5, 0.5))
#define frag_map(id) (OUTPUT_pt * frag_pos(id))
#define HOOKED_map(id) frag_map(id)

#define gl_LocalInvocationIndex (threadId.y*MP_NUM_THREADS_X + threadId.x)
#define gl_LocalInvocationID threadId
#define gl_WorkGroupSize (uint2(MP_NUM_THREADS_X, MP_NUM_THREADS_Y))
#define gl_WorkGroupID (blockStart / uint2(MP_BLOCK_WIDTH, MP_BLOCK_HEIGHT))
#define gl_GlobalInvocationID (gl_WorkGroupID*gl_WorkGroupSize + threadId.xy)

// disable warning about unknown pragma
#pragma warning(disable: 3568)
// disable warning about too many threads (ravu-r4-rgb triggers it)
#pragma warning(disable: 4714)

// https://www.itu.int/dms_pubrec/itu-r/rec/bt/R-REC-BT.709-6-201506-I!!PDF-E.pdf
static const float3 rgb2y = float3(0.2126, 0.7152, 0.0722);
static const float2x3 rgb2uv = {
	-0.2126/1.8556, -0.7152/1.8556,  0.9278/1.8556,
	 0.7874/1.5748, -0.7152/1.5748, -0.0722/1.5748
};
static const float3x3 yuv2rgb = {
	1,  0,         1.5748,
	1, -0.187324, -0.468124,
	1,  1.8556,    0
};
