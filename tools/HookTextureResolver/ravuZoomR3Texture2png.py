from resolve import resolve
import imageio
import numpy as np

IN_FILE = 'TEXTURE.txt'
OUT_FILE = 'weights.tiff'

weights: np.ndarray = resolve(IN_FILE)
weights = weights.astype(np.float32)
imageio.imwrite(OUT_FILE, weights)
