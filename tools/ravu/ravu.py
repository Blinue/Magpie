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

from common import FloatFormat, Profile
from magpie import MagpieBase, MagpieHook


class Step(enum.Enum):
    step1 = 0
    step2 = 1
    step3 = 2
    step4 = 3


class RAVU(userhook.UserHook):
    """
    An experimental prescaler inspired by RAISR (Rapid and Accurate Image Super
    Resolution).
    """

    def __init__(self,
                 profile=Profile.luma,
                 weights_file=None,
                 lut_name="ravu_lut",
                 int_tex_name="ravu_int",
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

        self.profile = profile
        self.lut_name = "%s%d" % (lut_name, self.radius)

        self.tex_name = [["HOOKED", int_tex_name + "01"],
                         [int_tex_name + "10", int_tex_name + "11"]]

        self.lut_height = self.quant_angle * self.quant_strength * self.quant_coherence
        self.lut_width = ((self.radius * 2) ** 2 // 2 + 3) // 4

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
                    kernel = self.model_weights[i][j][k]
                    assert len(kernel) == (self.radius * 2) ** 2
                    kernel_sum = sum(kernel)
                    kernel2 = []
                    for idx in range(len(kernel) // 2):
                        assert abs(kernel[idx] - kernel[~idx]) < 1e-6, "filter kernel is not symmetric"
                        kernel2.append((kernel[idx] + kernel[~idx]) / kernel_sum / 2.0)
                    while len(kernel2) % 4 != 0:
                        kernel2.append(0.0)
                    weights.extend(kernel2)
        return weights

    def get_id_from_texname(self, tex_name):
        for i in range(2):
            for j in range(2):
                if self.tex_name[i][j] == tex_name:
                    return (i, j)
        raise Exception("unknown texture %s" % tex_name)

    def get_chroma_offset(self):
        if self.profile == Profile.chroma_left:
            return (-0.5, 0.0)
        if self.profile == Profile.chroma_center:
            return (0.0, 0.0)
        raise Exception("not a chroma profile: %s" % self.profile)

    def is_luma_required(self, x, y):
        n = self.radius * 2

        border_width = self.radius - self.gradient_radius

        return min(x, n - 1 - x) >= border_width or min(y, n - 1 - y) >= border_width

    def get_sample_positions(self, offset, use_gather=False):
        n = self.radius * 2

        if offset == (1, 1):
            pos_func = lambda x, y: (self.tex_name[0][0], x - (n // 2 - 1), y - (n // 2 - 1))
        elif offset == (0, 1) or offset == (1, 0):
            def pos_func(x, y):
                x, y = x + y - (n - 1), y - x
                x += offset[0]
                y += offset[1]
                assert x % 2 == y % 2
                return (self.tex_name[x % 2][y % 2], x // 2, y // 2)
        else:
            raise Exception("invalid offset")

        sample_positions = {}
        for i in range(n):
            for j in range(n):
                tex, x, y = pos_func(i, j)
                # tex_name, tex_offset -> logical offset
                sample_positions.setdefault(tex, {})[x, y] = i, j

        gathered_groups = {}
        gathered_positions = {}
        if use_gather:
            gather_offsets = [(0, 1), (1, 1), (1, 0), (0, 0)]
            for tex in sorted(sample_positions.keys()):
                mapping = sample_positions[tex]
                used_keys = set()
                for x, y in sorted(mapping.keys()):
                    # (x, y) should be the minimum among |tex_offsets|
                    tex_offsets = [(x + dx, y + dy) for dx, dy in gather_offsets]
                    if all(key in mapping and key not in used_keys for key in tex_offsets):
                        used_keys |= set(tex_offsets)
                        logical_offsets = [mapping[key] for key in tex_offsets]
                        # tex_name, tex_offset_base -> logical offset
                        gathered_groups.setdefault(tex, {})[x, y] = logical_offsets
                gathered_positions[tex] = used_keys

        return sample_positions, gathered_positions, gathered_groups

    def setup_condition(self):
        if self.profile not in [Profile.chroma_left, Profile.chroma_center]:
            # This checks against all passes, and works since "HOOKED" is same for
            # all of them.
            self.set_skippable(2, 2)
        else:
            # Check upscaled CHROMA size against LUMA, instead of OUTPUT. Use
            # a value slightly less than 2 to make inequation hold.
            self.add_cond("LUMA.w CHROMA.w 1.8 * >")
            self.add_cond("LUMA.h CHROMA.h 1.8 * >")

    def setup_profile(self):
        GLSL = self.add_glsl

        if self.profile == Profile.luma:
            self.add_mappings(
                sample_type="float",
                sample_zero="0.0",
                hook_return_value="vec4(res, 0.0, 0.0, 0.0)",
                comps_swizzle = ".x")
        elif self.profile in [Profile.rgb, Profile.yuv]:
            self.add_mappings(
                sample_type="vec3",
                sample_zero="vec3(0.0)",
                hook_return_value="vec4(res, 1.0)",
                comps_swizzle = ".xyz")
            if self.profile == Profile.rgb:
                # Assumes Rec. 709
                GLSL("const vec3 color_primary = vec3(0.2126, 0.7152, 0.0722);")
            elif self.profile == Profile.yuv:
                self.assert_yuv()
        else:
            self.add_mappings(
                sample_type="vec2",
                sample_zero="vec2(0.0)",
                hook_return_value="vec4(res, 0.0, 0.0)",
                comps_swizzle = ".xy")
            self.bind_tex("LUMA")

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
        if self.min_strength == [0.001, 0.002, 0.004, 0.008, 0.016, 0.032, 0.064, 0.128]:
            GLSL("float strength = clamp(floor(log2(lambda * 2000.0 + %s)), 0.0, 8.0);" % eps)
        else:
            GLSL("float strength = %s;" % quantize("lambda", self.min_strength, 0, self.quant_strength - 1))
        GLSL("float coherence = %s;" % quantize("mu", self.min_coherence, 0, self.quant_coherence - 1))

    def apply_convolution_kernel(self, samples_list):
        GLSL = self.add_glsl
        n = self.radius * 2

        assert len(samples_list) == n * n

        GLSL("float coord_y = ((angle * %d.0 + strength) * %d.0 + coherence + 0.5) / %d.0;" %
             (self.quant_strength, self.quant_coherence, self.quant_angle * self.quant_strength * self.quant_coherence))

        GLSL("$sample_type res = $sample_zero;")
        GLSL("vec4 w;")
        for i in range(len(samples_list) // 2):
            if i % 4 == 0:
                coord_x = (float(i // 4) + 0.5) / float(self.lut_width)
                GLSL("w = texture(%s, vec2(%s, coord_y));" % (self.lut_name, coord_x))
            GLSL("res += (%s + %s) * w[%d];" % (samples_list[i], samples_list[~i], i % 4))
        GLSL("res = clamp(res, 0.0, 1.0);")


    def generate(self, step, use_gather=False):
        self.reset()
        GLSL = self.add_glsl

        self.set_description("RAVU (%s, %s, r%d)" %
                             (step.name, self.profile.name, self.radius))

        self.setup_condition()

        if step == Step.step4:
            self.set_transform(2, 2, -0.5, -0.5)

            self.bind_tex(self.tex_name[0][1])
            self.bind_tex(self.tex_name[1][0])
            self.bind_tex(self.tex_name[1][1])

            # FIXME: get rid of branching (is it even possible?)
            GLSL("""
vec4 hook() {
    vec2 dir = fract(HOOKED_pos * HOOKED_size) - 0.5;
    if (dir.x < 0.0) {
        if (dir.y < 0.0)
            return %s_texOff(-dir);
        return %s_texOff(-dir);
    } else {
        if (dir.y < 0.0)
            return %s_texOff(-dir);
        return %s_texOff(-dir);
    }
}
""" % (self.tex_name[0][0], self.tex_name[0][1],
       self.tex_name[1][0], self.tex_name[1][1]))

            return super().generate()

        self.bind_tex(self.lut_name)

        self.setup_profile()

        n = self.radius * 2
        samples = {(x, y): "sample%d" % (x * n + y) for x in range(n) for y in range(n)}

        if self.profile == Profile.luma:
            luma = lambda x, y: samples[x, y]
        elif self.profile == Profile.yuv:
            def luma(x, y):
                s = samples[x, y]
                if s.startswith("sample"):
                    return s + ".x"
                # sample is combined from gathered components, use first
                # arguments of "vec3()" directly for luma component
                return s[s.find('(') + 1 : s.find(',')]
        else:
            luma = lambda x, y: "luma%d" % (x * n + y)

        if step == Step.step1:
            target_offset = (1, 1)
        else:
            self.bind_tex(self.tex_name[1][1])
            if step == Step.step2:
                target_offset = (1, 0)
            elif step == Step.step3:
                target_offset = (0, 1)

        self.save_tex(self.tex_name[target_offset[0]][target_offset[1]])

        GLSL("""
vec4 hook() {""")

        sample_positions, gathered_positions, gathered_groups = self.get_sample_positions(target_offset, use_gather)

        gathered = 0
        for tex in sorted(gathered_groups.keys()):
            mapping = gathered_groups[tex]
            for base_x, base_y in sorted(mapping.keys()):
                logical_offsets = mapping[base_x, base_y]
                if self.profile == Profile.luma:
                    gathered_names = ["gathered%d" % gathered]
                elif self.profile == Profile.yuv:
                    gathered_names = ["gathered%d_y" % gathered, "gathered%d_u" % gathered, "gathered%d_v" % gathered]
                elif self.profile == Profile.rgb:
                    gathered_names = ["gathered%d_r" % gathered, "gathered%d_g" % gathered, "gathered%d_b" % gathered]
                else:
                    gathered_names = ["gathered%d_u" % gathered, "gathered%d_v" % gathered]
                gathered += 1
                for comp, gathered_name in enumerate(gathered_names):
                    GLSL("vec4 %s = %s_mul * textureGatherOffset(%s_raw, %s_pos, ivec2(%d, %d), %d);" %
                         (gathered_name, tex, tex, tex, base_x, base_y, comp))
                for idx in range(len(logical_offsets)):
                    i, j = logical_offsets[idx]
                    templ = "$sample_type(%s)" if len(gathered_names) > 1 else "%s"
                    samples[i, j] = templ % ", ".join("%s[%d]" % (gathered_name, idx) for gathered_name in gathered_names)

        for tex in sorted(sample_positions.keys()):
            mapping = sample_positions[tex]
            already_gathered = gathered_positions.get(tex, set())
            for x, y in sorted(mapping.keys()):
                i, j = mapping[x, y]
                if (x, y) not in already_gathered:
                    GLSL('$sample_type %s = %s_texOff(vec2(%d.0, %d.0))$comps_swizzle;' %
                         (samples[i, j], tex, x, y))
                if not self.is_luma_required(i, j):
                    continue
                if self.profile == Profile.rgb:
                    GLSL('float %s = dot(%s, color_primary);' % (luma(i, j), samples[i, j]))
                elif self.profile in [Profile.chroma_left, Profile.chroma_center]:
                    chroma_offset = self.get_chroma_offset()
                    bound_tex_id = self.get_id_from_texname(tex)
                    # |(x, y) * 2 + bound_tex_id| is the offset of luma texel
                    # relative to upscaled HOOKED texels.
                    offset_x = x * 2 + bound_tex_id[0] - chroma_offset[0]
                    offset_y = y * 2 + bound_tex_id[1] - chroma_offset[1]
                    # use bilinear sampling for luma downscaling
                    GLSL('float %s = LUMA_tex(HOOKED_pos+LUMA_pt*(vec2(%s,%s)+tex_offset)).x;' % (luma(i, j), offset_x, offset_y))

        self.extract_key(luma)

        samples_list = [samples[i, j] for i in range(n) for j in range(n)]
        self.apply_convolution_kernel(samples_list)

        GLSL("""
return $hook_return_value;
}""")

        return super().generate()

    def function_header_compute(self):
        GLSL = self.add_glsl

        GLSL("""
void hook() {""")

    def samples_loop(self, array_size, offset_base, tex_idx, tex):
        GLSL = self.add_glsl

        GLSL("""
for (int id = int(gl_LocalInvocationIndex); id < %d; id += int(gl_WorkGroupSize.x * gl_WorkGroupSize.y)) {""" % (array_size[0] * array_size[1]))

        GLSL("int x = id / %d, y = id %% %d;" % (array_size[1], array_size[1]))

        GLSL("inp%d[id] = %s_tex(%s_pt * vec2(float(group_base.x+x)+(%s), float(group_base.y+y)+(%s)))$comps_swizzle;" %
                (tex_idx, tex, tex, offset_base[0] + 0.5, offset_base[1] + 0.5))

        if self.profile == Profile.yuv:
            GLSL("inp_luma%d[id] = inp%d[id].x;" % (tex_idx, tex_idx))
        elif self.profile == Profile.rgb:
            GLSL("inp_luma%d[id] = dot(inp%d[id], color_primary);" % (tex_idx, tex_idx))
        elif self.profile in [Profile.chroma_left, Profile.chroma_center]:
            chroma_offset = self.get_chroma_offset()
            bound_tex_id = self.get_id_from_texname(tex)
            # |(group_base+(x,y)+offset_base)*2+bound_tex_id| is the luma texel id
            offset_x = offset_base[0] * 2 + bound_tex_id[0] + 0.5 - chroma_offset[0]
            offset_y = offset_base[1] * 2 + bound_tex_id[1] + 0.5 - chroma_offset[1]
            GLSL('inp_luma%d[id] = LUMA_tex(LUMA_pt * (vec2(float(group_base.x+x)*2.0+(%s),float(group_base.y+y)*2.0+(%s))+tex_offset)).x;' %
                    (tex_idx, offset_x, offset_y))

        GLSL("""
}""")

    def check_viewport(self):
        # not needed for mpv
        pass

    def save_format(self, value):
        # not needed for mpv
        pass

    def generate_compute(self, step, block_size):
        # compute shader requires only two steps
        if step == Step.step3 or step == Step.step4:
            return ""

        self.reset()
        GLSL = self.add_glsl

        self.set_description("RAVU (%s, %s, r%d, compute)" %
                             (step.name, self.profile.name, self.radius))

        block_width, block_height = block_size

        self.setup_condition()
        self.bind_tex(self.lut_name)

        self.setup_profile()

        if step == Step.step1:
            self.set_compute(block_width, block_height)

            target_offsets = [(1, 1)]
            self.save_tex(self.tex_name[1][1])
            self.save_format(self.mappings["sample_type"])
        elif step == Step.step2:
            self.set_compute(block_width * 2, block_height * 2,
                             block_width, block_height)
            self.set_transform(2, 2, -0.5, -0.5)

            target_offsets = [(0, 1), (1, 0)]
            self.bind_tex(self.tex_name[1][1])

        n = self.radius * 2
        sample_positions_by_target = [self.get_sample_positions(target_offset, False)[0] for target_offset in target_offsets]

        # for each bound texture, declare global variables/shared arrays and
        # prepare index/samples mapping
        bound_tex_names = list(sample_positions_by_target[0].keys())
        offset_for_tex = []
        array_size_for_tex = []
        samples_mapping_for_target = [{} for sample_positions in sample_positions_by_target]
        for tex_idx, tex in enumerate(bound_tex_names):
            tex_offsets = set()
            for sample_positions in sample_positions_by_target:
                tex_offsets |= set(sample_positions[tex].keys())
            minx = min(key[0] for key in tex_offsets)
            maxx = max(key[0] for key in tex_offsets)
            miny = min(key[1] for key in tex_offsets)
            maxy = max(key[1] for key in tex_offsets)

            offset_for_tex.append((minx, miny))
            array_size = (maxx - minx + block_width, maxy - miny + block_height)
            array_size_for_tex.append(array_size)

            GLSL("shared $sample_type inp%d[%d];" % (tex_idx, array_size[0] * array_size[1]))
            if self.profile != Profile.luma:
                GLSL("shared float inp_luma%d[%d];" % (tex_idx, array_size[0] * array_size[1]))

            # Samples mapping are different for different sample_positions
            for target_idx, sample_positions in enumerate(sample_positions_by_target):
                samples_mapping = samples_mapping_for_target[target_idx]
                mapping = sample_positions[tex]
                for tex_offset in mapping.keys():
                    logical_offset = mapping[tex_offset]
                    samples_mapping[logical_offset] = "inp%d[local_pos + %d]" % \
                                                      (tex_idx, (tex_offset[0] - minx) * array_size[1] + (tex_offset[1] - miny))

        self.function_header_compute()

        # load all samples
        GLSL("ivec2 group_base = ivec2(gl_WorkGroupID) * ivec2(gl_WorkGroupSize);")
        GLSL("int local_pos = int(gl_LocalInvocationID.x) * %d + int(gl_LocalInvocationID.y);" % array_size[1])
        for tex_idx, tex in enumerate(bound_tex_names):
            offset_base = offset_for_tex[tex_idx]
            array_size = array_size_for_tex[tex_idx]
            self.samples_loop(array_size, offset_base, tex_idx, tex)

        GLSL("barrier();")

        self.check_viewport()

        for target_idx, sample_positions in enumerate(sample_positions_by_target):
            target_offset = target_offsets[target_idx]
            samples_mapping = samples_mapping_for_target[target_idx]

            GLSL("{")

            luma = lambda x, y: "luma%d" % (x * n + y)
            for sample_xy, (x, y) in sorted((samples_mapping[key], key) for key in samples_mapping.keys()):
                if not self.is_luma_required(x, y):
                    continue
                luma_xy = luma(x, y)
                if self.profile == Profile.luma:
                    GLSL("float %s = %s;" % (luma_xy, sample_xy))
                else:
                    GLSL("float %s = %s;" % (luma_xy, sample_xy.replace("inp", "inp_luma")))

            self.extract_key(luma)

            samples_list = [samples_mapping[i, j] for i in range(n) for j in range(n)]
            self.apply_convolution_kernel(samples_list)

            if step == Step.step1:
                pos = "ivec2(gl_GlobalInvocationID)"
            else:
                pos = "ivec2(gl_GlobalInvocationID) * 2 + ivec2(%d, %d)" % (target_offset[0], target_offset[1])
            GLSL("imageStore(out_image, %s, $hook_return_value);" % pos)

            GLSL("}")

        if step == Step.step2:
            GLSL("$sample_type res;")
            for tex_idx, tex in enumerate(bound_tex_names):
                offset_base = offset_for_tex[tex_idx]
                bound_tex_id = self.get_id_from_texname(tex)
                pos = "ivec2(gl_GlobalInvocationID) * 2 + ivec2(%d, %d)" % bound_tex_id
                res = "inp%d[local_pos + %d]" % (tex_idx, (-offset_base[0]) * array_size[1] + (-offset_base[1]))
                GLSL("res = %s;" % res)
                GLSL("imageStore(out_image, %s, $hook_return_value);" % pos)

        GLSL("""
}""")

        return super().generate()


class Magpie_RAVU(MagpieBase, RAVU, MagpieHook):
    @staticmethod
    def main(args, profile=Profile.luma):
        assert profile in [Profile.luma, Profile.rgb]
        hook = ["INPUT"]
        weights_file = args.weights_file[0]
        max_downscaling_ratio = args.max_downscaling_ratio[0]
        assert max_downscaling_ratio is None
        assert not args.use_gather
        assert args.use_compute_shader
        compute_shader_block_size = args.compute_shader_block_size
        float_format = FloatFormat[args.float_format[0]]
        assert float_format in [FloatFormat.float16dx, FloatFormat.float32dx]

        gen = Magpie_RAVU(
            hook=hook,
            profile=profile,
            weights_file=weights_file,
            target_tex="OUTPUT",
            max_downscaling_ratio=max_downscaling_ratio
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

    def setup_profile(self):
        GLSL = self.add_glsl

        if self.profile == Profile.luma:
            self.add_mappings(
                sample_type="float",
                sample_zero="0.0",
                hook_return_value="vec3(res, 0.0, 0.0)",
                comps_swizzle = ".x")
        elif self.profile == Profile.rgb:
            self.add_mappings(
                sample_type="vec3",
                sample_zero="vec3(0.0, 0.0, 0.0)",
                hook_return_value="res",
                comps_swizzle = ".xyz")
            GLSL("static const vec3 color_primary = vec3(0.2126, 0.7152, 0.0722);")
        else:
            assert False, "Profile not supported"

    def samples_loop(self, array_size, offset_base, tex_idx, tex):
        GLSL = self.add_glsl

        GLSL("""
{
for (int id = int(gl_LocalInvocationIndex); id < %d; id += int(gl_WorkGroupSize.x * gl_WorkGroupSize.y)) {""" % (array_size[0] * array_size[1]))

        GLSL("uint x = (uint)id / %d, y = (uint)id %% %d;" % (array_size[1], array_size[1]))

        GLSL("inp%d[id] = %s_tex(%s_pt * vec2(float(group_base.x+x)+(%s), float(group_base.y+y)+(%s)))$comps_swizzle;" %
                (tex_idx, tex, tex, offset_base[0] + 0.5, offset_base[1] + 0.5))

        if self.profile == Profile.rgb:
            GLSL("inp_luma%d[id] = dot(inp%d[id], color_primary);" % (tex_idx, tex_idx))

        GLSL("""
}
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

    profile_mapping = {
        "luma": (["LUMA"], Profile.luma),
        "rgb": (["MAIN"], Profile.rgb),
        "yuv": (["NATIVE"], Profile.yuv),
        "chroma-left": (["CHROMA"], Profile.chroma_left),
        "chroma-center": (["CHROMA"], Profile.chroma_center)
    }

    parser = argparse.ArgumentParser(
        description="generate RAVU user shader for mpv")
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
    target = args.target[0]
    hook, profile = profile_mapping[target]
    if args.use_magpie:
        Magpie_RAVU.main(args, profile=profile)
        exit(0)

    weights_file = args.weights_file[0]
    max_downscaling_ratio = args.max_downscaling_ratio[0]
    use_gather = args.use_gather
    use_compute_shader = args.use_compute_shader
    compute_shader_block_size = args.compute_shader_block_size
    float_format = FloatFormat[args.float_format[0]]

    gen = RAVU(hook=hook,
               profile=profile,
               weights_file=weights_file,
               target_tex="OUTPUT",
               max_downscaling_ratio=max_downscaling_ratio)

    sys.stdout.write(userhook.LICENSE_HEADER)
    for step in list(Step):
        if use_compute_shader:
            shader = gen.generate_compute(step, compute_shader_block_size)
        else:
            shader = gen.generate(step, use_gather)
        sys.stdout.write(shader)
    sys.stdout.write(gen.generate_tex(float_format))
