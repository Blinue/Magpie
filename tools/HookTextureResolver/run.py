# TEXTURE.txt 格式：第一行为纹理的长和高，用空格隔开，第二行为16进制纹理数据

import struct

IN_FILE = 'TEXTURE.txt'
OUT_FILE = 'out.txt'

with open(IN_FILE) as f:
    header = f.readline()
    raw_data = f.read()

(width, _, height) = header[:-1].partition(' ')
width = int(width)
height = int(height)

weights = struct.unpack("<%df" % (width * height * 4), bytes.fromhex(raw_data))

with open(OUT_FILE, mode='w') as f:
    f.write('{\n')
    for i in range(width * height - 1):
        cur = i * 4
        f.write('\t{: .6f}, {: .6f}, {: .6f}, {: .6f},\n'
                .format(weights[cur], weights[cur + 1], weights[cur + 2], weights[cur + 3]))

    cur = (width * height - 1) * 4
    f.write('\t{: .6f}, {: .6f}, {: .6f}, {: .6f}\n'
            .format(weights[cur], weights[cur + 1], weights[cur + 2], weights[cur + 3]))
    f.write('}\n')
