#!/usr/bin/env python3

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
import os
import struct

import userhook

#
# Copyright (c) 2016 mpv developers <mpv-team@googlegroups.com>
#

#
# NNEDI3, an intra-field deinterlacer
#
# The original filter was authored by Kevin Stone (aka. tritical) and is
# licensed under GPL2 terms:
#     http://bengal.missouri.edu/~kes25c/
#
# A LGPLv3 licensed OpenCL kernel was created by SEt:
#     http://forum.doom9.org/showthread.php?t=169766
#
# A HLSL port further modified by madshi, Shiandow and Zach Saw could be
# found at (also LGPLv3 licensed):
#     https://github.com/zachsaw/MPDN_Extensions
#


class Neurons(enum.Enum):
    nns16 = 0
    nns32 = 1
    nns64 = 2
    nns128 = 3
    nns256 = 4

    def get_neurons(self):
        return [16, 32, 64, 128, 256][self.value]


class Window(enum.Enum):
    win8x4 = 0
    win8x6 = 1

    def get_width(self):
        return 8

    def get_height(self):
        return [4, 6][self.value]


class Step(enum.Enum):
    double_y = 0
    combine_y = 1
    double_x = 2
    combine_x = 3


class NNEDI3(userhook.UserHook):

    weight_offsets = [0, 1088, 3264, 7616, 16320, 33728, 35328, 38528, 44928, 57728]
    weights = None

    weights_file = "nnedi3_weights.bin"
    weights_filesize = 83328 * 4
    weights_dirs = [os.path.join(os.path.dirname(os.path.realpath(__file__)), "weights"),
                    os.path.realpath(os.getcwd())]

    weight_fmt = struct.Struct("<i")
    assert weight_fmt.size == 4

    def __init__(self, neurons, window, int_tex_name = "nnedi3_int", **args):
        super().__init__(**args)

        self.neurons = neurons.get_neurons()
        self.window_width = window.get_width()
        self.window_height = window.get_height()
        self.offset = NNEDI3.weight_offsets[window.value * len(Neurons) +
                                            neurons.value]
        self.int_tex_name = int_tex_name

    @staticmethod
    def load_weights():
        if NNEDI3.weights:
            return
        for weights_dir in NNEDI3.weights_dirs:
            try:
                NNEDI3.weights = open(
                    os.path.join(weights_dir, NNEDI3.weights_file),
                    "rb").read()
                assert len(NNEDI3.weights) == NNEDI3.weights_filesize
                return
            except IOError:
                pass
        raise Exception("unable to load %s" % NNEDI3.weights_file)

    @staticmethod
    def weight_at(ptr):
        return NNEDI3.weight_fmt.unpack_from(NNEDI3.weights, ptr * 4)[0]

    def weightW(self, n, s, x, y):
        window_size = self.window_width * self.window_height
        ptr = self.offset + \
              (window_size * 2 + 4) * n + window_size * s + \
              x + y * self.window_width
        return self.weight_at(ptr)

    def weightWS(self, n, s, i):
        window_size = self.window_width * self.window_height
        ptr = self.offset + \
              (window_size * 2 + 4) * n + window_size * 2 + \
              i
        return self.weight_at(ptr)

    def function_header_compute(self):
        GLSL = self.add_glsl

        GLSL("void hook() {")

    def samples_loop(self, array_size, array_offset):
        GLSL = self.add_glsl

        GLSL("""
for (int id = int(gl_LocalInvocationIndex); id < %d; id += int(gl_WorkGroupSize.x * gl_WorkGroupSize.y)) {""" % (array_size[0] * array_size[1]))

        GLSL("int x = id / %d, y = id %% %d;" % (array_size[1], array_size[1]))

        GLSL("inp[id] = HOOKED_tex(HOOKED_pt * vec2(float(group_base.x+x-(%d))+0.5,float(group_base.y+y-(%d))+0.5)).x;" % array_offset)

        GLSL("""
}""")

    def check_viewport(self):
        # not needed for mpv
        pass

    def save_format(self, value):
        # not needed for mpv
        pass

    def generate(self, step, use_gather=False, use_compute=False, compute_shader_block_size=None):
        self.load_weights()
        self.reset()
        GLSL = self.add_glsl

        width = self.window_width
        height = self.window_height

        if use_compute:
            use_gather = False
            block_width, block_height = compute_shader_block_size

            # Don't inline the nnedi3() function. The benefits of inlining is
            # negligible but could confuse the nvidia's shader compiler.
            #
            # Without this, the performance on nvidia's card is unpredictable.
            # Even expanding expression could make whole shader several
            # times slower.
            #
            # This workaround is obtained by trial and error, and could be broken
            # again anytime nvidia updates its driver.
            #
            # Applied only to compute shader since:
            #   1. Only compute shader suffers from the performance issue with
            #      my setup at this moment.
            #   2. gather version failed to compile without inlining (a compiler bug)
            #      "error C5213: Component must be a constant in the range [0..3]"
            #   3. regular version is too slow without inlining
            #
            GLSL('#pragma optionNV(inline none)')

        self.set_description("NNEDI3 (%s, nns%d, win%dx%d)" %
                             (step.name, self.neurons, width, height))

        assert width % 2 == 0 and height % 2 == 0
        sample_count = width * height // 4

        if step == Step.double_y or step == Step.combine_y:
            self.add_mappings(
                double_dir="y",
                double_mul="ivec2(1, 2)",
                double_offset="ivec2(0, 1)")
            self.set_skippable(mul_y=2)
            if use_compute == (step == Step.double_y):
                self.set_transform(1, 2, 0.0, -0.5)
            if use_compute:
                self.set_compute(
                    block_width, block_height * 2,
                    block_width, block_height)
        elif step == Step.double_x or step == Step.combine_x:
            self.add_mappings(
                double_dir="x",
                double_mul="ivec2(2, 1)",
                double_offset="ivec2(1, 0)")
            self.set_skippable(mul_x=2)
            if use_compute == (step == Step.double_x):
                self.set_transform(2, 1, -0.5, 0.0)
            if use_compute:
                self.set_compute(
                    block_width * 2, block_height,
                    block_width, block_height)

        if step == Step.combine_y or step == Step.combine_x:
            if use_compute:
                return ""

            self.bind_tex(self.int_tex_name)

            # FIXME: get rid of branching (is it even possible?)
            GLSL("""
vec4 hook() {
    vec2 dir = fract(HOOKED_pos * HOOKED_size) - 0.5;
    if (dir.$double_dir < 0.0) {
        return HOOKED_texOff(-dir);
    } else {
        return %s_texOff(-dir);
    }
}
""" % self.int_tex_name)

            return super().generate()

        center_x = width // 2 - 1
        center_y = height // 2 - 1

        if not use_compute:
            self.save_tex(self.int_tex_name)

        sample_positions = {}
        for y in range(height):
            for x in range(width):
                if step == Step.double_y:
                    nx, ny = x - center_x, y - center_y
                else:
                    ny, nx = x - center_x, y - center_y
                sample_positions[nx, ny] = x, y

        gather_offsets = [(0, 1), (1, 1), (1, 0), (0, 0)]

        sampling_info = []
        while len(sample_positions) > 0:
            if use_gather:
                base = min(sample_positions.keys())
                global_pos = [(base[0] + dx, base[1] + dy) for dx, dy in gather_offsets]
            else:
                global_pos = sorted(sample_positions.keys())[:4]
            window_pos = []
            for npos in global_pos:
                window_pos.append(sample_positions.pop(npos))
            sampling_info.append((global_pos, window_pos))

        assert len(sampling_info) == sample_count

        GLSL("""
float nnedi3(vec4 samples[%d]) {""" % sample_count)


        GLSL("""
float sum = 0.0, sumsq = 0.0;
for (int i = 0; i < %d; i++) {
    sum += dot(samples[i], vec4(1.0, 1.0, 1.0, 1.0));
    sumsq += dot(samples[i], samples[i]);
}""" % sample_count)

        GLSL("""
float mstd0 = sum / %d.0;
float mstd1 = sumsq / %d.0 - mstd0 * mstd0;
float mstd2 = mix(0.0, inversesqrt(mstd1), mstd1 >= %s);
mstd1 *= mstd2;""" % (width * height, width * height, "1.192092896e-7"))

        GLSL("""
float vsum = 0.0, wsum = 0.0, sum1, sum2;
#define T(x) intBitsToFloat(x)
#define W(i,w0,w1,w2,w3) dot(samples[i],vec4(T(w0),T(w1),T(w2),T(w3)))
#define WS(w0,w1) \
sum1 = exp(sum1 * mstd2 + T(w0)); \
sum2 = sum2 * mstd2 + T(w1); \
wsum += sum1; \
vsum += sum1*(sum2/(1.0+abs(sum2)));""")

        for n in range(self.neurons):
            line = []
            for s in range(2):
                line.append("sum%d" % (s + 1))
                for i in range(sample_count):
                    global_pos, window_pos = sampling_info[i]
                    weights = []
                    for x, y in window_pos:
                        weights.append(self.weightW(n, s, x, y))
                    line.append("%sW(%d,%d,%d,%d,%d)" % (
                        "=" if i == 0 else "+",
                        i, weights[0], weights[1], weights[2], weights[3]))
                line.append(";")
            line.append("WS(%d,%d);" %
                        (self.weightWS(n, s, 0), self.weightWS(n, s, 1)))
            GLSL("".join(line))

        GLSL("""
return clamp(mstd0 + 5.0 * vsum / wsum * mstd1, 0.0, 1.0);
}  // nnedi3""")

        if use_compute:
            minx = min(key[0][i][0] for key in sampling_info for i in range(4))
            maxx = max(key[0][i][0] for key in sampling_info for i in range(4))
            miny = min(key[0][i][1] for key in sampling_info for i in range(4))
            maxy = max(key[0][i][1] for key in sampling_info for i in range(4))
            array_size = (maxx - minx + block_width, maxy - miny + block_height)
            array_offset = (-minx, -miny)

            GLSL("shared float inp[%d];" % (array_size[0] * array_size[1]))

        if use_compute:
            self.function_header_compute()
        else:
            GLSL("vec4 hook() {")

        if use_compute:
            GLSL("ivec2 group_base = ivec2(gl_WorkGroupID) * ivec2(gl_WorkGroupSize);")
            GLSL("int local_pos = int(gl_LocalInvocationID.x) * %d + int(gl_LocalInvocationID.y);" % array_size[1])

            self.samples_loop(array_size, array_offset)

            GLSL("barrier();")

        GLSL("vec4 ret = vec4(0.0, 0.0, 0.0, 0.0);")
        if use_compute:
            GLSL("vec4 ret0 = vec4(0.0, 0.0, 0.0, 0.0);")
        GLSL("vec4 samples[%d];" % sample_count)
        for i in range(sample_count):
            global_pos, window_pos = sampling_info[i]
            if use_compute:
                for j, pos in enumerate(global_pos):
                    to_fetch = "inp[local_pos + %d]"
                    to_fetch = to_fetch % ((pos[0] + array_offset[0]) * array_size[1] + (pos[1] + array_offset[1]))
                    GLSL("samples[%d][%d] = %s;" % (i, j, to_fetch))
            elif use_gather:
                base = min(global_pos)
                to_fetch = "HOOKED_mul * textureGatherOffset(HOOKED_raw, HOOKED_pos, ivec2(%d, %d), 0)"
                to_fetch = to_fetch % (base[0], base[1])
                GLSL("samples[%d] = %s;" % (i, to_fetch))
            else:
                for j, pos in enumerate(global_pos):
                    to_fetch = "HOOKED_texOff(vec2(%d.0, %d.0)).x"
                    to_fetch = to_fetch % (pos[0], pos[1])
                    GLSL("samples[%d][%d] = %s;" % (i, j, to_fetch))
        GLSL("ret[0] = nnedi3(samples);")
        if use_compute:
            GLSL("ret0[0] = inp[local_pos + %d];" %
                (array_offset[0] * array_size[1] + array_offset[1]))

        self.check_viewport()

        if use_compute:
            GLSL("imageStore(out_image, ivec2(gl_GlobalInvocationID) * $double_mul, ret0);")
            GLSL("imageStore(out_image, ivec2(gl_GlobalInvocationID) * $double_mul + $double_offset, ret);")
        else:
            GLSL("return ret;")

        GLSL("""
}""")

        return super().generate()


if __name__ == "__main__":
    import argparse
    import sys

    neurons = {16: Neurons.nns16,
               32: Neurons.nns32,
               64: Neurons.nns64,
               128: Neurons.nns128,
               256: Neurons.nns256}

    windows = {"8x4": Window.win8x4, "8x6": Window.win8x6}

    parser = argparse.ArgumentParser(
        description="generate NNEDI3 user shader for mpv")
    parser.add_argument('-n',
                        '--nns',
                        nargs=1,
                        type=int,
                        choices=sorted(neurons.keys()),
                        default=[32],
                        help='neurons for NNEDI3 (default: 32)')
    parser.add_argument('-w',
                        '--win',
                        nargs=1,
                        choices=sorted(windows.keys()),
                        default=["8x4"],
                        help='sampling window size of NNEDI3 (default: 8x4)')
    parser.add_argument('-r',
                        '--max-downscaling-ratio',
                        nargs=1,
                        type=float,
                        default=[None],
                        help='allowed downscaling ratio (default: no limit)')
    parser.add_argument('--use-gather',
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

    args = parser.parse_args()
    neuron = neurons[args.nns[0]]
    window = windows[args.win[0]]
    max_downscaling_ratio = args.max_downscaling_ratio[0]
    use_gather = args.use_gather
    use_compute = args.use_compute_shader
    compute_shader_block_size = args.compute_shader_block_size

    gen = NNEDI3(neuron,
                 window,
                 hook=["LUMA"],
                 target_tex="OUTPUT",
                 max_downscaling_ratio=max_downscaling_ratio)

    sys.stdout.write(userhook.LICENSE_HEADER)
    for step in list(Step):
        sys.stdout.write(gen.generate(
            step,
            use_gather=use_gather,
            use_compute=use_compute,
            compute_shader_block_size=compute_shader_block_size))
