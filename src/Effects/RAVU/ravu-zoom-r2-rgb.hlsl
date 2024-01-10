// Generated by ravu-zoom.py
// 
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




//!MAGPIE EFFECT
//!VERSION 3
//
//


//!TEXTURE
Texture2D INPUT;

//!SAMPLER
//!FILTER POINT
SamplerState sam_INPUT;


//!SAMPLER
//!FILTER LINEAR
SamplerState sam_INPUT_LINEAR;

//!TEXTURE
//!SOURCE ravu_zoom_lut2_f16.dds
//!FORMAT R16G16B16A16_FLOAT
Texture2D ravu_zoom_lut2;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam_ravu_zoom_lut2;





//!COMMON
// Conversion from GLSL to HLSL is done through defines as much as possible to ease synchronization and comparison with upstream
#define ivec2 int2

#define vec2 float2
#define vec3 float3
#define vec4 float4

#define shared groupshared

// TODO: check
// some sources suggest that atan2 has reverse order of arguments compared to atan
#define atan atan2
#define fract frac
#define intBitsToFloat asfloat
#define inversesqrt rsqrt
#define mix lerp

// mod deals only with positive numbers here and it could be substituted by fmod
#define mod fmod

#define barrier GroupMemoryBarrierWithGroupSync
#define texture(tex, pos) tex.SampleLevel(sam_##tex, pos, 0.0)

// TODO: check
// HLSL uses row-major matrixes, while GLSL uses column-major matrixes
// Is this the proper way to deal with this difference?
#define mat4x3 float4x3
#define matrixCompMult(mtx1, mtx2) (mtx1 * mtx2)

#define OUTPUT_pt float2(GetOutputPt())
#define frag_pos(id) (vec2(id) + vec2(0.5, 0.5))
#define frag_map(id) (OUTPUT_pt * frag_pos(id))

#define gl_LocalInvocationIndex (threadId.y*MP_NUM_THREADS_X + threadId.x)
#define gl_LocalInvocationID threadId
#define gl_WorkGroupSize (uint2(MP_NUM_THREADS_X, MP_NUM_THREADS_Y))
#define gl_WorkGroupID (blockStart / uint2(MP_BLOCK_WIDTH, MP_BLOCK_HEIGHT))
#define gl_GlobalInvocationID (gl_WorkGroupID*gl_WorkGroupSize + threadId.xy)

#define LAST_PASS 1

// disable warning about unknown pragma
#pragma warning(disable: 3568)
// disable warning about too many threads (ravu-r4-rgb triggers it)
#pragma warning(disable: 4714)
//!PASS 1
//!DESC RAVU-Zoom (rgb, r2, compute)
//!IN INPUT, ravu_zoom_lut2
//!BLOCK_SIZE 32, 8
//!NUM_THREADS 32, 8
static const vec3 color_primary = vec3(0.2126, 0.7152, 0.0722);
#define LUTPOS(x, lut_size) mix(0.5 / (lut_size), 1.0 - 0.5 / (lut_size), (x))
shared vec3 samples[432];
#define CURRENT_PASS 1
#define HOOKED_map(id) frag_map(id)
#define GET_SAMPLE(x) x
#define imageStore(out_image, pos, val) imageStoreOverride(pos, val.xyz)
void imageStoreOverride(uint2 pos, float3 value) {
    WriteToOutput(pos, value);
}
#define INPUT_tex(pos) GET_SAMPLE(vec4(texture(INPUT, pos)))
static const float2 INPUT_size = float2(GetInputSize());
static const float2 INPUT_pt = float2(GetInputPt());
#define ravu_zoom_lut2_tex(pos) (vec4(texture(ravu_zoom_lut2, pos)))
#define HOOKED_tex(pos) INPUT_tex(pos)
#define HOOKED_size INPUT_size
#define HOOKED_pt INPUT_pt
void Pass1(uint2 blockStart, uint3 threadId) {
ivec2 group_begin = ivec2(gl_WorkGroupID) * ivec2(gl_WorkGroupSize);
ivec2 group_end = group_begin + ivec2(gl_WorkGroupSize);
ivec2 rectl = ivec2(floor(HOOKED_size * HOOKED_map(group_begin) - 0.5)) - 1;
ivec2 rectr = ivec2(floor(HOOKED_size * HOOKED_map(group_end) - 0.5)) + 2;
ivec2 rect = rectr - rectl + 1;
for (int id = int(gl_LocalInvocationIndex); id < rect.x * rect.y; id += int(gl_WorkGroupSize.x * gl_WorkGroupSize.y)) {
    uint y = (uint)id / rect.x, x = (uint)id % rect.x;
    samples[x + y * 36] = HOOKED_tex(HOOKED_pt * (vec2(rectl + ivec2(x, y)) + vec2(0.5,0.5))).xyz;
}
barrier();
#if CURRENT_PASS == LAST_PASS
uint2 destPos = blockStart + threadId.xy;
if (!CheckViewport(destPos)) {
    return;
}
#endif
vec2 pos = HOOKED_size * HOOKED_map(ivec2(gl_GlobalInvocationID));
vec2 subpix = fract(pos - 0.5);
pos -= subpix;
subpix = LUTPOS(subpix, vec2(9.0, 9.0));
vec2 subpix_inv = 1.0 - subpix;
subpix /= vec2(2.0, 288.0);
subpix_inv /= vec2(2.0, 288.0);
ivec2 ipos = ivec2(floor(pos)) - rectl;
int lpos = ipos.x + ipos.y * 36;
vec3 sample0 = samples[-37 + lpos];
vec3 sample1 = samples[-1 + lpos];
vec3 sample2 = samples[35 + lpos];
vec3 sample3 = samples[71 + lpos];
vec3 sample4 = samples[-36 + lpos];
vec3 sample5 = samples[0 + lpos];
vec3 sample6 = samples[36 + lpos];
vec3 sample7 = samples[72 + lpos];
vec3 sample8 = samples[-35 + lpos];
vec3 sample9 = samples[1 + lpos];
vec3 sample10 = samples[37 + lpos];
vec3 sample11 = samples[73 + lpos];
vec3 sample12 = samples[-34 + lpos];
vec3 sample13 = samples[2 + lpos];
vec3 sample14 = samples[38 + lpos];
vec3 sample15 = samples[74 + lpos];
float luma0 = dot(sample0, color_primary);
float luma1 = dot(sample1, color_primary);
float luma2 = dot(sample2, color_primary);
float luma3 = dot(sample3, color_primary);
float luma4 = dot(sample4, color_primary);
float luma5 = dot(sample5, color_primary);
float luma6 = dot(sample6, color_primary);
float luma7 = dot(sample7, color_primary);
float luma8 = dot(sample8, color_primary);
float luma9 = dot(sample9, color_primary);
float luma10 = dot(sample10, color_primary);
float luma11 = dot(sample11, color_primary);
float luma12 = dot(sample12, color_primary);
float luma13 = dot(sample13, color_primary);
float luma14 = dot(sample14, color_primary);
float luma15 = dot(sample15, color_primary);
vec3 abd = vec3(0.0, 0.0, 0.0);
float gx, gy;
gx = (luma4-luma0);
gy = (luma1-luma0);
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
gx = (luma5-luma1);
gy = (luma2-luma0)/2.0;
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
gx = (luma6-luma2);
gy = (luma3-luma1)/2.0;
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
gx = (luma7-luma3);
gy = (luma3-luma2);
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
gx = (luma8-luma0)/2.0;
gy = (luma5-luma4);
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
gx = (luma9-luma1)/2.0;
gy = (luma6-luma4)/2.0;
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
gx = (luma10-luma2)/2.0;
gy = (luma7-luma5)/2.0;
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
gx = (luma11-luma3)/2.0;
gy = (luma7-luma6);
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
gx = (luma12-luma4)/2.0;
gy = (luma9-luma8);
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
gx = (luma13-luma5)/2.0;
gy = (luma10-luma8)/2.0;
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
gx = (luma14-luma6)/2.0;
gy = (luma11-luma9)/2.0;
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.07901060453704994;
gx = (luma15-luma7)/2.0;
gy = (luma11-luma10);
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
gx = (luma12-luma8);
gy = (luma13-luma12);
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
gx = (luma13-luma9);
gy = (luma14-luma12)/2.0;
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
gx = (luma14-luma10);
gy = (luma15-luma13)/2.0;
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.06153352068439959;
gx = (luma15-luma11);
gy = (luma15-luma14);
abd += vec3(gx * gx, gx * gy, gy * gy) * 0.04792235409415088;
float a = abd.x, b = abd.y, d = abd.z;
float T = a + d, D = a * d - b * b;
float delta = sqrt(max(T * T / 4.0 - D, 0.0));
float L1 = T / 2.0 + delta, L2 = T / 2.0 - delta;
float sqrtL1 = sqrt(L1), sqrtL2 = sqrt(L2);
float theta = mix(mod(atan(L1 - a, b) + 3.141592653589793, 3.141592653589793), 0.0, abs(b) < 1.192092896e-7);
float lambda = sqrtL1;
float mu = mix((sqrtL1 - sqrtL2) / (sqrtL1 + sqrtL2), 0.0, sqrtL1 + sqrtL2 < 1.192092896e-7);
float angle = floor(theta * 24.0 / 3.141592653589793);
float strength = mix(mix(0.0, 1.0, lambda >= 0.004), mix(2.0, 3.0, lambda >= 0.05), lambda >= 0.016);
float coherence = mix(mix(0.0, 1.0, mu >= 0.25), 2.0, mu >= 0.5);
float coord_y = ((angle * 4.0 + strength) * 3.0 + coherence) / 288.0;
vec3 res = vec3(0.0, 0.0, 0.0);
vec4 w;
w = texture(ravu_zoom_lut2, vec2(0.0, coord_y) + subpix);
res += sample0 * w[0];
res += sample1 * w[1];
res += sample2 * w[2];
res += sample3 * w[3];
w = texture(ravu_zoom_lut2, vec2(0.5, coord_y) + subpix);
res += sample4 * w[0];
res += sample5 * w[1];
res += sample6 * w[2];
res += sample7 * w[3];
w = texture(ravu_zoom_lut2, vec2(0.0, coord_y) + subpix_inv);
res += sample15 * w[0];
res += sample14 * w[1];
res += sample13 * w[2];
res += sample12 * w[3];
w = texture(ravu_zoom_lut2, vec2(0.5, coord_y) + subpix_inv);
res += sample11 * w[0];
res += sample10 * w[1];
res += sample9 * w[2];
res += sample8 * w[3];
res = clamp(res, 0.0, 1.0);
imageStore(out_image, ivec2(gl_GlobalInvocationID), res);
}
