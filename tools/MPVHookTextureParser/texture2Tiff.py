# -*- coding: utf-8 -*-

import struct
import imageio
import numpy as np
import sys


def resolve(in_file: str):
    with open(in_file) as f:
        header = f.readline()
        raw_data = f.read()

    (width, _, height) = header[:-1].partition(' ')
    width = int(width)
    height = int(height)

    weights = struct.unpack("<%df" % (width * height * 4), bytes.fromhex(raw_data))
    return np.array(weights, dtype=np.float32).reshape((height, width, 4))


IN_FILE = sys.argv[1]
OUT_FILE = sys.argv[2]

imageio.imwrite(OUT_FILE, resolve(IN_FILE))
