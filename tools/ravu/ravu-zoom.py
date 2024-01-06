#!/usr/bin/env python3
#
# Copyright (C) 2019 Bin Jin <bjin@ctrl-d.org>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import math

import userhook

from common import FloatFormat, Profile


class RAVU_Zoom(userhook.UserHook):
    """
    RAVU variant trained to perform arbitrary ratio upscaling.
    """

    def __init__(self,
                 profile=Profile.luma,
                 weights_file=None,
                 lut_name="ravu_zoom_lut",
                 anti_ringing=None,
                 **args):
        super().__init__(**args)

        exec(open(weights_file).read())

        self.radius = locals()['radius']
        self.lut_size = locals()['lut_size']
        self.gradient_radius = locals()['gradient_radius']
        self.quant_angle = locals()['quant_angle']
        self.quant_strength = locals()['quant_strength']
        self.quant_coherence = locals()['quant_coherence']
        self.min_strength = locals()['min_strength']
        self.min_coherence = locals()['min_coherence']
        self.gaussian = locals()['gaussian']
        self.model_weights = locals()['model_weights']
        self.model_weights_ar = locals()['model_weights_ar']

        assert len(self.min_strength) + 1 == self.quant_strength
        assert len(self.min_coherence) + 1 == self.quant_coherence

        self.profile = profile
        self.lut_name = "%s%d" % (lut_name, self.radius)
        self.lut_name_ar = self.lut_name + "_ar"

        self.lut_height = self.quant_angle * self.quant_strength * self.quant_coherence * self.lut_size
        self.lut_width = (self.radius * self.radius * 2 + 3) // 4 * self.lut_size
        self.lut_width_ar = (2 * 2 * 2 + 3) // 4 * self.lut_size

        self.lut_macro ="#define LUTPOS(x, lut_size) mix(0.5 / (lut_size), 1.0 - 0.5 / (lut_size), (x))"

        self.anti_ringing = anti_ringing

    def generate_tex(self, float_format=FloatFormat.float32, ar_kernel=False):
        import struct

        if ar_kernel:
            lut_name, model_weights, radius, lut_width = self.lut_name_ar, self.model_weights_ar, 2, self.lut_width_ar
        else:
            lut_name, model_weights, radius, lut_width = self.lut_name, self.model_weights, self.radius, self.lut_width

        tex_format, item_format_str = {
            FloatFormat.float16gl: ("rgba16f", 'f'),
            FloatFormat.float16vk: ("rgba16hf", 'e'),
            FloatFormat.float32:   ("rgba32f", 'f')
        }[float_format]

        weights = self.weights(model_weights=model_weights, radius=radius)

        assert len(weights) == lut_width * self.lut_height * 4
        weights_raw = struct.pack('<%d%s' % (len(weights), item_format_str), *weights).hex()

        headers = [
            "//!TEXTURE %s" % lut_name,
            "//!SIZE %d %d" % (lut_width, self.lut_height),
            "//!FORMAT %s" % tex_format,
            "//!FILTER LINEAR"
        ]

        return "\n".join(headers + [weights_raw, ""])

    def weights(self, model_weights, radius):
        weights = []
        for i in range(self.quant_angle):
            for j in range(self.quant_strength):
                for k in range(self.quant_coherence):
                    kernel_with_lut = model_weights[i][j][k]
                    kernel_size = radius * radius * 2
                    assert len(kernel_with_lut) == self.lut_size * self.lut_size * kernel_size

                    for u in range(self.lut_size):
                        for vec4_idx in range((kernel_size + 3) // 4):
                            for v in range(self.lut_size):
                                for vec4_dim in range(4):
                                    kernel_pos = vec4_idx * 4 + vec4_dim
                                    if kernel_pos >= kernel_size:
                                        weights.append(0.0)
                                    else:
                                        pos = (v * self.lut_size + u) * kernel_size + kernel_pos
                                        weights.append(kernel_with_lut[pos])
        return weights

    def is_luma_required(self, x, y):
        n = self.radius * 2

        border_width = self.radius - self.gradient_radius

        return min(x, n - 1 - x) >= border_width or min(y, n - 1 - y) >= border_width

    def setup_profile(self):
        GLSL = self.add_glsl

        if self.profile == Profile.luma:
            self.add_mappings(
                sample_type="float",
                sample_zero="0.0",
                sample4_type="vec4",
                hook_return_value="vec4(res, 0.0, 0.0, 0.0)",
                comps_swizzle = ".x")
        else:
            self.add_mappings(
                sample_type="vec3",
                sample_zero="vec3(0.0)",
                sample4_type="mat4x3",
                hook_return_value="vec4(res, 1.0)",
                comps_swizzle = ".xyz")
            if self.profile == Profile.rgb:
                # Assumes Rec. 709
                GLSL("const vec3 color_primary = vec3(0.2126, 0.7152, 0.0722);")
            elif self.profile == Profile.yuv:
                self.assert_yuv()

    def setup_condition(self):
        self.add_cond("HOOKED.w OUTPUT.w <")
        self.add_cond("HOOKED.h OUTPUT.h <")
        self.set_output_size("OUTPUT.w", "OUTPUT.h")
        self.align_to_reference()

    def extract_key(self, luma):
        GLSL = self.add_glsl
        n = self.radius * 2

        # Calculate local gradient
        gradient_left = self.radius - self.gradient_radius
        gradient_right = n - gradient_left

        GLSL("vec3 abd = vec3(0.0, 0.0, 0.0);")
        GLSL("float gx, gy;")
        for i in range(gradient_left, gradient_right):
            for j in range(gradient_left, gradient_right):

                def numerial_differential(f, x):
                    if x == 0:
                        return "(%s-%s)" % (f(x + 1), f(x))
                    if x == n - 1:
                        return "(%s-%s)" % (f(x), f(x - 1))
                    if x == 1 or x == n - 2:
                        return "(%s-%s)/2.0" % (f(x + 1), f(x - 1))
                    return "(-%s+8.0*%s-8.0*%s+%s)/12.0" % (f(x + 2), f(x + 1), f(x - 1), f(x - 2))

                GLSL("gx = %s;" % numerial_differential(
                    lambda i2: luma(i2, j), i))
                GLSL("gy = %s;" % numerial_differential(
                    lambda j2: luma(i, j2), j))
                gw = self.gaussian[i - gradient_left][j - gradient_left]
                GLSL("abd += vec3(gx * gx, gx * gy, gy * gy) * %s;" % gw)

        # Eigenanalysis of gradient matrix
        eps = "1.192092896e-7"
        GLSL("""
float a = abd.x, b = abd.y, d = abd.z;
float T = a + d, D = a * d - b * b;
float delta = sqrt(max(T * T / 4.0 - D, 0.0));
float L1 = T / 2.0 + delta, L2 = T / 2.0 - delta;
float sqrtL1 = sqrt(L1), sqrtL2 = sqrt(L2);
float theta = mix(mod(atan(L1 - a, b) + %s, %s), 0.0, abs(b) < %s);
float lambda = sqrtL1;
float mu = mix((sqrtL1 - sqrtL2) / (sqrtL1 + sqrtL2), 0.0, sqrtL1 + sqrtL2 < %s);
""" % (math.pi, math.pi, eps, eps))

        # Extract convolution kernel based on quantization of (angle, strength, coherence)
        def quantize(var_name, seps, l, r):
            if l == r:
                return "%d.0" % l
            m = (l + r) // 2
            return "mix(%s, %s, %s >= %s)" % (quantize(var_name, seps, l, m),
                                              quantize(var_name, seps, m + 1, r),
                                              var_name,
                                              seps[m])

        GLSL("float angle = floor(theta * %d.0 / %s);" % (self.quant_angle, math.pi))
        GLSL("float strength = %s;" % quantize("lambda", self.min_strength, 0, self.quant_strength - 1))
        GLSL("float coherence = %s;" % quantize("mu", self.min_coherence, 0, self.quant_coherence - 1))

    def apply_convolution_kernel(self, samples_list):
        GLSL = self.add_glsl
        n = self.radius * 2

        assert len(samples_list) == n * n

        GLSL("float coord_y = ((angle * %d.0 + strength) * %d.0 + coherence) / %d.0;" %
             (self.quant_strength, self.quant_coherence, self.quant_angle * self.quant_strength * self.quant_coherence))

        GLSL("$sample_type res = $sample_zero;")
        GLSL("vec4 w;")
        if self.anti_ringing:
            GLSL("$sample4_type cg, cg1;")
            GLSL("$sample_type lo = $sample_zero, hi = $sample_zero;")
            GLSL("$sample_type lo2 = $sample_zero, hi2 = $sample_zero;")
            samples_list_ar = []
            for i, sample_i in enumerate(samples_list):
                dx = i // n - n // 2
                dy = i % n - n // 2
                if -2 <= dx <= 1 and -2 <= dy <= 1:
                    samples_list_ar.append(sample_i)
        for step in range(2):
            subpix_name = ["subpix", "subpix_inv"][step]
            for i in range(len(samples_list) // 2):
                if i % 4 == 0:
                    coord_x = float(i // 4) / float(self.lut_width // self.lut_size)
                    GLSL("w = texture(%s, vec2(%s, coord_y) + %s);" % (self.lut_name, coord_x, subpix_name))
                sample_i = samples_list[[i, ~i][step]]
                GLSL("res += %s * w[%d];" % (sample_i, i % 4))

        if self.anti_ringing:
            assert len(samples_list_ar) % 4 == 0
            for step in range(2):
                subpix_name = [self.subpix_ar, self.subpix_inv_ar][step]
                last_sample = None
                for i in range(len(samples_list_ar) // 2):
                    if i % 4 == 0:
                        coord_x = float(i // 4) / float(self.lut_width_ar // self.lut_size)
                        GLSL("w = texture(%s, vec2(%s, coord_y) + %s);" % (self.lut_name_ar, coord_x, subpix_name))
                    sample_i = samples_list_ar[[i, ~i][step]]
                    if i % 2 == 0:
                        last_sample = sample_i
                    else:
                        GLSL("cg = $sample4_type(0.1 + %s, 1.1 - %s, 0.1 + %s, 1.1 - %s);" % (last_sample, last_sample, sample_i, sample_i));
                        GLSL("cg1 = cg;")
                        last_sample = None
                        if self.profile == Profile.luma:
                            GLSL("cg *= cg;" * 5)
                        else:
                            GLSL("cg = matrixCompMult(cg, cg);" * 5)
                        GLSL("hi += cg[0] * w[%d] + cg[2] * w[%d];" % (i % 4 - 1, i % 4))
                        GLSL("lo += cg[1] * w[%d] + cg[3] * w[%d];" % (i % 4 - 1, i % 4))
                        if self.profile == Profile.luma:
                            GLSL("cg *= cg1;")
                        else:
                            GLSL("cg = matrixCompMult(cg, cg1);")
                        GLSL("hi2 += cg[0] * w[%d] + cg[2] * w[%d];" % (i % 4 - 1, i % 4))
                        GLSL("lo2 += cg[1] * w[%d] + cg[3] * w[%d];" % (i % 4 - 1, i % 4))
            GLSL("hi = hi2 / hi - 0.1;")
            GLSL("lo = 1.1 - lo2 / lo;")
            GLSL("res = mix(res, clamp(res, lo, hi), %f);" % self.anti_ringing)
        else:
            GLSL("res = clamp(res, 0.0, 1.0);")

    def calculate_subpix(self):
        GLSL = self.add_glsl

        GLSL("vec2 subpix = fract(pos - 0.5);")
        GLSL("pos -= subpix;")

        GLSL("subpix = LUTPOS(subpix, vec2(%s, %s));" % (float(self.lut_size), float(self.lut_size)))
        GLSL("vec2 subpix_inv = 1.0 - subpix;")

        if self.anti_ringing:
            if self.radius == 2:
                self.subpix_ar = "subpix"
                self.subpix_inv_ar = "subpix_inv"
            else:
                block_factor_ar = float(self.lut_width_ar / self.lut_size), float(self.lut_height / self.lut_size)
                GLSL("vec2 subpix_ar = subpix / vec2(%s, %s);" % block_factor_ar)
                GLSL("vec2 subpix_inv_ar = subpix_inv / vec2(%s, %s);" % block_factor_ar)
                self.subpix_ar = "subpix_ar"
                self.subpix_inv_ar = "subpix_inv_ar"

        block_factor = float(self.lut_width / self.lut_size), float(self.lut_height / self.lut_size)
        GLSL("subpix /= vec2(%s, %s);" % block_factor)
        GLSL("subpix_inv /= vec2(%s, %s);" % block_factor)


    def generate(self, use_gather=False):
        self.reset()
        GLSL = self.add_glsl

        self.set_description("RAVU-Zoom%s (%s, r%d)" % ("-AR" if self.anti_ringing else "", self.profile.name, self.radius))

        self.bind_tex(self.lut_name)
        if self.anti_ringing:
            self.bind_tex(self.lut_name_ar)
        self.setup_profile()
        self.setup_condition()

        GLSL(self.lut_macro)

        if self.profile != Profile.luma:
            # Only use textureGather for luma
            use_gather = False

        n = self.radius * 2

        GLSL("""
vec4 hook() {""")

        GLSL("vec2 pos = HOOKED_pos * HOOKED_size;")
        self.calculate_subpix()

        gather_offsets = [(0, 1), (1, 1), (1, 0), (0, 0)]

        samples_list = [None] * (n * n)
        for i in range(n):
            for j in range(n):
                dx, dy = i - (self.radius - 1), j - (self.radius - 1)
                idx = i * n + j
                if samples_list[idx]:
                    continue
                if use_gather:
                    assert i + 1 < n and j + 1 < n
                    gather_name = "gather%d" % idx
                    GLSL("vec4 %s = HOOKED_mul * textureGatherOffset(HOOKED_raw, pos * HOOKED_pt, ivec2(%d, %d), 0);"
                            % (gather_name, dx, dy))
                    for k in range(4):
                        ox, oy = gather_offsets[k]
                        samples_list[(i + ox) * n + (j + oy)] = "%s.%s" % (gather_name, "xyzw"[k])
                else:
                    sample_name = "sample%d" % idx
                    GLSL("$sample_type %s = HOOKED_tex((pos + vec2(%s,%s)) * HOOKED_pt)$comps_swizzle;"
                            % (sample_name, float(dx), float(dy)))
                    samples_list[idx] = sample_name

        if self.profile == Profile.luma:
            luma = lambda x, y: samples_list[x * n + y]
        elif self.profile == Profile.yuv:
            luma = lambda x, y: "%s.x" % samples_list[x * n + y]
        else:
            luma = lambda x, y: "luma%d" % (x * n + y)
            for i in range(n):
                for j in range(n):
                    if not self.is_luma_required(i, j):
                        continue
                    GLSL("float %s = dot(%s, color_primary);" % (luma(i, j), samples_list[i * n + j]))

        self.extract_key(luma)

        self.apply_convolution_kernel(samples_list)

        GLSL("""
return $hook_return_value;
}""")

        return super().generate()

    def function_header_compute(self):
        GLSL = self.add_glsl

        GLSL("""
void hook() {""")

    def samples_loop(self, stride):
        GLSL = self.add_glsl

        GLSL("""
for (int id = int(gl_LocalInvocationIndex); id < rect.x * rect.y; id += int(gl_WorkGroupSize.x * gl_WorkGroupSize.y)) {
    int y = id / rect.x, x = id %% rect.x;
    samples[x + y * %d] = HOOKED_tex(HOOKED_pt * (vec2(rectl + ivec2(x, y)) + vec2(0.5,0.5)))$comps_swizzle;
}""" % stride)

    def check_viewport(self):
        # not needed for mpv
        pass

    def generate_compute(self, block_size):
        self.reset()
        GLSL = self.add_glsl

        self.set_description("RAVU-Zoom%s (%s, r%d, compute)" % ("-AR" if self.anti_ringing else "", self.profile.name, self.radius))

        self.bind_tex(self.lut_name)
        if self.anti_ringing:
            self.bind_tex(self.lut_name_ar)
        self.setup_profile()
        self.setup_condition()

        GLSL(self.lut_macro)

        block_width, block_height = block_size
        self.set_compute(block_width, block_height)

        n = self.radius * 2
        GLSL("shared $sample_type samples[%d];" % ((block_height + n) * (block_width + n)))
        stride = block_width + n

        self.function_header_compute()

        GLSL("ivec2 group_begin = ivec2(gl_WorkGroupID) * ivec2(gl_WorkGroupSize);")
        GLSL("ivec2 group_end = group_begin + ivec2(gl_WorkGroupSize) - ivec2(1, 1);")
        GLSL("ivec2 rectl = ivec2(floor(HOOKED_size * HOOKED_map(group_begin) - 0.5)) - %d;" % (self.radius - 1))
        GLSL("ivec2 rectr = ivec2(floor(HOOKED_size * HOOKED_map(group_end) - 0.5)) + %d;" % self.radius)
        GLSL("ivec2 rect = rectr - rectl + 1;")

        self.samples_loop(stride)

        GLSL("barrier();")

        self.check_viewport()

        samples = {(x, y): "sample%d" % (x * n + y) for x in range(n) for y in range(n)}

        GLSL("vec2 pos = HOOKED_size * HOOKED_map(ivec2(gl_GlobalInvocationID));")
        self.calculate_subpix()
        GLSL("ivec2 ipos = ivec2(floor(pos)) - rectl;")
        GLSL("int lpos = ipos.x + ipos.y * %d;" % stride)

        samples_list = []
        for i in range(n):
            for j in range(n):
                x = i - (self.radius - 1)
                y = j - (self.radius - 1)
                GLSL("$sample_type %s = samples[%d + lpos];" % (samples[i, j], x + y * stride))
                samples_list.append(samples[i, j])

        if self.profile == Profile.luma:
            luma = lambda x, y: samples[x, y]
        elif self.profile == Profile.yuv:
            luma = lambda x, y: "%s.x" % samples[x, y]
        else:
            luma = lambda x, y: "luma%d" % (x * n + y)
            for i in range(n):
                for j in range(n):
                    if not self.is_luma_required(i, j):
                        continue
                    GLSL("float %s = dot(%s, color_primary);" % (luma(i, j), samples[i, j]))

        self.extract_key(luma)

        self.apply_convolution_kernel(samples_list)

        GLSL("imageStore(out_image, ivec2(gl_GlobalInvocationID), $hook_return_value);")

        GLSL("""
}""")

        return super().generate()


if __name__ == "__main__":
    import argparse
    import sys

    profile_mapping = {
        "luma": (["LUMA"], Profile.luma),
        "rgb": (["MAIN"], Profile.rgb),
        "yuv": (["NATIVE"], Profile.yuv),
    }

    parser = argparse.ArgumentParser(
        description="generate RAVU-Zoom user shader for mpv")
    parser.add_argument(
        '-t',
        '--target',
        nargs=1,
        choices=sorted(profile_mapping.keys()),
        default=["rgb"],
        help='target that shader is hooked on (default: rgb)')
    parser.add_argument(
        '-w',
        '--weights-file',
        nargs=1,
        required=True,
        type=str,
        help='weights file name')
    parser.add_argument(
        '--use-gather',
        action='store_true',
        help="enable use of textureGatherOffset (requires OpenGL 4.0)")
    parser.add_argument(
        '--use-compute-shader',
        action='store_true',
        help="enable use of compute shader (requires OpenGL 4.3)")
    parser.add_argument(
        '--compute-shader-block-size',
        nargs=2,
        metavar=('block_width', 'block_height'),
        default=[32, 8],
        type=int,
        help='specify the block size of compute shader (default: 32 8)')
    parser.add_argument(
        '--anti-ringing',
        nargs=1,
        type=float,
        default=[None],
        help="enable anti-ringing (based on EWA filter anti-ringing from libplacebo) with specified strength (default: disabled)")
    parser.add_argument(
        '--float-format',
        nargs=1,
        choices=FloatFormat.__members__,
        default=["float32"],
        help="specify the float format of LUT")

    args = parser.parse_args()
    target = args.target[0]
    hook, profile = profile_mapping[target]
    weights_file = args.weights_file[0]
    use_gather = args.use_gather
    use_compute_shader = args.use_compute_shader
    compute_shader_block_size = args.compute_shader_block_size
    anti_ringing = args.anti_ringing[0]
    float_format = FloatFormat[args.float_format[0]]

    gen = RAVU_Zoom(hook=hook,
                    profile=profile,
                    weights_file=weights_file,
                    anti_ringing=anti_ringing)

    sys.stdout.write(userhook.LICENSE_HEADER)
    if use_compute_shader:
        sys.stdout.write(gen.generate_compute(compute_shader_block_size))
    else:
        sys.stdout.write(gen.generate(use_gather))
    sys.stdout.write(gen.generate_tex(float_format))
    if anti_ringing:
        sys.stdout.write(gen.generate_tex(float_format, ar_kernel=True))
