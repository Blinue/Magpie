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
    print(minElem, maxElem, sep=', ')

    weights[:, :, i] = (weights[:, :, i] - minElem) / (maxElem - minElem)

weights = np.round(weights * 65535).astype(np.int)

out = np.zeros((weights.shape[0], weights.shape[1] * 4, 3), dtype=np.uint8)

for i in range(4):
    out[:, weights.shape[1] * i: weights.shape[1] * (i + 1), 0] = weights[:, :, i] / 256
    out[:, weights.shape[1] * i: weights.shape[1] * (i + 1), 1] = weights[:, :, i] % 256

imageio.imwrite(OUT_FILE, out)
