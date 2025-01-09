// CuNNy 4x4C BILINEAR RGB NVL DN - https://github.com/funnyplanter/CuNNy

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
// 
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

//!MAGPIE EFFECT
//!VERSION 4
//!SORT_NAME CuNNy-DN-D04N04

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH * 2
//!HEIGHT INPUT_HEIGHT * 2
Texture2D OUTPUT;

//!SAMPLER
//!FILTER POINT
SamplerState SP;

//!SAMPLER
//!FILTER LINEAR
SamplerState SL;

//!COMMON
#define O(t, p) t.SampleLevel(SP, pos + p * pt, 0)
#define V4 min16float4
#define M4 min16float4x4

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R8G8B8A8_SNORM
Texture2D t0;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R8G8B8A8_SNORM
Texture2D t1;

//!PASS 1
//!DESC in
//!BLOCK_SIZE 8
//!NUM_THREADS 64
//!IN INPUT
//!OUT t0

#define l0(x, y) min16float((dot(float3(2.428e-01, 4.714e-01, 1.229e-01), O(INPUT, float2(x, y)).rgb) + -7.696e-02))

V4 f0(min16float s0_0, min16float s0_1, min16float s0_2, min16float s0_3, min16float s0_4, min16float s0_5, min16float s0_6, min16float s0_7, min16float s0_8) {
	V4 r = 0.0;
	r += V4(9.154e-02, 3.758e-01, 2.353e-02, -5.798e-02) * s0_0;
	r += V4(-5.382e-01, 1.688e-01, -1.190e-01, 4.082e-02) * s0_1;
	r += V4(2.460e-02, -5.810e-02, 7.788e-02, 3.018e-02) * s0_2;
	r += V4(1.211e-01, -1.552e-01, -9.990e-02, 3.963e-02) * s0_3;
	r += V4(-2.611e-01, -4.835e-01, -6.965e-01, -4.893e-01) * s0_4;
	r += V4(-3.017e-01, -4.435e-02, 1.836e-01, 4.600e-01) * s0_5;
	r += V4(1.275e-01, 2.485e-01, 7.354e-02, -4.648e-02) * s0_6;
	r += V4(2.527e-01, 1.279e-01, 3.053e-01, 3.957e-02) * s0_7;
	r += V4(1.003e-02, 1.193e-01, 2.476e-01, -2.051e-02) * s0_8;
	r += V4(1.690e-02, 8.856e-03, -9.136e-04, 2.267e-02);
	return r;
}

void Pass1(uint2 blockStart, uint3 tid) {
	float2 pt = float2(GetInputPt());
	uint2 gxy = Rmp8x8(tid.x) + blockStart;
	uint2 size = GetInputSize();
	if (gxy.x >= size.x || gxy.y >= size.y) {
		return;
	}
	float2 pos = (gxy + 0.5) * pt;

	min16float s0_0 = l0(-1.0, -1.0);
	min16float s0_1 = l0(0.0, -1.0);
	min16float s0_2 = l0(1.0, -1.0);
	min16float s0_3 = l0(-1.0, 0.0);
	min16float s0_4 = l0(0.0, 0.0);
	min16float s0_5 = l0(1.0, 0.0);
	min16float s0_6 = l0(-1.0, 1.0);
	min16float s0_7 = l0(0.0, 1.0);
	min16float s0_8 = l0(1.0, 1.0);

	t0[gxy] = f0(s0_0, s0_1, s0_2, s0_3, s0_4, s0_5, s0_6, s0_7, s0_8);
}

//!PASS 2
//!DESC conv1
//!BLOCK_SIZE 8
//!NUM_THREADS 64
//!IN t0
//!OUT t1

#define l0(x, y) V4(O(t0, float2(x, y)))

V4 f0(V4 s0_0, V4 s0_1, V4 s0_2, V4 s0_3, V4 s0_4, V4 s0_5, V4 s0_6, V4 s0_7, V4 s0_8, V4 s1_0, V4 s1_1, V4 s1_2, V4 s1_3, V4 s1_4, V4 s1_5, V4 s1_6, V4 s1_7, V4 s1_8) {
	V4 r = 0.0;
	r += mul(s0_0, M4(-4.540e-03, -2.499e-01, 4.202e-02, 1.132e-02, 2.910e-02, -3.788e-02, 3.330e-02, -2.254e-02, -1.953e-01, 1.226e-01, -1.907e-01, -1.378e-01, 9.555e-02, -2.443e-01, 6.124e-02, -7.256e-03));
	r += mul(s0_1, M4(-1.225e-01, -1.812e-01, -1.238e-02, 4.088e-01, -9.977e-02, 4.395e-02, -2.394e-02, -5.584e-03, 2.939e-01, 4.102e-01, 6.228e-02, 3.822e-01, 8.618e-02, -1.109e-01, 1.776e-01, -7.505e-02));
	r += mul(s0_2, M4(2.047e-01, -6.853e-02, 1.880e-02, -9.030e-03, 1.505e-01, 7.782e-02, 1.347e-02, 5.566e-01, -6.951e-02, -1.352e-01, 1.941e-03, 3.975e-02, 1.637e-01, 6.708e-02, 1.501e-02, 1.373e-01));
	r += mul(s0_3, M4(-1.974e-01, 1.068e-01, -1.102e-01, 5.909e-02, 2.355e-03, 1.275e-01, -5.986e-02, -5.288e-02, 8.785e-04, -1.440e-01, -3.369e-01, -9.128e-02, 2.030e-01, 4.937e-01, -1.637e-01, 4.814e-02));
	r += mul(s0_4, M4(-3.954e-01, 4.772e-01, -5.841e-01, -8.070e-02, -2.056e-01, -2.335e-01, -2.091e-01, 1.223e-01, -2.686e-01, 1.240e+00, 7.095e-02, 6.502e-01, 1.044e-01, -3.071e-01, -2.892e-01, 4.861e-01));
	r += mul(s0_5, M4(5.943e-02, 2.245e-01, 4.014e-01, -1.063e-01, -1.869e-01, 1.384e-01, 2.996e-01, -1.928e-01, 1.212e-01, 2.849e-01, 2.093e-01, -3.821e-01, -8.705e-02, 1.976e-01, 5.176e-01, -7.461e-02));
	r += mul(s0_6, M4(1.048e-01, 2.374e-02, 2.730e-01, 1.446e-01, -5.406e-02, -1.587e-02, -2.014e-01, -3.422e-02, -2.114e-01, -5.198e-01, 2.674e-02, -6.078e-02, -2.293e-01, -9.914e-02, -2.110e-01, 7.008e-02));
	r += mul(s0_7, M4(5.799e-02, 4.932e-01, 4.559e-01, -3.118e-02, 4.706e-02, -2.242e-01, -3.165e-01, -9.912e-02, 4.041e-01, 7.241e-01, -1.696e-01, 1.990e-01, 4.697e-01, 9.965e-03, -1.141e-02, -1.365e-02));
	r += mul(s0_8, M4(-1.744e-01, -7.119e-02, 3.632e-01, -2.802e-01, -3.155e-01, 4.455e-01, -1.866e-02, -2.667e-02, 1.255e-01, -5.762e-01, -2.226e-02, 2.812e-02, -2.349e-01, 1.552e-01, -6.424e-03, 7.450e-02));
	r += mul(s1_0, M4(6.159e-02, -4.426e-02, 2.277e-02, 1.040e-01, -6.306e-04, -1.704e-01, 3.807e-02, -8.670e-02, -1.403e-01, 1.644e-01, -9.679e-02, -1.055e-01, 2.394e-01, -5.504e-02, 8.006e-02, 6.312e-02));
	r += mul(s1_1, M4(-1.134e-01, -1.030e-01, -2.777e-02, 2.955e-01, -1.225e-01, -4.096e-02, -2.748e-02, 9.404e-02, 2.890e-01, -2.441e-01, 1.560e-01, 1.694e-01, 1.853e-01, 3.311e-01, 3.408e-01, -8.678e-02));
	r += mul(s1_2, M4(1.821e-01, 3.898e-02, -2.560e-02, 1.160e-01, 2.382e-01, -1.638e-01, -1.345e-01, 3.193e-01, -1.839e-01, -2.638e-01, 5.265e-02, 2.415e-01, 2.803e-01, 1.919e-01, -7.340e-02, 1.762e-02));
	r += mul(s1_3, M4(-2.606e-01, -1.263e-01, -3.067e-02, -1.695e-02, 4.665e-03, 2.947e-02, -1.965e-02, -2.658e-02, -7.935e-02, -1.566e-01, -3.246e-01, -1.075e-03, 1.896e-01, -2.937e-01, -1.020e-01, -1.513e-01));
	r += mul(s1_4, M4(-3.696e-01, 8.901e-02, -1.890e-01, -2.804e-02, -2.998e-01, -6.597e-02, -2.613e-01, 3.877e-01, -1.032e+00, -2.328e-01, 7.941e-02, 5.733e-01, 8.618e-02, 4.213e-02, -1.242e+00, 5.861e-01));
	r += mul(s1_5, M4(1.919e-02, -5.609e-02, 3.295e-01, -2.364e-01, -4.238e-01, -6.041e-01, 3.389e-01, -4.460e-01, 4.482e-02, 1.077e-03, 8.990e-02, -2.725e-01, -4.829e-02, 1.184e-01, 1.941e-01, -3.646e-01));
	r += mul(s1_6, M4(2.968e-01, 2.018e-01, 2.695e-01, 8.891e-02, -5.857e-02, 6.005e-02, -2.440e-01, -1.349e-02, -7.572e-02, -3.213e-01, 6.274e-02, -1.229e-02, -7.589e-01, -2.313e-01, -1.627e-01, 2.538e-01));
	r += mul(s1_7, M4(-5.728e-02, 1.333e-01, 2.492e-01, -3.609e-02, 1.936e-01, -1.276e-01, -3.034e-01, -1.091e-01, 1.390e-01, 3.356e-01, -1.183e-01, 2.047e-01, 3.779e-01, -3.353e-01, 2.019e-01, 4.337e-02));
	r += mul(s1_8, M4(-1.386e-01, 1.179e-01, 2.340e-01, -1.604e-01, -4.890e-01, -5.407e-01, -1.546e-01, -1.826e-01, 1.596e-01, -1.784e-01, 5.777e-02, 3.961e-02, -2.290e-01, 2.752e-01, -4.260e-02, 9.649e-02));
	r += V4(-4.697e-03, -2.213e-02, 3.898e-01, -1.481e-02);
	return r;
}

void Pass2(uint2 blockStart, uint3 tid) {
	float2 pt = float2(GetInputPt());
	uint2 gxy = Rmp8x8(tid.x) + blockStart;
	uint2 size = GetInputSize();
	if (gxy.x >= size.x || gxy.y >= size.y) {
		return;
	}
	float2 pos = (gxy + 0.5) * pt;

	V4 s0_0 = l0(-1.0, -1.0);
	V4 s0_1 = l0(0.0, -1.0);
	V4 s0_2 = l0(1.0, -1.0);
	V4 s0_3 = l0(-1.0, 0.0);
	V4 s0_4 = l0(0.0, 0.0);
	V4 s0_5 = l0(1.0, 0.0);
	V4 s0_6 = l0(-1.0, 1.0);
	V4 s0_7 = l0(0.0, 1.0);
	V4 s0_8 = l0(1.0, 1.0);
	V4 s1_0 = -max(-s0_0, 0.0);
	V4 s1_1 = -max(-s0_1, 0.0);
	V4 s1_2 = -max(-s0_2, 0.0);
	V4 s1_3 = -max(-s0_3, 0.0);
	V4 s1_4 = -max(-s0_4, 0.0);
	V4 s1_5 = -max(-s0_5, 0.0);
	V4 s1_6 = -max(-s0_6, 0.0);
	V4 s1_7 = -max(-s0_7, 0.0);
	V4 s1_8 = -max(-s0_8, 0.0);
	s0_0 = max(s0_0, 0.0);
	s0_1 = max(s0_1, 0.0);
	s0_2 = max(s0_2, 0.0);
	s0_3 = max(s0_3, 0.0);
	s0_4 = max(s0_4, 0.0);
	s0_5 = max(s0_5, 0.0);
	s0_6 = max(s0_6, 0.0);
	s0_7 = max(s0_7, 0.0);
	s0_8 = max(s0_8, 0.0);

	t1[gxy] = f0(s0_0, s0_1, s0_2, s0_3, s0_4, s0_5, s0_6, s0_7, s0_8, s1_0, s1_1, s1_2, s1_3, s1_4, s1_5, s1_6, s1_7, s1_8);
}

//!PASS 3
//!DESC conv2
//!BLOCK_SIZE 8
//!NUM_THREADS 64
//!IN t1
//!OUT t0

#define l0(x, y) V4(O(t1, float2(x, y)))

V4 f0(V4 s0_0, V4 s0_1, V4 s0_2, V4 s0_3, V4 s0_4, V4 s0_5, V4 s0_6, V4 s0_7, V4 s0_8, V4 s1_0, V4 s1_1, V4 s1_2, V4 s1_3, V4 s1_4, V4 s1_5, V4 s1_6, V4 s1_7, V4 s1_8) {
	V4 r = 0.0;
	r += mul(s0_0, M4(-1.362e-01, -5.847e-02, 2.766e-02, 2.969e-02, 9.796e-02, 6.555e-02, -3.067e-02, -5.139e-02, 1.512e-01, 1.401e-01, -3.820e-03, 2.649e-02, -1.802e-01, -2.099e-02, -6.604e-02, 4.042e-02));
	r += mul(s0_1, M4(-2.144e-01, -1.437e-01, 4.670e-02, -2.348e-01, 9.990e-02, -5.186e-02, 1.658e-01, 9.557e-02, -1.353e-01, -1.146e-01, -9.837e-02, -8.956e-02, 1.229e-01, 2.354e-01, -2.342e-01, -1.343e-01));
	r += mul(s0_2, M4(5.918e-01, 2.130e-02, 5.753e-01, -6.941e-02, -3.156e-02, -4.438e-02, -6.348e-02, 2.682e-02, -1.078e-02, 9.727e-03, 8.472e-02, 1.460e-01, -1.921e-01, 1.872e-01, 6.067e-02, 3.762e-02));
	r += mul(s0_3, M4(1.341e-01, 1.082e-01, -4.460e-02, -1.008e-02, -1.262e-01, -7.942e-02, 5.610e-02, 4.418e-02, -1.725e-01, -1.158e-01, 6.377e-03, -1.171e-01, -3.447e-02, 4.459e-02, 2.822e-04, -7.623e-02));
	r += mul(s0_4, M4(1.994e-01, -2.251e-01, -2.432e-01, 2.467e-02, 3.717e-02, 3.275e-01, 2.005e-01, 1.427e-01, 1.122e-01, 2.864e-01, 1.478e-01, 3.701e-01, 3.111e-01, -1.704e-01, -1.410e-01, -7.490e-01));
	r += mul(s0_5, M4(-1.392e-01, -2.284e-02, 2.819e-01, -5.560e-02, -2.624e-01, 7.282e-02, -2.417e-01, -5.534e-02, -6.351e-03, -1.714e-01, -1.505e-01, -3.035e-01, -3.580e-02, 4.429e-02, 1.628e-01, -1.101e-01));
	r += mul(s0_6, M4(8.306e-04, 3.258e-02, -2.746e-02, -3.143e-02, -1.301e-02, -5.828e-02, 2.411e-03, 1.395e-02, 3.728e-02, -8.319e-02, 3.326e-02, 1.294e-01, -6.226e-02, 5.103e-02, -1.218e-02, 2.411e-01));
	r += mul(s0_7, M4(-6.323e-02, -1.343e-02, 3.400e-02, -1.727e-02, 3.683e-02, 6.325e-02, 4.834e-04, 3.849e-02, 9.424e-03, -2.010e-02, -3.447e-02, -1.330e-01, -4.107e-01, -7.682e-02, 4.138e-01, 5.994e-02));
	r += mul(s0_8, M4(7.556e-02, 1.846e-02, 1.847e-02, 1.057e-01, -1.140e-01, -2.834e-02, -3.141e-02, -1.045e-01, -2.025e-02, 4.729e-02, -2.822e-02, -4.072e-02, 3.368e-01, 6.871e-02, 1.184e-01, 1.536e-01));
	r += mul(s1_0, M4(-6.688e-02, 2.483e-02, 1.598e-01, -4.834e-02, 2.141e-01, -4.911e-02, -4.452e-02, -4.879e-02, -9.473e-01, 6.527e-01, -6.118e-01, -2.436e-01, -3.017e-02, -3.402e-01, 1.343e-01, 9.397e-02));
	r += mul(s1_1, M4(-1.330e-01, 2.557e-01, 6.838e-02, -3.936e-01, 4.806e-01, 1.828e-01, 5.073e-01, 4.502e-01, -1.404e+00, -2.954e-01, -6.745e-02, 5.594e-02, 2.640e-01, 2.330e-02, 1.331e-02, -2.700e-02));
	r += mul(s1_2, M4(2.695e-01, -1.004e-01, 9.104e-02, -4.919e-01, 3.357e-01, 4.895e-02, 4.062e-01, -3.494e-02, -4.352e-01, -1.232e-01, 8.889e-03, 3.472e-01, -1.174e-01, 7.690e-02, 6.341e-02, 9.255e-02));
	r += mul(s1_3, M4(1.805e-01, 2.494e-01, 3.474e-02, 3.930e-02, 2.671e-02, -1.438e-02, 7.294e-02, 4.854e-02, -2.864e+00, -5.832e-01, 4.350e-01, -4.265e-01, -2.643e-02, -6.234e-01, 1.283e-01, 5.168e-02));
	r += mul(s1_4, M4(-2.192e-01, 2.982e-01, -2.860e-01, -4.050e-01, 8.612e-02, 5.008e-02, 5.366e-01, 5.256e-01, -6.222e-01, 1.169e+00, 1.897e+00, 3.009e+00, 9.105e-02, -2.369e-01, -4.718e-01, -2.725e-01));
	r += mul(s1_5, M4(-7.441e-01, -1.820e-01, -5.828e-02, -6.348e-01, 5.721e-01, 1.143e-01, 2.871e-01, 3.254e-01, -1.446e-01, 1.446e-01, -8.526e-02, 7.228e-01, -9.749e-02, -1.665e-01, -1.116e-01, -2.705e-01));
	r += mul(s1_6, M4(-6.357e-02, -2.576e-02, 1.277e-02, -3.956e-02, 2.724e-02, -2.141e-02, 9.778e-02, 7.199e-03, -1.153e+00, -6.945e-01, -4.788e-01, -1.246e+00, 1.909e-01, 1.315e-01, 4.454e-02, 2.678e-01));
	r += mul(s1_7, M4(-1.022e-01, 1.572e-01, 9.404e-02, 6.768e-02, 2.191e-01, -3.163e-02, 1.257e-01, 1.058e-01, -6.394e-01, 7.223e-03, -6.930e-01, -2.963e-01, -2.666e-01, 3.461e-03, 2.203e-01, -1.212e-01));
	r += mul(s1_8, M4(-1.179e-01, 7.311e-02, 1.371e-01, -4.039e-02, 2.171e-01, 3.131e-02, 2.219e-01, 1.564e-02, -4.895e-01, -5.067e-03, -4.528e-01, 5.694e-02, 6.858e-02, 6.808e-03, -1.017e-01, 6.675e-03));
	r += V4(-8.341e-03, 1.434e-02, 5.791e-03, -1.033e-02);
	return r;
}

void Pass3(uint2 blockStart, uint3 tid) {
	float2 pt = float2(GetInputPt());
	uint2 gxy = Rmp8x8(tid.x) + blockStart;
	uint2 size = GetInputSize();
	if (gxy.x >= size.x || gxy.y >= size.y) {
		return;
	}
	float2 pos = (gxy + 0.5) * pt;

	V4 s0_0 = l0(-1.0, -1.0);
	V4 s0_1 = l0(0.0, -1.0);
	V4 s0_2 = l0(1.0, -1.0);
	V4 s0_3 = l0(-1.0, 0.0);
	V4 s0_4 = l0(0.0, 0.0);
	V4 s0_5 = l0(1.0, 0.0);
	V4 s0_6 = l0(-1.0, 1.0);
	V4 s0_7 = l0(0.0, 1.0);
	V4 s0_8 = l0(1.0, 1.0);
	V4 s1_0 = -max(-s0_0, 0.0);
	V4 s1_1 = -max(-s0_1, 0.0);
	V4 s1_2 = -max(-s0_2, 0.0);
	V4 s1_3 = -max(-s0_3, 0.0);
	V4 s1_4 = -max(-s0_4, 0.0);
	V4 s1_5 = -max(-s0_5, 0.0);
	V4 s1_6 = -max(-s0_6, 0.0);
	V4 s1_7 = -max(-s0_7, 0.0);
	V4 s1_8 = -max(-s0_8, 0.0);
	s0_0 = max(s0_0, 0.0);
	s0_1 = max(s0_1, 0.0);
	s0_2 = max(s0_2, 0.0);
	s0_3 = max(s0_3, 0.0);
	s0_4 = max(s0_4, 0.0);
	s0_5 = max(s0_5, 0.0);
	s0_6 = max(s0_6, 0.0);
	s0_7 = max(s0_7, 0.0);
	s0_8 = max(s0_8, 0.0);

	t0[gxy] = f0(s0_0, s0_1, s0_2, s0_3, s0_4, s0_5, s0_6, s0_7, s0_8, s1_0, s1_1, s1_2, s1_3, s1_4, s1_5, s1_6, s1_7, s1_8);
}

//!PASS 4
//!DESC conv3
//!BLOCK_SIZE 8
//!NUM_THREADS 64
//!IN t0
//!OUT t1

#define l0(x, y) V4(O(t0, float2(x, y)))

V4 f0(V4 s0_0, V4 s0_1, V4 s0_2, V4 s0_3, V4 s0_4, V4 s0_5, V4 s0_6, V4 s0_7, V4 s0_8, V4 s1_0, V4 s1_1, V4 s1_2, V4 s1_3, V4 s1_4, V4 s1_5, V4 s1_6, V4 s1_7, V4 s1_8) {
	V4 r = 0.0;
	r += mul(s0_0, M4(-6.123e-02, 9.666e-03, 4.969e-02, 3.030e-02, 1.714e-02, -3.117e-02, -9.470e-02, 2.078e-03, 4.109e-02, -5.560e-02, 3.757e-02, -3.667e-03, -3.500e-02, -8.151e-02, 1.104e-01, -1.219e-01));
	r += mul(s0_1, M4(9.596e-02, -6.361e-02, 1.162e-02, -3.138e-02, -1.277e-02, -4.005e-02, 1.805e-02, -1.459e-02, -7.903e-03, 1.138e-02, 1.542e-02, -2.357e-02, -1.421e-01, -2.953e-01, 1.322e-01, 6.480e-03));
	r += mul(s0_2, M4(1.571e-01, -1.081e-01, 1.345e-01, -5.616e-02, -1.211e-02, 4.515e-02, 1.797e-02, 6.143e-02, -9.605e-02, 7.782e-02, -1.421e-01, 3.195e-02, 1.841e-01, -7.735e-02, 1.082e-01, 1.785e-02));
	r += mul(s0_3, M4(1.739e-03, -4.187e-02, 1.093e-01, 1.042e-01, -6.538e-03, 5.025e-02, -7.052e-03, -1.033e-01, -1.394e-01, -4.638e-01, 4.354e-02, -1.188e-02, 7.809e-04, 2.484e-01, -8.330e-01, -2.787e-01));
	r += mul(s0_4, M4(-6.489e-03, -6.309e-01, 7.169e-01, 1.557e-01, 1.478e-01, 2.977e-01, -2.818e-01, 5.129e-02, 7.598e-01, 8.124e-01, -1.262e-02, -1.325e-01, -2.764e-01, 3.485e-01, 4.717e-01, -2.467e-01));
	r += mul(s0_5, M4(2.022e-02, -1.396e-01, 1.865e-01, 1.568e-02, 3.924e-01, -2.466e-01, 4.990e-01, 3.971e-02, -1.176e-01, 1.792e-01, -2.861e-01, 3.555e-02, -1.428e-01, 2.528e-01, -2.085e-01, -1.311e-01));
	r += mul(s0_6, M4(3.340e-02, -1.203e-01, 1.014e-01, 1.154e-01, -9.031e-03, -5.586e-02, -5.700e-03, 2.391e-02, -3.509e-01, 6.729e-02, 1.004e-01, -3.277e-01, 1.026e-01, 3.286e-03, -6.603e-02, -3.238e-03));
	r += mul(s0_7, M4(-6.854e-01, 1.013e-01, -6.298e-02, -5.464e-01, 2.486e-01, -2.186e-01, 3.986e-02, 3.800e-01, -1.267e-01, 1.037e-01, 1.538e-01, -2.069e-01, 9.431e-02, 5.337e-02, -8.507e-02, 2.015e-01));
	r += mul(s0_8, M4(-5.009e-03, 1.493e-01, -3.010e-02, -2.429e-02, -3.137e-01, -2.276e-01, 1.556e-01, 1.452e-02, 2.063e-01, 3.699e-02, -1.675e-03, 8.221e-02, -6.732e-02, 8.296e-02, -8.474e-02, -1.458e-01));
	r += mul(s1_0, M4(-3.003e-02, -9.777e-03, 1.239e-02, -3.907e-02, 1.841e-01, -8.959e-02, 9.257e-02, 1.333e-01, 5.703e-04, -1.367e-01, -1.026e-01, 6.398e-02, 1.262e-02, 1.101e-02, 4.291e-02, -4.238e-02));
	r += mul(s1_1, M4(5.516e-02, 9.884e-04, -5.383e-02, -1.048e-02, 2.529e-01, 9.819e-02, 1.255e-01, 3.149e-02, -8.249e-02, -1.386e-02, 6.214e-02, 2.957e-02, 1.001e-01, 1.590e-01, 1.159e-02, 5.273e-02));
	r += mul(s1_2, M4(4.571e-02, -6.277e-03, 1.496e-01, -4.044e-02, 4.089e-02, -3.801e-02, -3.690e-02, -1.037e-01, -6.031e-02, 2.117e-03, -9.644e-02, 6.392e-02, 5.093e-02, -2.512e-02, 1.131e-01, 1.304e-01));
	r += mul(s1_3, M4(-3.118e-02, 2.185e-02, 1.763e-01, 8.327e-02, 6.337e-02, 8.724e-02, 6.808e-02, -4.070e-01, -6.922e-02, -2.417e-01, -1.175e-01, -1.845e-01, -3.773e-03, -1.869e-01, -9.345e-02, -2.340e-01));
	r += mul(s1_4, M4(-1.159e-01, -4.476e-01, 2.989e-01, 2.794e-01, 5.756e-01, -4.803e-01, -5.979e-02, -1.959e-01, 5.261e-02, -2.399e-01, -6.616e-02, -9.243e-01, 4.622e-01, 1.139e-01, 2.482e-01, 2.254e-01));
	r += mul(s1_5, M4(1.064e-01, -1.989e-02, 8.581e-02, 3.218e-02, 3.344e-01, -5.684e-01, 4.009e-01, 4.482e-01, 7.737e-02, 8.716e-02, -1.382e-01, -7.145e-02, -1.225e-01, 1.471e-01, -1.866e-01, 3.674e-02));
	r += mul(s1_6, M4(5.376e-02, -6.192e-03, -1.760e-01, 7.590e-02, -3.279e-02, -1.888e-01, 2.057e-01, 2.114e-01, -3.941e-01, 5.584e-03, 9.400e-03, -4.289e-01, -2.289e-01, 1.880e-01, 3.184e-02, -4.442e-01));
	r += mul(s1_7, M4(-4.174e-01, -1.344e-01, 3.866e-02, 4.521e-02, -4.215e-01, 1.479e-01, 2.476e-01, -7.051e-01, -4.153e-01, 3.373e-01, 8.098e-02, -6.680e-01, 3.920e-01, -1.023e-01, -2.166e-02, 3.816e-01));
	r += mul(s1_8, M4(-3.441e-02, 3.404e-03, -4.958e-02, 9.652e-03, -1.930e-02, -2.470e-01, 1.610e-01, 1.112e-01, 2.574e-02, 2.310e-01, 3.643e-02, -5.044e-02, 7.788e-02, 1.923e-03, -7.115e-02, -6.575e-03));
	r += V4(1.370e-02, 1.151e-02, 2.567e-03, -1.881e-03);
	return r;
}

void Pass4(uint2 blockStart, uint3 tid) {
	float2 pt = float2(GetInputPt());
	uint2 gxy = Rmp8x8(tid.x) + blockStart;
	uint2 size = GetInputSize();
	if (gxy.x >= size.x || gxy.y >= size.y) {
		return;
	}
	float2 pos = (gxy + 0.5) * pt;

	V4 s0_0 = l0(-1.0, -1.0);
	V4 s0_1 = l0(0.0, -1.0);
	V4 s0_2 = l0(1.0, -1.0);
	V4 s0_3 = l0(-1.0, 0.0);
	V4 s0_4 = l0(0.0, 0.0);
	V4 s0_5 = l0(1.0, 0.0);
	V4 s0_6 = l0(-1.0, 1.0);
	V4 s0_7 = l0(0.0, 1.0);
	V4 s0_8 = l0(1.0, 1.0);
	V4 s1_0 = -max(-s0_0, 0.0);
	V4 s1_1 = -max(-s0_1, 0.0);
	V4 s1_2 = -max(-s0_2, 0.0);
	V4 s1_3 = -max(-s0_3, 0.0);
	V4 s1_4 = -max(-s0_4, 0.0);
	V4 s1_5 = -max(-s0_5, 0.0);
	V4 s1_6 = -max(-s0_6, 0.0);
	V4 s1_7 = -max(-s0_7, 0.0);
	V4 s1_8 = -max(-s0_8, 0.0);
	s0_0 = max(s0_0, 0.0);
	s0_1 = max(s0_1, 0.0);
	s0_2 = max(s0_2, 0.0);
	s0_3 = max(s0_3, 0.0);
	s0_4 = max(s0_4, 0.0);
	s0_5 = max(s0_5, 0.0);
	s0_6 = max(s0_6, 0.0);
	s0_7 = max(s0_7, 0.0);
	s0_8 = max(s0_8, 0.0);

	t1[gxy] = f0(s0_0, s0_1, s0_2, s0_3, s0_4, s0_5, s0_6, s0_7, s0_8, s1_0, s1_1, s1_2, s1_3, s1_4, s1_5, s1_6, s1_7, s1_8);
}

//!PASS 5
//!DESC conv4
//!BLOCK_SIZE 8
//!NUM_THREADS 64
//!IN t1
//!OUT t0

#define l0(x, y) V4(O(t1, float2(x, y)))

V4 f0(V4 s0_0, V4 s0_1, V4 s0_2, V4 s0_3, V4 s0_4, V4 s0_5, V4 s0_6, V4 s0_7, V4 s0_8, V4 s1_0, V4 s1_1, V4 s1_2, V4 s1_3, V4 s1_4, V4 s1_5, V4 s1_6, V4 s1_7, V4 s1_8) {
	V4 r = 0.0;
	r += mul(s0_0, M4(2.376e-02, 2.931e-02, 7.304e-02, -5.238e-02, -6.500e-03, -3.887e-02, 2.506e-02, 5.201e-03, 5.599e-02, -1.951e-01, -3.847e-01, 8.685e-02, -1.106e-01, -3.954e-02, 1.571e-01, 2.293e-02));
	r += mul(s0_1, M4(-2.738e-02, 1.554e-01, 1.120e-01, 1.856e-02, 9.513e-03, -2.222e-01, -2.174e-01, -1.065e-02, 3.001e-02, 7.638e-02, -7.497e-02, -2.727e-02, -1.521e-02, 1.843e-01, 3.547e-01, -1.642e-02));
	r += mul(s0_2, M4(-2.533e-02, -1.959e-02, -6.274e-02, 8.121e-03, -8.703e-03, 5.091e-02, 6.548e-02, 1.988e-02, 4.089e-02, -4.827e-02, -4.089e-02, -4.361e-02, -1.112e-02, -1.101e-02, 2.968e-02, -2.196e-03));
	r += mul(s0_3, M4(1.813e-02, -2.087e-01, -2.474e-01, -1.066e-01, 2.549e-01, 6.466e-01, 3.169e-01, -1.109e-01, -1.551e-02, -3.119e-01, -3.959e-01, 2.141e-01, 1.121e-01, 3.268e-01, 1.038e-01, -5.818e-02));
	r += mul(s0_4, M4(-3.147e-01, 2.716e-01, 1.304e-01, 3.887e-01, 9.396e-02, -9.787e-02, -1.596e-01, -7.138e-02, -2.462e-01, -3.027e-01, 6.980e-01, -1.546e-01, 3.730e-02, -7.502e-02, -4.408e-02, 3.814e-02));
	r += mul(s0_5, M4(-4.177e-02, -1.326e-02, -7.497e-02, 1.168e-03, 5.595e-03, 3.603e-02, 2.589e-02, -2.179e-02, 1.998e-02, -3.544e-03, 1.125e-01, 2.648e-03, -2.417e-02, -1.876e-02, 4.009e-02, 5.481e-02));
	r += mul(s0_6, M4(-7.181e-02, -2.968e-02, -3.169e-02, -1.899e-02, -3.692e-02, -2.156e-02, 9.595e-02, 1.055e-01, -1.274e-01, -2.576e-02, 8.706e-02, 1.895e-01, 6.316e-04, -4.574e-02, 2.201e-02, 1.199e-01));
	r += mul(s0_7, M4(-2.193e-01, 1.563e-02, 1.287e-01, 2.403e-01, 2.222e-01, -1.748e-02, 1.486e-02, -7.685e-02, 4.971e-01, 2.920e-01, -2.253e-01, -8.145e-01, 3.018e-01, -4.559e-02, -1.509e-01, -3.003e-01));
	r += mul(s0_8, M4(1.685e-02, -1.082e-02, 3.539e-03, -2.765e-02, -5.968e-03, -4.628e-03, 3.847e-02, 6.426e-02, -6.284e-02, 5.455e-02, -3.291e-02, 1.636e-01, 5.828e-02, -5.613e-02, -4.404e-02, -1.715e-02));
	r += mul(s1_0, M4(1.875e-02, 7.150e-02, 3.015e-02, -4.917e-02, 9.333e-03, -1.519e-01, -1.153e-01, 4.344e-02, -1.603e-02, -4.775e-02, -4.484e-02, 6.567e-02, -6.714e-02, 2.569e-01, 4.638e-01, 3.038e-02));
	r += mul(s1_1, M4(-4.046e-02, 1.372e-01, 2.476e-01, 6.565e-02, 6.481e-04, -1.529e-02, 1.376e-02, 1.367e-02, 2.941e-04, 1.423e-01, 2.311e-01, 7.538e-03, -6.762e-02, -3.992e-01, -1.160e-02, 3.123e-02));
	r += mul(s1_2, M4(-3.926e-02, 1.709e-04, -4.761e-02, -8.731e-03, 5.123e-03, 7.039e-02, 1.061e-01, -1.322e-03, 4.069e-02, -1.182e-01, -3.698e-04, -7.746e-02, -3.827e-02, 9.957e-02, 9.991e-02, 5.215e-02));
	r += mul(s1_3, M4(-1.865e-01, -9.784e-01, -5.871e-01, 1.384e-01, 2.097e-01, -1.229e-01, -4.912e-01, -4.254e-02, 3.395e-04, -8.968e-02, -6.923e-02, -4.916e-02, 2.424e-01, 7.730e-01, 2.573e-01, -2.380e-01));
	r += mul(s1_4, M4(-9.293e-01, 6.176e-01, 1.970e-01, 3.467e-01, 4.341e-01, 9.866e-01, 3.035e-01, -1.062e-01, -1.501e-01, 2.709e-01, 1.991e-01, -2.164e-01, 2.881e-01, -1.696e-01, -4.141e-01, -1.004e+00));
	r += mul(s1_5, M4(-8.323e-02, -1.285e-02, -3.468e-02, 1.551e-01, 1.330e-01, -1.238e-01, -1.675e-03, 5.588e-02, 2.128e-01, -2.327e-01, -2.891e-02, 1.567e-01, -1.448e-01, 8.781e-02, 3.254e-02, 7.142e-02));
	r += mul(s1_6, M4(1.231e-01, 5.139e-02, -9.426e-02, -2.822e-01, 1.761e-03, 6.853e-03, 1.165e-01, 7.861e-02, -9.715e-03, 5.489e-03, -1.066e-02, -8.332e-03, -9.111e-02, 3.911e-02, 1.757e-01, 2.222e-01));
	r += mul(s1_7, M4(2.275e-02, 1.199e-01, 5.904e-02, -2.051e-01, 6.950e-01, 1.592e-02, -9.888e-02, -6.701e-01, -9.096e-02, 3.203e-02, 1.204e-01, 2.153e-01, 1.448e-01, -5.225e-03, 6.786e-02, 2.005e-02));
	r += mul(s1_8, M4(-3.290e-02, -3.758e-02, -3.158e-02, 8.713e-02, 3.917e-02, 4.275e-02, -2.450e-02, 3.970e-02, 1.928e-01, 5.498e-02, -5.673e-02, -3.743e-01, 4.981e-02, -1.785e-02, 1.958e-02, 3.487e-02));
	r += V4(7.249e-03, 2.949e-03, 5.297e-03, 3.693e-03);
	return r;
}

void Pass5(uint2 blockStart, uint3 tid) {
	float2 pt = float2(GetInputPt());
	uint2 gxy = Rmp8x8(tid.x) + blockStart;
	uint2 size = GetInputSize();
	if (gxy.x >= size.x || gxy.y >= size.y) {
		return;
	}
	float2 pos = (gxy + 0.5) * pt;

	V4 s0_0 = l0(-1.0, -1.0);
	V4 s0_1 = l0(0.0, -1.0);
	V4 s0_2 = l0(1.0, -1.0);
	V4 s0_3 = l0(-1.0, 0.0);
	V4 s0_4 = l0(0.0, 0.0);
	V4 s0_5 = l0(1.0, 0.0);
	V4 s0_6 = l0(-1.0, 1.0);
	V4 s0_7 = l0(0.0, 1.0);
	V4 s0_8 = l0(1.0, 1.0);
	V4 s1_0 = -max(-s0_0, 0.0);
	V4 s1_1 = -max(-s0_1, 0.0);
	V4 s1_2 = -max(-s0_2, 0.0);
	V4 s1_3 = -max(-s0_3, 0.0);
	V4 s1_4 = -max(-s0_4, 0.0);
	V4 s1_5 = -max(-s0_5, 0.0);
	V4 s1_6 = -max(-s0_6, 0.0);
	V4 s1_7 = -max(-s0_7, 0.0);
	V4 s1_8 = -max(-s0_8, 0.0);
	s0_0 = max(s0_0, 0.0);
	s0_1 = max(s0_1, 0.0);
	s0_2 = max(s0_2, 0.0);
	s0_3 = max(s0_3, 0.0);
	s0_4 = max(s0_4, 0.0);
	s0_5 = max(s0_5, 0.0);
	s0_6 = max(s0_6, 0.0);
	s0_7 = max(s0_7, 0.0);
	s0_8 = max(s0_8, 0.0);

	t0[gxy] = f0(s0_0, s0_1, s0_2, s0_3, s0_4, s0_5, s0_6, s0_7, s0_8, s1_0, s1_1, s1_2, s1_3, s1_4, s1_5, s1_6, s1_7, s1_8);
}

//!PASS 6
//!DESC out-shuffle
//!BLOCK_SIZE 16
//!NUM_THREADS 64
//!IN INPUT, t0
//!OUT OUTPUT

#define l0(x, y) V4(O(t0, float2(x, y)))

V4 f0(V4 s0_0, V4 s0_1, V4 s0_2, V4 s0_3, V4 s0_4, V4 s0_5, V4 s0_6, V4 s0_7, V4 s0_8, V4 s1_0, V4 s1_1, V4 s1_2, V4 s1_3, V4 s1_4, V4 s1_5, V4 s1_6, V4 s1_7, V4 s1_8) {
	V4 r = 0.0;
	r += mul(s0_0, M4(2.340e-02, 8.171e-02, -1.124e-01, -5.065e-02, -5.505e-02, -5.540e-02, -3.000e-03, -1.346e-02, 3.800e-02, 4.944e-02, -2.084e-02, 6.388e-03, 8.566e-02, 2.480e-02, 1.184e-01, -1.075e-04));
	r += mul(s0_1, M4(-2.188e-02, -2.056e-01, 1.480e-02, -7.451e-02, 5.240e-02, 4.098e-02, -4.668e-03, 1.810e-02, -2.533e-02, -6.403e-02, 1.984e-02, -5.716e-02, -3.356e-03, -2.173e-01, 1.218e-01, 1.179e-01));
	r += mul(s0_2, M4(7.330e-03, 2.521e-02, 1.372e-02, 3.411e-02, -1.438e-02, -1.009e-02, 7.676e-03, -1.712e-02, 5.980e-03, 2.040e-02, -8.766e-03, 3.442e-02, -1.623e-02, -2.557e-02, -6.086e-03, 5.413e-04));
	r += mul(s0_3, M4(1.754e-01, 6.364e-02, 2.842e-01, 2.378e-01, -1.684e-01, -1.911e-02, -3.838e-01, -2.622e-02, 2.065e-01, 3.951e-02, 4.217e-01, 4.374e-02, -1.028e-02, 2.417e-02, -1.595e-02, 6.305e-02));
	r += mul(s0_4, M4(-5.620e-02, -8.609e-02, -1.256e-01, -3.166e-01, -1.712e-01, -1.602e-01, -1.577e-01, -4.901e-01, -5.012e-02, 1.082e-01, -7.271e-02, 4.072e-01, -7.789e-02, -1.725e-01, -1.397e-01, -4.507e-01));
	r += mul(s0_5, M4(1.401e-02, 4.716e-02, 1.486e-02, 4.642e-02, 1.131e-02, 3.865e-02, -9.865e-03, 9.301e-02, 3.441e-03, -8.098e-03, -6.012e-03, -1.549e-01, 1.486e-02, 1.872e-02, -2.469e-03, 1.294e-02));
	r += mul(s0_6, M4(-3.894e-02, -4.136e-05, -3.022e-02, 1.045e-03, -3.730e-02, -1.838e-02, -5.573e-02, -2.760e-02, 3.516e-02, 1.602e-02, 6.358e-02, 3.111e-02, -3.045e-02, -7.728e-03, -4.189e-02, -1.102e-02));
	r += mul(s0_7, M4(-1.184e-02, 1.728e-02, 7.925e-03, 6.763e-02, 2.590e-03, -9.456e-03, -4.407e-02, -2.044e-02, 4.472e-02, 2.228e-02, 7.233e-02, 4.863e-02, -1.814e-02, -2.034e-03, -4.994e-02, -2.460e-02));
	r += mul(s0_8, M4(-3.292e-03, -9.015e-03, -3.171e-03, -2.504e-02, 2.120e-03, 3.064e-02, 2.108e-02, 4.592e-02, 2.258e-03, -2.192e-04, -3.576e-03, 3.733e-02, -1.931e-03, -5.083e-03, 5.877e-03, -1.764e-02));
	r += mul(s1_0, M4(4.321e-02, -8.135e-02, -1.567e-01, -6.888e-03, -6.542e-02, -1.656e-02, 1.236e-02, -7.563e-03, 4.657e-02, 9.222e-03, -6.696e-03, -3.545e-03, -6.401e-01, 1.189e-01, 1.509e-01, 2.417e-01));
	r += mul(s1_1, M4(-2.058e-02, 1.174e-01, -2.482e-02, -8.423e-02, -1.692e-02, -1.094e-02, 3.530e-02, 1.780e-02, -9.937e-02, -9.030e-02, 2.304e-02, 1.294e-02, 7.976e-02, -3.096e-01, 1.382e-01, 2.456e-01));
	r += mul(s1_2, M4(4.491e-02, -1.336e-02, 3.593e-02, -3.503e-02, -8.630e-03, -4.295e-03, -1.356e-02, 3.843e-02, 9.887e-03, 1.913e-03, 2.247e-03, 1.113e-02, -7.234e-04, -3.058e-02, 2.833e-03, -1.707e-02));
	r += mul(s1_3, M4(2.007e-01, 6.756e-02, 9.393e-01, 9.057e-02, -3.701e-01, -1.729e-02, -4.136e-01, 2.233e-02, 2.783e-01, 3.590e-02, 3.564e-01, 8.342e-03, 1.333e-01, 7.944e-02, -2.312e-01, 8.354e-02));
	r += mul(s1_4, M4(-3.334e-01, -2.705e-01, -4.072e-01, 3.946e-01, 5.159e-03, -5.860e-01, 1.578e-01, -3.614e-01, 5.366e-01, 4.699e-01, -3.700e-01, 9.463e-02, -4.090e-02, -9.767e-02, -7.999e-02, -4.859e-01));
	r += mul(s1_5, M4(5.700e-02, 6.092e-02, 4.114e-02, -1.564e-02, -1.345e-02, 9.692e-02, 1.456e-03, 9.371e-02, -3.845e-02, -4.751e-02, -2.509e-02, -2.842e-01, 2.938e-03, 2.387e-02, -6.191e-04, -3.120e-04));
	r += mul(s1_6, M4(3.888e-02, 4.969e-02, -1.851e-01, -9.866e-03, -3.527e-02, -1.377e-02, -7.594e-02, -2.619e-02, 3.259e-02, 9.636e-03, 8.622e-03, 1.788e-02, -3.505e-02, -1.048e-03, -1.329e-02, 1.425e-02));
	r += mul(s1_7, M4(6.891e-03, 8.118e-02, -6.443e-02, -1.487e-01, 2.183e-02, 1.106e-03, 6.656e-02, -9.506e-02, 7.418e-04, -6.015e-02, 3.594e-01, 1.039e-02, -3.600e-02, -7.771e-03, -3.406e-02, 2.935e-02));
	r += mul(s1_8, M4(-4.598e-03, -4.678e-03, 1.595e-02, -8.273e-03, 6.740e-03, 1.175e-02, -2.997e-02, -6.116e-03, -3.788e-02, -9.471e-02, -2.149e-02, 4.139e-02, -9.614e-03, -5.573e-03, -1.643e-02, -1.712e-02));
	r += V4(2.510e-03, 4.409e-03, 2.891e-03, 4.977e-03);
	return tanh(r);
}

void Pass6(uint2 blockStart, uint3 tid) {
	float2 pt = float2(GetInputPt());
	uint2 gxy = (Rmp8x8(tid.x) << 1) + blockStart;
	uint2 size = GetOutputSize();
	if (gxy.x >= size.x || gxy.y >= size.y) {
		return;
	}
	float2 pos = ((gxy >> 1) + 0.5) * pt;

	V4 s0_0 = l0(-1.0, -1.0);
	V4 s0_1 = l0(0.0, -1.0);
	V4 s0_2 = l0(1.0, -1.0);
	V4 s0_3 = l0(-1.0, 0.0);
	V4 s0_4 = l0(0.0, 0.0);
	V4 s0_5 = l0(1.0, 0.0);
	V4 s0_6 = l0(-1.0, 1.0);
	V4 s0_7 = l0(0.0, 1.0);
	V4 s0_8 = l0(1.0, 1.0);
	V4 s1_0 = -max(-s0_0, 0.0);
	V4 s1_1 = -max(-s0_1, 0.0);
	V4 s1_2 = -max(-s0_2, 0.0);
	V4 s1_3 = -max(-s0_3, 0.0);
	V4 s1_4 = -max(-s0_4, 0.0);
	V4 s1_5 = -max(-s0_5, 0.0);
	V4 s1_6 = -max(-s0_6, 0.0);
	V4 s1_7 = -max(-s0_7, 0.0);
	V4 s1_8 = -max(-s0_8, 0.0);
	s0_0 = max(s0_0, 0.0);
	s0_1 = max(s0_1, 0.0);
	s0_2 = max(s0_2, 0.0);
	s0_3 = max(s0_3, 0.0);
	s0_4 = max(s0_4, 0.0);
	s0_5 = max(s0_5, 0.0);
	s0_6 = max(s0_6, 0.0);
	s0_7 = max(s0_7, 0.0);
	s0_8 = max(s0_8, 0.0);

	V4 r = f0(s0_0, s0_1, s0_2, s0_3, s0_4, s0_5, s0_6, s0_7, s0_8, s1_0, s1_1, s1_2, s1_3, s1_4, s1_5, s1_6, s1_7, s1_8);

	static const float3x3 rgb2yuv = {0.299, 0.587, 0.114, -0.169, -0.331, 0.5, 0.5, -0.419, -0.081};
	static const float3x3 yuv2rgb = {1, -0.00093, 1.401687, 1, -0.3437, -0.71417, 1, 1.77216, 0.00099};
	float2 opt = float2(GetOutputPt());

	pos -= 0.5f * opt;
	float3 yuv = mul(rgb2yuv, INPUT.SampleLevel(SL, pos, 0).rgb);
	OUTPUT[gxy] = float4(mul(yuv2rgb, float3(saturate(yuv.r + r.x), yuv.yz)), 1);

	++gxy.x;
	pos.x += opt.x;
	yuv = mul(rgb2yuv, INPUT.SampleLevel(SL, pos, 0).rgb);
	OUTPUT[gxy] = float4(mul(yuv2rgb, float3(saturate(yuv.r + r.y), yuv.yz)), 1);

	++gxy.y;
	pos.y += opt.y;
	yuv = mul(rgb2yuv, INPUT.SampleLevel(SL, pos, 0).rgb);
	OUTPUT[gxy] = float4(mul(yuv2rgb, float3(saturate(yuv.r + r.w), yuv.yz)), 1);

	--gxy.x;
	pos.x -= opt.x;
	yuv = mul(rgb2yuv, INPUT.SampleLevel(SL, pos, 0).rgb);
	OUTPUT[gxy] = float4(mul(yuv2rgb, float3(saturate(yuv.r + r.z), yuv.yz)), 1);
}
