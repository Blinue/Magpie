# TEXTURE.txt 格式：第一行为纹理的长和高，用空格隔开，第二行为16进制纹理数据

import struct
import numpy as np


def resolve(in_file: str) -> np.ndarray:
    with open(in_file) as f:
        header = f.readline()
        raw_data = f.read()

    (width, _, height) = header[:-1].partition(' ')
    width = int(width)
    height = int(height)

    weights = struct.unpack("<%df" % (width * height * 4), bytes.fromhex(raw_data))
    weights = np.array(weights, dtype=np.float)

    return weights.reshape((height, width, 4))
