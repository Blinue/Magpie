# 将纹理数据导出为 hlsl 数组
# 适合较小的纹理

from resolve import resolve

IN_FILE = 'TEXTURE.txt'
OUT_FILE = 'out.txt'

weights = resolve(IN_FILE).flat

with open(OUT_FILE, mode='w') as f:
    f.write('{\n')
    for i in range(0, len(weights) - 4, 4):
        f.write('\t{: .6f}, {: .6f}, {: .6f}, {: .6f},\n'
                .format(weights[i], weights[i + 1], weights[i + 2], weights[i + 3]))

    i = len(weights) - 4
    f.write('\t{: .6f}, {: .6f}, {: .6f}, {: .6f}\n'
            .format(weights[i], weights[i + 1], weights[i + 2], weights[i + 3]))
    f.write('}\n')
