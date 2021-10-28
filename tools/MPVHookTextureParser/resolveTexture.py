# -*- coding: utf-8 -*-

# 将二进制的 TEXTURE 块转换为文本文件

import struct
import imageio
import numpy as np
import sys

IN_FILE = sys.argv[1]
OUT_FILE = sys.argv[2]

with open(IN_FILE) as f:
    header = f.readline()
    raw_data = f.read()

(width, _, height) = header[:-1].partition(' ')
width = int(width)
height = int(height)

weights = struct.unpack("<%df" % (width * height * 4), bytes.fromhex(raw_data))

with open(OUT_FILE, 'w') as f:
    f.write(f"{width} {height}\n")
    for w in weights:
        f.write(f"{w}\n")
