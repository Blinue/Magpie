from resolve import resolve
import imageio
import numpy as np

IN_FILE = 'TEXTURE.txt'
OUT_FILE = 'RavuZoomR3Weights.png'

weights: np.ndarray = resolve(IN_FILE)

# 将四个分量分别压缩到 [0, 1]
for i in range(4):
    minElem = np.amin(weights[:, :, i])
    maxElem = np.amax(weights[:, :, i])
    print(minElem, maxElem)

    weights[:, :, i] = (weights[:, :, i] - minElem) / (maxElem - minElem)

out = np.zeros((weights.shape[0], weights.shape[1] * 2, 3), dtype=np.uint8)

# x 和 y 用一个字节表示
out[:, :weights.shape[1], 0:2] = np.round(weights[:, :, 0:2] * 255).astype(np.uint8)

# z 和 w 用两个字节表示，高字节放在像素的 z 分量里
z = np.round(weights[:, :, 2] * 65535).astype(np.int)
w = np.round(weights[:, :, 3] * 65535).astype(np.int)

out[:, :weights.shape[1], 2] = z / 256
out[:, weights.shape[1]:, 0] = z % 256
out[:, weights.shape[1]:, 1] = w % 256
out[:, weights.shape[1]:, 2] = w / 256

imageio.imwrite(OUT_FILE, out)
