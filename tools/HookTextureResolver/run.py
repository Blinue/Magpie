import struct

WIDTH = 5
HEIGHT = 288

IN_FILE = 'TEXTURE.txt'
OUT_FILE = 'out.txt'

with open(IN_FILE) as f:
    read_data = f.read()

data = bytes.fromhex(read_data)
weights = struct.unpack("<%df" % (WIDTH * HEIGHT * 4), data)
print(weights)
with open(OUT_FILE, mode='w') as f:
    f.write('{\n')
    for i in range(HEIGHT * WIDTH - 1):
        cur = i * 4
        f.write('\t{: .6f}, {: .6f}, {: .6f}, {: .6f},\n'
                .format(weights[cur], weights[cur + 1], weights[cur + 2], weights[cur + 3]))

    cur = (HEIGHT * WIDTH - 1) * 4
    f.write('\t{: .6f}, {: .6f}, {: .6f}, {: .6f}\n'
            .format(weights[cur], weights[cur + 1], weights[cur + 2], weights[cur + 3]))
    f.write('}\n')
