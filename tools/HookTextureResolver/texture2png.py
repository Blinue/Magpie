from resolve import resolve
import imageio
import numpy as np

IN_FILE = 'TEXTURE.txt'
OUT_FILE = 'out.png'

weights: np.ndarray = resolve(IN_FILE)

for i in range(4):
    minElem = np.amin(weights[:, :, i])
    maxElem = np.amax(weights[:, :, i])
    print(minElem, maxElem)

    weights[:, :, i] = (weights[:, :, i] - minElem) / (maxElem - minElem)

out = np.zeros((weights.shape[0], weights.shape[1] * 2, 3), dtype=np.float)
out[:, :weights.shape[1], 0:2] = weights[:, :, 0:2]
out[:, weights.shape[1]:, 0:2] = weights[:, :, 2:4]
out = np.round(out * 255).astype(np.uint8)
imageio.imwrite(OUT_FILE, out)
