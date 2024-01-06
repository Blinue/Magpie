#!/usr/bin/env python3
#
# Copyright (C) 2017 Bin Jin <bjin@ctrl-d.org>
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

import enum
import math

import userhook

from common import FloatFormat
from magpie import MagpieBase, MagpieHook


class Step(enum.Enum):
    step1 = 0
    step2 = 1


class RAVU_Lite(userhook.UserHook):
    """
    A faster and luma-only variant of RAVU
    """


    def __init__(self,
                 weights_file=None,
                 lut_name="ravu_lite_lut",
                 int_tex_name="ravu_lite_int",
                 anti_ringing=None,
                 **args):
        super().__init__(**args)

        exec(open(weights_file).read())

        self.radius = locals()['radius']
        self.gradient_radius = locals()['gradient_radius']
        self.quant_angle = locals()['quant_angle']
        self.quant_strength = locals()['quant_strength']
        self.quant_coherence = locals()['quant_coherence']
        self.min_strength = locals()['min_strength']
        self.min_coherence = locals()['min_coherence']
        self.gaussian = locals()['gaussian']
        self.model_weights = locals()['model_weights']

        assert len(self.min_strength) + 1 == self.quant_strength
        assert len(self.min_coherence) + 1 == self.quant_coherence

        self.lut_name = "%s%d" % (lut_name, self.radius)
        self.int_tex_name = int_tex_name

        n = self.radius * 2 - 1

        self.lut_height = self.quant_angle * self.quant_strength * self.quant_coherence
        self.lut_width = (n * n + 1) // 2

        self.anti_ringing = anti_ringing

    def generate_tex(self, float_format=FloatFormat.float32):
        import struct

        tex_format, item_format_str = {
            FloatFormat.float16gl: ("rgba16f", 'f'),
            FloatFormat.float16vk: ("rgba16hf", 'e'),
            FloatFormat.float32:   ("rgba32f", 'f')
        }[float_format]

        weights = self.weights()

        assert len(weights) == self.lut_width * self.lut_height * 4
        weights_raw = struct.pack('<%d%s' % (len(weights), item_format_str), *weights).hex()

        headers = [
            "//!TEXTURE %s" % self.lut_name,
            "//!SIZE %d %d" % (self.lut_width, self.lut_height),
            "//!FORMAT %s" % tex_format,
            "//!FILTER NEAREST"
        ]

        return "\n".join(headers + [weights_raw, ""])

    def weights(self):
        weights = []
        for i in range(self.quant_angle):
            for j in range(self.quant_strength):
                for k in range(self.quant_coherence):
                    w = self.model_weights[i][j][k]
                    for pos in range(self.lut_width):
                        for z in range(4):
                            assert abs(w[z][pos] - w[~z][~pos]) < 1e-6, "filter kernel is not symmetric"
                            weights.append((w[z][pos] + w[~z][~pos]) / 2.0)
        return weights

    def extract_key(self, samples_list):
        GLSL = self.add_glsl
        n = self.radius * 2 - 1

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
                    return "(%s-%s)/2.0" % (f(x + 1), f(x - 1))

                GLSL("gx = %s;" % numerial_differential(
                    lambda i2: samples_list[i2 * n + j], i))
                GLSL("gy = %s;" % numerial_differential(
                    lambda j2: samples_list[i * n + j2], j))
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
        n = self.radius * 2 - 1

        GLSL("float coord_y = ((angle * %d.0 + strength) * %d.0 + coherence + 0.5) / %d.0;" %
             (self.quant_strength, self.quant_coherence, self.quant_angle * self.quant_strength * self.quant_coherence))

        GLSL("vec4 res = vec4(0.0, 0.0, 0.0, 0.0), w;")

        if self.anti_ringing:
            GLSL("vec4 lo = vec4(0.0, 0.0, 0.0, 0.0), hi = vec4(0.0, 0.0, 0.0, 0.0), lo2 = vec4(0.0, 0.0, 0.0, 0.0), hi2 = vec4(0.0, 0.0, 0.0, 0.0), wg, cg4, cg4_1;")
            in_ar_kernel = [None] * self.lut_width
            for i in range(self.lut_width):
                dx = i // n - n // 2
                dy = i % n - n // 2
                in_ar_kernel[i] = dx ** 2 + dy ** 2 <= 4

        for i in range(self.lut_width):
            use_ar = self.anti_ringing and in_ar_kernel[i]
            coord_x = (float(i) + 0.5) / float(self.lut_width)
            GLSL("w = texture(%s, vec2(%s, coord_y));" % (self.lut_name, coord_x))
            j = n * n - 1 - i
            if use_ar:
                GLSL("wg = max(vec4(0.0, 0.0, 0.0, 0.0), w);");

            if i < j:
                GLSL("res += %s * w + %s * w.wzyx;" % (samples_list[i], samples_list[j]))
                if use_ar:
                    GLSL("cg4 = vec4(0.1 + %s, 1.1 - %s, 0.1 + %s, 1.1 - %s);" % (samples_list[i], samples_list[i], samples_list[j], samples_list[j]))
                    GLSL("cg4_1 = cg4;")
                    GLSL("cg4 *= cg4;" * 5)
                    GLSL("hi += cg4.x * wg + cg4.z * wg.wzyx;")
                    GLSL("lo += cg4.y * wg + cg4.w * wg.wzyx;")
                    GLSL("cg4 *= cg4_1;")
                    GLSL("hi2 += cg4.x * wg + cg4.z * wg.wzyx;")
                    GLSL("lo2 += cg4.y * wg + cg4.w * wg.wzyx;")
            elif i == j:
                GLSL("res += %s * w;" % samples_list[i])
                if use_ar:
                    GLSL("vec2 cg2 = vec2(0.1 + %s, 1.1 - %s);" % (samples_list[i], samples_list[i]))
                    GLSL("vec2 cg2_1 = cg2;")
                    GLSL("cg2 *= cg2;" * 5)
                    GLSL("hi += cg2.x * wg;")
                    GLSL("lo += cg2.y * wg;")
                    GLSL("cg2 *= cg2_1;")
                    GLSL("hi2 += cg2.x * wg;")
                    GLSL("lo2 += cg2.y * wg;")

        if self.anti_ringing:
            GLSL("lo = 1.1 - lo2 / lo;")
            GLSL("hi = hi2 / hi - 0.1;")
            GLSL("res = mix(res, clamp(res, lo, hi), %f);" % self.anti_ringing)
        else:
            GLSL("res = clamp(res, 0.0, 1.0);")

    def generate(self, step, use_gather=False):
        self.reset()
        GLSL = self.add_glsl
        n = self.radius * 2 - 1

        self.set_description("RAVU-Lite%s (%s, r%d)" % ("-AR" if self.anti_ringing else "", step.name, self.radius))

        self.set_skippable(2, 2)

        if step == Step.step2:
            self.set_transform(2, 2, 0.0, 0.0)

            self.bind_tex(self.int_tex_name)
            self.set_components(1)

            GLSL("""
vec4 hook() {
    vec2 dir = fract(HOOKED_pos * HOOKED_size) - 0.5;
    int idx = int(dir.x > 0.0) * 2 + int(dir.y > 0.0);
    return vec4(%s_texOff(-dir)[idx], 0.0, 0.0, 0.0);
}
""" % self.int_tex_name)

            return super().generate()

        self.bind_tex(self.lut_name)
        self.save_tex(self.int_tex_name)
        self.set_components(4)

        GLSL("""
vec4 hook() {""")

        gather_offsets = [(0, 1), (1, 1), (1, 0), (0, 0)]

        samples_list = {}
        for i in range(n):
            for j in range(n):
                dx, dy = i - (self.radius - 1), j - (self.radius - 1)
                idx = i * n + j
                if idx in samples_list:
                    continue
                if use_gather and i + 1 < n and j + 1 < n:
                    gather_name = "gather%d" % idx
                    GLSL("vec4 %s = HOOKED_mul * textureGatherOffset(HOOKED_raw, HOOKED_pos, ivec2(%d, %d), 0);" % (gather_name, dx, dy))
                    for k in range(4):
                        ox, oy = gather_offsets[k]
                        samples_list[(i + ox) * n + (j + oy)] = "%s.%s" % (gather_name, "xyzw"[k])
                elif use_gather and i + 1 < n:
                    gather_name = "gather%d" % idx
                    GLSL("vec2 %s = HOOKED_mul * textureGatherOffset(HOOKED_raw, HOOKED_pos, ivec2(%d, %d), 0).wz;" % (gather_name, dx, dy))
                    samples_list[idx] = "%s.x" % gather_name
                    samples_list[idx + n] = "%s.y" % gather_name
                elif use_gather and j + 1 < n:
                    gather_name = "gather%d" % idx
                    GLSL("vec2 %s = HOOKED_mul * textureGatherOffset(HOOKED_raw, HOOKED_pos, ivec2(%d, %d), 0).wx;" % (gather_name, dx, dy))
                    samples_list[idx] = "%s.x" % gather_name
                    samples_list[idx + 1] = "%s.y" % gather_name
                else:
                    sample_name = "luma%d" % idx
                    GLSL("float %s = HOOKED_texOff(vec2(%d.0, %d.0)).x;" % (sample_name, dx, dy))
                    samples_list[idx] = sample_name

        self.extract_key(samples_list)

        self.apply_convolution_kernel(samples_list)

        GLSL("""
return res;
}""")

        return super().generate()

    def function_header_compute(self):
        GLSL = self.add_glsl

        GLSL("""
void hook() {""")

    def samples_loop(self, array_size, offset_base):
        GLSL = self.add_glsl

        GLSL("""
for (int id = int(gl_LocalInvocationIndex); id < %d; id += int(gl_WorkGroupSize.x * gl_WorkGroupSize.y)) {""" % (array_size[0] * array_size[1]))

        GLSL("int x = id / %d, y = id %% %d;" % (array_size[1], array_size[1]))

        GLSL("inp[id] = HOOKED_tex(HOOKED_pt * vec2(float(group_base.x+x)+(%s), float(group_base.y+y)+(%s))).x;" %
             (offset_base + 0.5, offset_base + 0.5))

        GLSL("""
}""")

    def check_viewport(self):
        # not needed for mpv
        pass

    def generate_compute(self, step, block_size):
        # compute shader requires only one step
        if step != Step.step1:
            return ""

        self.reset()
        GLSL = self.add_glsl
        n = self.radius * 2 - 1

        self.set_description("RAVU-Lite%s (r%d, compute)" % ("-AR" if self.anti_ringing else "", self.radius))

        block_width, block_height = block_size

        self.set_skippable(2, 2)
        self.set_transform(2, 2, 0.0, 0.0)
        self.bind_tex(self.lut_name)
        self.set_compute(block_width * 2, block_height * 2,
                         block_width, block_height)

        offset_base = -(self.radius - 1)
        array_size = block_width + n - 1, block_height + n - 1
        GLSL("shared float inp[%d];" % (array_size[0] * array_size[1]))

        self.function_header_compute()

        # load all samples
        GLSL("ivec2 group_base = ivec2(gl_WorkGroupID) * ivec2(gl_WorkGroupSize);")
        GLSL("int local_pos = int(gl_LocalInvocationID.x) * %d + int(gl_LocalInvocationID.y);" % array_size[1])

        self.samples_loop(array_size, offset_base)

        GLSL("barrier();")

        self.check_viewport()

        samples_list = []
        for dx in range(1 - self.radius, self.radius):
            for dy in range(1 - self.radius, self.radius):
                offset = (dx - offset_base) * array_size[1] + (dy - offset_base)
                samples_list.append("inp[local_pos + %d]" % offset)

        self.extract_key(samples_list)

        self.apply_convolution_kernel(samples_list)

        for i in range(4):
            pos = "ivec2(gl_GlobalInvocationID) * 2 + ivec2(%d, %d)" % (i // 2, i % 2)
            GLSL("imageStore(out_image, %s, vec4(res[%d], 0.0, 0.0, 0.0));" % (pos, i))

        GLSL("""
}""")

        return super().generate()


class Magpie_RAVU_Lite(MagpieBase, RAVU_Lite, MagpieHook):
    @staticmethod
    def main(args):
        hook = ["INPUT"]
        weights_file = args.weights_file[0]
        max_downscaling_ratio = args.max_downscaling_ratio[0]
        assert max_downscaling_ratio is None
        assert not args.use_gather
        assert args.use_compute_shader
        compute_shader_block_size = args.compute_shader_block_size
        anti_ringing = args.anti_ringing[0]
        float_format = FloatFormat[args.float_format[0]]
        assert float_format in [FloatFormat.float16dx, FloatFormat.float32dx]

        gen = Magpie_RAVU_Lite(
            hook=hook,
            target_tex="OUTPUT",
            weights_file=weights_file,
            max_downscaling_ratio=max_downscaling_ratio,
            anti_ringing=anti_ringing
        )

        shader  = gen.magpie_header()
        shader += gen.tex_headers("INPUT", filter="POINT")
        shader += gen.sampler_headers("INPUT_LINEAR", filter="LINEAR")
        shader += gen.generate_tex(float_format, overwrite=args.overwrite)
        shader += gen.hlsl_defines()
        for step in list(Step):
            shader += gen.generate_compute(step, compute_shader_block_size)
        shader = gen.finish(shader)
        sys.stdout.write(shader)

    def generate_tex(self, float_format, **kwargs):
        weights = self.weights()
        return self.generate_tex_magpie(
            self.lut_name,
            weights,
            self.lut_width,
            self.lut_height,
            float_format=float_format,
            **kwargs
        )

    def samples_loop(self, array_size, offset_base):
        GLSL = self.add_glsl

        GLSL("""#pragma warning(disable: 3557)
for (int id = int(gl_LocalInvocationIndex); id < %d; id += int(gl_WorkGroupSize.x * gl_WorkGroupSize.y)) {""" % (array_size[0] * array_size[1]))

        GLSL("uint x = (uint)id / %d, y = (uint)id %% %d;" % (array_size[1], array_size[1]))

        GLSL("inp[id] = HOOKED_tex(HOOKED_pt * vec2(float(group_base.x+x)+(%s), float(group_base.y+y)+(%s))).x;" %
             (offset_base + 0.5, offset_base + 0.5))

        GLSL("""
}""")

    def check_viewport(self):
        GLSL = self.add_glsl

        GLSL("""
#if CURRENT_PASS == LAST_PASS
uint2 destPos = blockStart + threadId.xy * 2;
if (!CheckViewport(destPos)) {
    return;
}
#endif
""")


if __name__ == "__main__":
    import argparse
    import sys

    parser = argparse.ArgumentParser(
        description="generate RAVU-Lite user shader for mpv")
    parser.add_argument(
        '-w',
        '--weights-file',
        nargs=1,
        required=True,
        type=str,
        help='weights file name')
    parser.add_argument(
        '-r',
        '--max-downscaling-ratio',
        nargs=1,
        type=float,
        default=[None],
        help='allowed downscaling ratio (default: no limit)')
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
    parser.add_argument(
        '--use-magpie',
        action='store_true',
        help="enable Magpie mode")

    magpie_group = parser.add_argument_group('Magpie options', "Magpie options are only valid in Magpie mode")
    magpie_group.add_argument(
        '--overwrite',
        action="store_true",
        help="Overwrite existing .dds lut-textures in the current directory (default: disabled)"
    )

    args = parser.parse_args()
    if args.use_magpie:
        Magpie_RAVU_Lite.main(args)
        exit(0)

    weights_file = args.weights_file[0]
    max_downscaling_ratio = args.max_downscaling_ratio[0]
    use_gather = args.use_gather
    use_compute_shader = args.use_compute_shader
    compute_shader_block_size = args.compute_shader_block_size
    anti_ringing = args.anti_ringing[0]
    float_format = FloatFormat[args.float_format[0]]

    gen = RAVU_Lite(hook=["LUMA"],
                    weights_file=weights_file,
                    target_tex="OUTPUT",
                    max_downscaling_ratio=max_downscaling_ratio,
                    anti_ringing=anti_ringing)

    sys.stdout.write(userhook.LICENSE_HEADER)
    for step in list(Step):
        if use_compute_shader:
            shader = gen.generate_compute(step, compute_shader_block_size)
        else:
            shader = gen.generate(step, use_gather)
        sys.stdout.write(shader)
    sys.stdout.write(gen.generate_tex(float_format))
