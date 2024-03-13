// CuNNy 4x4C BILINEAR RGB NVL - https://github.com/cunnyplapper/CuNNy

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
//!SORT_NAME CuNNy-D04N04

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
//!DESC CuNNy-4x4C-BILINEAR-RGB-NVL-in
//!BLOCK_SIZE 8
//!NUM_THREADS 64
//!IN INPUT
//!OUT t0

#define l0(x, y) (dot(float3(-4.174e-01, -7.873e-01, -1.763e-01), O(INPUT, float2(x, y)).rgb) + 1.011e+00)

float4 f0(min16float s0_0, min16float s0_1, min16float s0_2, min16float s0_3, min16float s0_4, min16float s0_5, min16float s0_6, min16float s0_7, min16float s0_8) {
	V4 r = 0.0;
	r += V4(1.222e-01, 7.038e-03, 1.179e-01, 1.876e-01) * s0_0;
	r += V4(1.025e-01, -2.993e-01, 3.154e-01, -1.050e-01) * s0_1;
	r += V4(5.656e-02, -3.117e-03, -6.665e-02, -2.044e-01) * s0_2;
	r += V4(-5.045e-01, -4.189e-01, -3.076e-01, -3.691e-01) * s0_3;
	r += V4(1.365e-01, 6.699e-01, 3.389e-01, 4.561e-01) * s0_4;
	r += V4(-7.690e-02, 2.655e-02, -1.044e-02, 7.271e-02) * s0_5;
	r += V4(1.358e-02, 3.378e-03, -1.802e-01, -1.936e-01) * s0_6;
	r += V4(8.227e-02, 1.550e-02, -1.820e-01, -1.670e-01) * s0_7;
	r += V4(9.988e-03, 1.413e-03, -2.486e-02, 3.258e-01) * s0_8;
	r += V4(3.566e-02, -1.308e-03, -5.595e-03, -5.246e-03);
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
//!DESC CuNNy-4x4C-BILINEAR-RGB-NVL-conv1
//!BLOCK_SIZE 8
//!NUM_THREADS 64
//!IN t0
//!OUT t1

#define l0(x, y) O(t0, float2(x, y))

float4 f0(V4 s0_0, V4 s0_1, V4 s0_2, V4 s0_3, V4 s0_4, V4 s0_5, V4 s0_6, V4 s0_7, V4 s0_8, V4 s1_0, V4 s1_1, V4 s1_2, V4 s1_3, V4 s1_4, V4 s1_5, V4 s1_6, V4 s1_7, V4 s1_8) {
	V4 r = 0.0;
	r += mul(s0_0, M4(1.282e-01, 1.199e-01, 1.156e-01, -4.091e-02, -1.771e-02, -1.431e-01, -1.478e-02, 4.041e-02, -1.559e-01, 1.231e-02, -8.571e-02, 2.159e-02, -6.484e-02, 3.819e-02, -3.386e-02, -3.344e-02));
	r += mul(s0_1, M4(6.131e-02, 1.493e-01, 1.954e-01, -2.565e-01, 1.570e-01, -3.852e-01, -2.313e-01, 9.262e-02, 1.038e-01, -4.169e-01, -2.446e-01, 9.953e-02, -1.830e-01, -9.774e-02, -1.498e-01, 8.626e-02));
	r += mul(s0_2, M4(9.908e-02, 1.372e-01, -1.254e-02, 4.486e-03, 1.023e-01, 6.484e-02, 1.645e-01, -4.932e-02, -4.221e-02, -1.919e-01, -2.135e-02, 6.955e-02, -1.406e-01, 8.082e-02, -7.935e-02, 3.010e-02));
	r += mul(s0_3, M4(-7.203e-02, -1.210e-01, 1.084e-01, -6.958e-03, 1.303e-01, 1.030e-01, -2.392e-01, -1.084e-01, 2.173e-01, -7.864e-02, -2.983e-01, -3.510e-01, -3.076e-01, 4.533e-02, 1.940e-01, 4.051e-01));
	r += mul(s0_4, M4(9.270e-02, -4.072e-01, 2.338e-01, 4.098e-01, -1.440e-01, 6.971e-01, 5.515e-01, 2.682e-01, -1.401e-01, 3.504e-02, 1.366e-01, 6.149e-01, -3.330e-01, 1.880e-01, -4.170e-01, 3.244e-01));
	r += mul(s0_5, M4(-5.380e-01, -7.843e-02, -1.293e-01, -9.225e-02, 1.393e-01, -2.588e-01, 4.618e-01, -2.264e-02, -5.369e-02, 1.321e-01, -3.029e-02, 7.983e-02, -1.048e-01, 3.279e-02, -5.969e-02, -3.766e-03));
	r += mul(s0_6, M4(3.432e-02, 1.518e-02, 1.940e-02, -1.086e-01, 1.052e-01, -5.430e-02, -3.343e-02, 1.824e-01, -9.831e-02, 1.097e-02, 6.281e-02, 1.194e-01, 3.253e-02, 4.046e-02, -2.183e-02, -1.328e-01));
	r += mul(s0_7, M4(1.538e-01, 6.796e-02, -4.870e-01, 7.139e-02, -2.497e-01, 2.916e-02, 6.191e-01, -2.650e-01, -4.194e-02, 1.782e-01, -3.431e-01, -9.707e-02, 2.173e-02, -1.150e-01, -8.162e-03, 4.551e-02));
	r += mul(s0_8, M4(5.804e-02, 5.436e-02, -1.604e-01, 8.077e-02, 2.685e-01, 4.741e-02, 1.225e-01, -1.033e-01, -4.358e-02, -1.091e-01, 8.815e-02, -3.121e-02, -2.569e-02, -1.093e-02, -2.550e-02, -1.571e-02));
	r += mul(s1_0, M4(8.760e-02, 1.254e-01, 9.299e-02, -1.140e-02, 4.179e-02, -1.333e-01, 3.048e-03, -3.111e-02, -6.091e-02, 6.563e-03, 4.609e-03, -4.717e-02, -6.470e-02, -5.791e-02, -5.529e-03, 8.697e-02));
	r += mul(s1_1, M4(6.935e-02, 9.805e-02, 1.851e-01, -2.726e-01, 1.731e-01, -2.863e-01, -2.267e-01, -3.813e-02, 1.104e-01, -3.193e-01, -1.958e-01, 9.567e-02, 1.819e-01, -2.054e-01, 1.228e-01, 3.906e-02));
	r += mul(s1_2, M4(-1.957e-01, 7.733e-02, -2.023e-01, 1.297e-01, -1.646e-01, 1.304e-01, -1.728e-02, -4.396e-02, 7.828e-02, -2.639e-01, 3.389e-02, 1.101e-01, 1.388e-01, -4.075e-03, 1.023e-01, -7.785e-03));
	r += mul(s1_3, M4(-2.828e-02, -7.018e-02, 4.269e-02, -1.386e-01, 2.143e-02, 2.504e-01, -2.134e-01, -2.483e-01, 1.075e-01, -2.671e-02, -2.588e-01, -3.271e-01, 1.173e-01, -6.103e-02, 5.539e-01, 5.341e-01));
	r += mul(s1_4, M4(-2.415e-01, -2.975e-01, -6.622e-02, 4.027e-01, -5.871e-01, 7.506e-01, 1.939e-02, -1.680e-01, 4.796e-01, -2.840e-01, 5.077e-01, 9.122e-02, 1.463e-01, 2.124e-01, 6.358e-02, 2.993e-01));
	r += mul(s1_5, M4(4.298e-01, -1.754e-01, 5.357e-01, -1.440e-01, -4.439e-01, -3.819e-01, -1.009e-01, 2.113e-02, -2.275e-02, -1.842e-02, 1.441e-01, 6.590e-03, 2.627e-02, 3.381e-02, 9.956e-02, -1.935e-02));
	r += mul(s1_6, M4(-5.557e-02, 3.378e-02, -2.451e-02, -1.718e-01, -2.037e-01, 1.631e-02, -2.822e-01, -7.724e-02, -6.657e-02, -2.282e-02, 2.673e-02, 8.716e-02, 1.291e-01, 9.472e-03, 3.810e-02, -1.134e-01));
	r += mul(s1_7, M4(1.441e-01, 4.331e-02, -4.741e-01, 2.165e-01, -5.974e-01, -2.669e-02, -4.949e-02, -3.179e-01, 1.007e-01, 1.512e-01, -4.138e-02, -7.470e-02, 8.828e-02, -1.400e-01, 5.797e-02, -4.988e-03));
	r += mul(s1_8, M4(-2.478e-01, 1.392e-01, -8.663e-02, -3.629e-02, 1.823e-01, 7.573e-03, -2.445e-01, -1.641e-02, -5.197e-02, -8.804e-02, 1.244e-01, 2.095e-02, 1.683e-02, -4.073e-02, -5.207e-03, -3.854e-03));
	r += V4(-4.317e-03, 2.687e-03, -1.530e-03, 4.681e-04);
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
//!DESC CuNNy-4x4C-BILINEAR-RGB-NVL-conv2
//!BLOCK_SIZE 8
//!NUM_THREADS 64
//!IN t1
//!OUT t0

#define l0(x, y) O(t1, float2(x, y))

float4 f0(V4 s0_0, V4 s0_1, V4 s0_2, V4 s0_3, V4 s0_4, V4 s0_5, V4 s0_6, V4 s0_7, V4 s0_8, V4 s1_0, V4 s1_1, V4 s1_2, V4 s1_3, V4 s1_4, V4 s1_5, V4 s1_6, V4 s1_7, V4 s1_8) {
	V4 r = 0.0;
	r += mul(s0_0, M4(1.921e-01, -2.132e-02, -5.460e-03, -6.681e-02, 9.988e-02, -2.228e-02, 4.719e-02, 9.124e-03, -1.072e-01, 1.506e-01, 2.070e-02, -4.671e-02, 2.244e-01, -4.895e-02, -8.150e-03, -9.520e-02));
	r += mul(s0_1, M4(8.226e-02, 4.651e-02, -1.842e-01, -3.376e-02, 1.349e-01, 2.148e-02, -1.746e-01, 1.671e-02, 9.761e-02, 7.581e-02, 1.470e-01, -8.582e-02, -1.149e-01, 2.143e-02, -1.597e-01, 1.626e-01));
	r += mul(s0_2, M4(-5.810e-04, -3.566e-02, 4.708e-02, -3.068e-02, 1.578e-02, 5.503e-03, 3.081e-02, -4.174e-02, 3.394e-01, 7.398e-02, -9.467e-02, -1.127e-01, -1.314e-01, 1.511e-02, 1.538e-01, -5.695e-03));
	r += mul(s0_3, M4(2.959e-01, 3.316e-02, -5.716e-02, -2.233e-01, 5.020e-01, -1.416e-01, -6.082e-02, -3.393e-01, 3.292e-01, -6.813e-02, 9.009e-02, -1.638e-01, 1.190e-01, -2.728e-02, -6.042e-02, -1.360e-01));
	r += mul(s0_4, M4(5.902e-01, 3.040e-01, -2.870e-01, 2.228e-02, -1.646e-01, 2.078e-02, -1.480e-01, 2.083e-01, -4.397e-01, -2.549e-01, -1.168e-01, -4.199e-01, 2.199e-01, 2.596e-02, 2.598e-02, -1.313e-01));
	r += mul(s0_5, M4(1.043e-01, 1.050e-02, -5.654e-02, -1.265e-01, -1.978e-01, 3.772e-02, 2.474e-01, 1.395e-01, 2.041e-01, 6.617e-02, -2.602e-01, -1.601e-01, -5.577e-02, -1.591e-02, 2.096e-01, 2.594e-02));
	r += mul(s0_6, M4(7.245e-02, 6.156e-02, 5.317e-02, -3.912e-01, 1.871e-01, -2.079e-02, -2.552e-02, -6.961e-02, 2.686e-01, 8.518e-02, -1.026e-01, -4.040e-01, -6.324e-02, 7.999e-03, 1.317e-02, 1.619e-02));
	r += mul(s0_7, M4(1.240e-01, -8.349e-02, -1.258e-01, -3.269e-01, 6.624e-01, -1.357e-01, -6.738e-01, -5.998e-01, -8.375e-04, 2.226e-01, -1.880e-01, 5.678e-02, -8.383e-02, -3.455e-02, -1.399e-02, 4.540e-02));
	r += mul(s0_8, M4(-3.130e-02, 9.691e-02, 1.763e-01, -1.847e-02, -1.193e-01, -7.494e-03, 1.485e-02, 1.244e-02, 9.559e-02, 3.116e-02, 8.046e-03, -1.264e-01, -2.403e-01, 6.389e-02, 2.999e-01, 1.484e-01));
	r += mul(s1_0, M4(2.569e-01, -8.689e-03, -1.806e-02, -3.993e-02, 9.155e-02, -2.022e-02, 1.034e-02, -3.455e-02, -1.534e-01, 1.836e-02, -1.176e-03, 3.593e-03, 2.642e-01, -6.587e-02, -4.169e-02, -2.237e-01));
	r += mul(s1_1, M4(1.398e-01, 1.020e-02, -2.478e-01, 2.747e-02, 7.152e-02, 1.835e-02, -2.013e-01, 1.151e-02, -2.586e-01, -3.622e-02, 2.529e-01, 1.465e-01, -3.973e-01, 5.907e-02, -9.450e-02, 3.761e-02));
	r += mul(s1_2, M4(3.157e-02, 7.847e-03, 8.109e-03, -3.333e-02, -3.333e-02, -6.401e-03, -6.632e-03, 3.296e-02, -1.433e-02, 2.167e-02, 1.194e-01, -1.028e-01, -2.104e-01, 1.352e-02, -6.835e-02, 1.901e-01));
	r += mul(s1_3, M4(3.443e-01, -1.004e-01, -6.176e-02, -3.047e-01, 4.779e-01, -7.928e-02, -8.134e-02, -4.873e-01, -1.421e-01, 3.972e-02, 7.459e-02, 2.099e-01, 1.118e-01, -1.022e-02, -8.584e-02, -1.657e-01));
	r += mul(s1_4, M4(-1.721e-01, 2.625e-02, -7.292e-03, 2.646e-01, 2.505e-02, 1.479e-01, -3.357e-01, 1.088e-01, 1.016e-01, -1.902e-01, -1.622e-01, -6.326e-02, -4.305e-01, 4.763e-01, -1.357e-03, -5.685e-01));
	r += mul(s1_5, M4(3.324e-03, 1.692e-02, -5.726e-02, 2.853e-02, -3.135e-01, -4.534e-03, 2.549e-01, 1.183e-01, -1.277e-01, -5.030e-02, 9.190e-02, 1.145e-01, 3.445e-01, 6.425e-02, -2.707e-01, -1.701e-01));
	r += mul(s1_6, M4(2.164e-02, 1.998e-02, 1.667e-02, -6.126e-02, 2.400e-01, -9.253e-02, -4.525e-02, 8.615e-03, 5.148e-02, -1.803e-02, -7.495e-02, -7.102e-02, -2.646e-02, 6.819e-02, 1.465e-01, 1.904e-01));
	r += mul(s1_7, M4(-2.339e-02, 3.350e-02, -1.274e-01, 5.525e-02, 9.120e-01, -9.074e-01, -6.856e-01, -7.422e-02, 4.849e-02, -1.377e-02, -1.409e-01, -5.792e-02, -1.044e-01, 9.079e-02, 2.520e-01, 2.053e-01));
	r += mul(s1_8, M4(1.891e-02, -1.562e-02, -1.024e-02, -2.686e-02, -1.038e-01, -3.210e-02, 4.222e-01, -2.084e-01, -1.841e-01, 3.231e-02, 7.320e-02, 1.727e-01, 2.861e-01, 2.506e-02, -2.266e-01, -3.940e-01));
	r += V4(-1.043e-03, 3.601e-03, 5.622e-03, -7.848e-04);
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
//!DESC CuNNy-4x4C-BILINEAR-RGB-NVL-conv3
//!BLOCK_SIZE 8
//!NUM_THREADS 64
//!IN t0
//!OUT t1

#define l0(x, y) O(t0, float2(x, y))

float4 f0(V4 s0_0, V4 s0_1, V4 s0_2, V4 s0_3, V4 s0_4, V4 s0_5, V4 s0_6, V4 s0_7, V4 s0_8, V4 s1_0, V4 s1_1, V4 s1_2, V4 s1_3, V4 s1_4, V4 s1_5, V4 s1_6, V4 s1_7, V4 s1_8) {
	V4 r = 0.0;
	r += mul(s0_0, M4(-7.801e-03, 7.517e-03, 5.348e-02, 7.686e-02, -8.770e-03, 1.144e-02, -2.398e-02, 1.355e-02, -4.642e-02, 5.880e-02, 3.263e-02, 1.860e-01, -4.443e-02, -2.732e-02, -2.133e-02, -1.166e-01));
	r += mul(s0_1, M4(-1.751e-02, -1.230e-02, -1.218e-01, -1.231e-01, 4.092e-03, -8.769e-03, -2.251e-03, 5.142e-02, 4.354e-03, -4.445e-02, -2.369e-01, -1.616e-01, 4.495e-03, -1.326e-01, -5.371e-01, -5.119e-01));
	r += mul(s0_2, M4(3.143e-02, 2.366e-02, 8.884e-02, -1.819e-02, 2.358e-03, 3.812e-04, -4.972e-02, -5.311e-02, 1.729e-02, 1.523e-02, 7.798e-02, -1.705e-05, -2.295e-02, 6.567e-02, 1.422e-01, 1.890e-01));
	r += mul(s0_3, M4(2.363e-02, 1.555e-02, -1.307e-01, -8.190e-02, 1.026e-02, 9.724e-03, 5.358e-02, -2.783e-01, 7.268e-03, 1.659e-01, -5.801e-02, 3.076e-01, -1.575e-01, -9.567e-02, 3.294e-02, -7.694e-01));
	r += mul(s0_4, M4(1.677e-02, -1.324e-01, 4.019e-01, -2.902e-01, -6.051e-02, -4.625e-02, 8.409e-01, 4.756e-01, -1.135e-01, -3.213e-01, 6.389e-02, -2.083e-01, -1.219e+00, 2.280e-01, 9.667e-01, -3.604e-01));
	r += mul(s0_5, M4(-5.948e-02, 1.567e-01, 3.883e-02, -4.843e-03, -2.153e-02, 3.439e-02, -1.160e-01, -1.325e-02, -5.312e-02, 1.136e-01, -5.260e-02, -3.524e-02, 7.315e-02, 3.527e-01, 6.186e-01, -7.505e-02));
	r += mul(s0_6, M4(-3.841e-02, 1.620e-03, 9.449e-02, -8.648e-02, -2.656e-02, -1.676e-03, 2.364e-03, -7.221e-02, -9.590e-02, 4.160e-02, -1.278e-02, -3.171e-02, 6.213e-02, 2.673e-02, -7.931e-02, 2.588e-01));
	r += mul(s0_7, M4(-3.636e-02, -1.558e-01, 2.151e-01, 1.188e-01, 1.275e-01, -8.114e-02, -8.376e-02, -3.690e-02, -1.968e-02, -1.038e-01, 8.994e-02, 3.846e-02, -1.499e-01, 6.457e-01, -8.201e-02, -3.935e-01));
	r += mul(s0_8, M4(-2.833e-03, 2.529e-01, -3.350e-03, -3.433e-02, 1.943e-02, -2.796e-02, 3.313e-02, 1.582e-02, 1.702e-02, 5.663e-02, -1.647e-02, -2.229e-02, -4.865e-01, 3.285e-01, -4.462e-01, -4.307e-01));
	r += mul(s1_0, M4(-6.004e-02, 4.898e-03, 3.591e-02, 1.900e-01, -3.816e-02, -3.269e-02, 1.459e-01, -3.464e-03, -1.235e-02, -3.737e-02, 1.569e-02, 2.559e-01, -3.173e-04, 1.268e-02, 8.886e-03, 2.960e-02));
	r += mul(s1_1, M4(-1.582e-02, -7.507e-02, -2.026e-01, 2.027e-01, -6.107e-02, 2.055e-02, -5.811e-02, 5.420e-03, 1.028e-02, -1.374e-02, -6.152e-01, -2.259e-01, -3.408e-03, -1.800e-02, 4.574e-02, -9.590e-02));
	r += mul(s1_2, M4(4.210e-02, 2.126e-02, 8.277e-02, 2.079e-02, -1.733e-01, -2.483e-02, 2.686e-01, 1.498e-01, 7.352e-02, -2.511e-02, 3.159e-02, 5.775e-02, 5.942e-02, 3.383e-02, 1.274e-01, -5.928e-02));
	r += mul(s1_3, M4(5.614e-02, 7.561e-02, -8.328e-02, 2.427e-01, 7.214e-02, -1.122e-01, 9.434e-02, -2.602e-01, -1.052e-02, -6.944e-02, -3.023e-02, -1.655e-01, 1.236e-03, 4.025e-03, -3.082e-02, -1.533e-01));
	r += mul(s1_4, M4(6.675e-01, -2.254e-01, 1.173e+00, -8.261e-02, 5.655e-01, -2.000e-01, 8.301e-01, 1.458e+00, -2.497e-01, -1.091e+00, -4.698e-01, -1.876e-01, -3.358e-02, -2.854e-01, 5.032e-01, -1.558e-01));
	r += mul(s1_5, M4(-1.444e-02, 1.502e-01, -4.221e-02, -4.864e-02, 3.236e-01, -2.572e-01, 1.344e-01, 8.562e-02, -1.030e-01, 2.690e-01, 1.238e-01, 3.309e-02, -3.849e-02, 1.860e-01, 6.528e-03, 2.840e-02));
	r += mul(s1_6, M4(-1.161e-01, 5.405e-02, -3.101e-02, -1.009e-01, -9.594e-02, -1.207e-02, -3.836e-02, -6.894e-02, -1.770e-02, -2.958e-02, 8.484e-02, -2.284e-02, 2.585e-04, -2.764e-02, 4.972e-02, -5.968e-02));
	r += mul(s1_7, M4(-4.113e-02, -1.948e-01, -2.728e-02, -3.142e-02, -2.894e-01, -1.111e-01, 7.492e-02, -2.892e-02, 9.054e-02, 4.350e-02, 2.183e-01, 1.489e-01, 1.167e-02, -6.678e-02, 3.696e-02, -1.315e-02));
	r += mul(s1_8, M4(2.532e-02, 4.585e-02, -3.694e-02, -6.244e-02, -1.673e-01, 6.180e-02, -4.475e-02, 1.028e-02, -1.658e-02, 8.923e-02, 1.711e-02, 3.037e-03, 4.651e-02, 1.652e-01, 7.863e-03, -3.387e-02));
	r += V4(-6.562e-04, 7.371e-04, -4.319e-03, -8.757e-04);
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
//!DESC CuNNy-4x4C-BILINEAR-RGB-NVL-conv4
//!BLOCK_SIZE 8
//!NUM_THREADS 64
//!IN t1
//!OUT t0

#define l0(x, y) O(t1, float2(x, y))

float4 f0(V4 s0_0, V4 s0_1, V4 s0_2, V4 s0_3, V4 s0_4, V4 s0_5, V4 s0_6, V4 s0_7, V4 s0_8, V4 s1_0, V4 s1_1, V4 s1_2, V4 s1_3, V4 s1_4, V4 s1_5, V4 s1_6, V4 s1_7, V4 s1_8) {
	V4 r = 0.0;
	r += mul(s0_0, M4(-1.087e-01, -5.083e-02, 3.146e-01, -4.241e-02, 4.462e-02, -4.358e-02, -1.562e-01, -2.609e-03, 5.918e-02, -2.526e-02, -3.132e-02, -1.150e-02, -8.799e-03, 3.070e-02, -1.680e-02, -1.046e-02));
	r += mul(s0_1, M4(1.762e-01, 8.784e-01, -2.704e+00, -1.565e+00, -1.473e-01, -5.723e-01, 7.838e-02, -7.420e-03, -1.769e-01, -2.041e-01, -1.783e-03, -4.944e-03, 1.304e-02, 2.646e-01, -1.708e-01, 7.483e-03));
	r += mul(s0_2, M4(-1.907e-01, 1.514e-01, -3.657e-01, -5.840e-01, -4.943e-02, -1.014e-02, -2.869e-03, 6.488e-03, 2.266e-02, -3.850e-02, 6.125e-03, 1.899e-02, -3.541e-02, -2.011e-01, 1.567e-01, 1.008e-02));
	r += mul(s0_3, M4(-3.061e-01, -1.768e-01, 9.163e-02, -2.243e-01, 4.945e-02, 1.106e-01, -1.137e-01, 1.755e-02, 2.640e-01, -9.298e-02, -1.704e-01, 3.935e-02, 1.506e-01, -3.284e-02, 4.719e-02, 5.543e-02));
	r += mul(s0_4, M4(-4.579e-01, -6.198e-02, -9.889e-01, -4.446e-01, -1.612e-01, 1.518e-01, 2.588e-01, 1.075e-02, -1.527e+00, -7.923e-01, 8.120e-02, -1.116e-01, -2.079e-01, -1.206e-01, -4.422e-01, -1.951e-01));
	r += mul(s0_5, M4(1.064e-01, -1.684e-01, 2.316e-01, 4.211e-01, -9.153e-02, 9.155e-02, -7.649e-02, -1.385e-01, 9.422e-02, -1.631e-01, 8.278e-02, 3.318e-01, 7.284e-02, 3.489e-01, -2.303e-02, -6.554e-01));
	r += mul(s0_6, M4(-6.320e-02, -4.390e-02, 1.453e-02, 3.187e-02, 2.166e-02, 2.423e-03, 1.573e-03, -2.226e-02, 1.401e-01, 2.026e-01, -2.249e-01, 6.471e-02, 3.593e-02, -1.575e-02, -3.186e-02, 1.339e-02));
	r += mul(s0_7, M4(2.778e-02, 7.495e-02, -1.086e-01, 8.862e-02, -2.352e-02, 1.477e-02, 2.741e-02, 4.345e-02, -2.865e-01, 9.405e-02, 1.880e-01, -3.610e-01, -7.797e-02, -5.710e-03, 3.386e-02, 2.830e-02));
	r += mul(s0_8, M4(-3.734e-02, 3.357e-02, 5.657e-03, -1.596e-01, -7.661e-03, 1.603e-02, -3.137e-02, -7.023e-03, 6.522e-03, -2.715e-02, 2.765e-02, 4.724e-02, 1.922e-02, 3.944e-02, -8.276e-02, -1.915e-02));
	r += mul(s1_0, M4(-7.121e-02, -2.276e-02, 7.266e-02, -4.411e-03, -5.600e-01, 4.502e-01, -1.817e-01, -2.906e-01, -5.675e-02, 2.653e-02, 3.284e-02, -1.925e-03, -4.729e-03, -1.554e-03, -6.081e-03, -2.195e-02));
	r += mul(s1_1, M4(2.212e-01, 3.154e-01, -2.765e-01, 4.432e-02, 1.402e+00, 2.159e-01, 4.402e-01, 2.537e-01, 6.697e-02, 1.207e-01, -5.192e-02, 2.638e-02, 5.366e-02, 5.855e-02, -3.687e-02, 4.389e-03));
	r += mul(s1_2, M4(3.137e-02, -1.157e-01, 9.497e-02, -3.724e-02, 5.241e-02, 7.793e-02, 2.277e-04, -4.033e-01, 1.432e-02, 4.622e-02, -1.636e-02, -5.840e-03, -1.593e-02, -7.447e-02, 3.943e-02, -3.517e-03));
	r += mul(s1_3, M4(-1.209e-02, -1.350e-01, 3.018e-01, 1.233e-01, -1.262e-03, 2.194e-01, -2.919e-01, -8.031e-03, 4.620e-03, 5.318e-02, 1.247e-02, -4.260e-02, 7.155e-02, 3.256e-02, -9.839e-02, -6.741e-04));
	r += mul(s1_4, M4(3.291e-01, 2.397e-01, -2.820e-01, 5.703e-01, 7.831e-03, 5.816e-02, -1.696e-02, -1.957e-01, -1.851e-01, 3.696e-02, -2.611e-01, 7.039e-03, -1.562e-01, -7.676e-01, 9.080e-01, 7.823e-02));
	r += mul(s1_5, M4(9.918e-03, 6.364e-02, 3.364e-02, -3.291e-01, 1.393e-02, 3.139e-02, 1.701e-02, -5.675e-02, 5.085e-02, -2.050e-01, 1.160e-01, 4.875e-02, -1.189e-01, 2.310e-01, -1.353e-01, 2.046e-02));
	r += mul(s1_6, M4(-5.477e-03, -1.704e-02, 9.510e-03, -1.701e-02, 1.391e-02, -8.760e-03, -3.355e-02, -6.898e-03, -9.203e-03, -2.442e-02, 7.547e-03, 1.817e-02, 1.871e-02, -1.149e-02, 6.458e-02, 1.403e-02));
	r += mul(s1_7, M4(-5.073e-03, -5.454e-02, -2.710e-02, 1.292e-02, 2.458e-02, 1.739e-02, -2.319e-03, 3.865e-02, 5.399e-02, -1.176e-02, -1.315e-01, 1.489e-01, -7.903e-02, 8.120e-02, 4.749e-02, 1.961e-01));
	r += mul(s1_8, M4(4.163e-02, -1.603e-02, 8.659e-03, 1.023e-01, 5.233e-03, -2.900e-03, -5.293e-03, -5.829e-03, -1.453e-02, 2.467e-02, 7.198e-02, -2.407e-01, -4.023e-02, 1.009e-01, -1.560e-01, -1.567e-01));
	r += V4(-3.709e-04, 2.029e-04, -3.042e-03, -2.970e-04);
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
//!DESC CuNNy-4x4C-BILINEAR-RGB-NVL-out-shuffle
//!BLOCK_SIZE 16
//!NUM_THREADS 64
//!IN INPUT, t0
//!OUT OUTPUT

#define l0(x, y) O(t0, float2(x, y))

float4 f0(V4 s0_0, V4 s0_1, V4 s0_2, V4 s0_3, V4 s0_4, V4 s0_5, V4 s0_6, V4 s0_7, V4 s0_8, V4 s1_0, V4 s1_1, V4 s1_2, V4 s1_3, V4 s1_4, V4 s1_5, V4 s1_6, V4 s1_7, V4 s1_8) {
	V4 r = 0.0;
	r += mul(s0_0, M4(-6.857e-02, -6.042e-02, 3.293e-03, -2.389e-03, -1.606e-01, -1.556e-02, -5.115e-02, -4.602e-02, -3.762e-02, 1.994e-02, -2.370e-02, 3.558e-02, -7.142e-01, 8.184e-01, -1.361e-01, 1.228e-01));
	r += mul(s0_1, M4(-1.887e-01, -2.260e-01, 1.293e-02, -1.757e-02, 1.257e-01, 1.304e-01, -4.525e-02, 4.471e-02, 6.895e-01, -4.096e-01, 4.096e-02, 1.817e-02, -1.343e-01, -4.170e-01, 3.991e-03, 1.516e-03));
	r += mul(s0_2, M4(-2.667e-01, -8.692e-02, 1.481e-01, -1.466e-01, 6.142e-02, -2.084e-02, 1.942e-02, 6.700e-04, 3.942e-02, 3.109e-01, -1.323e-02, 2.240e-02, -2.306e-02, -4.749e-02, -1.155e-02, 1.843e-03));
	r += mul(s0_3, M4(-1.004e-01, -1.184e-02, -8.590e-02, -1.018e-01, 6.862e-02, -4.700e-02, -1.537e-01, -1.096e-01, -1.228e-01, 1.462e-02, -1.715e-01, 1.862e-02, 3.668e-01, -1.138e-01, 8.494e-04, 6.113e-01));
	r += mul(s0_4, M4(4.389e-01, -5.527e-01, -4.972e-01, -7.620e-01, 1.684e-01, 5.375e-02, 1.032e+00, 5.723e-01, 4.427e-02, -2.447e-01, 1.132e+00, -5.297e-01, 1.150e-01, 3.877e-01, 1.224e-01, 1.294e-01));
	r += mul(s0_5, M4(-1.023e+00, 1.567e+00, -9.747e-01, 1.051e+00, 1.537e-02, 1.993e-01, -1.679e-01, 1.139e-01, -7.358e-02, -1.782e-01, -1.938e-01, 4.419e-02, 2.001e-02, 5.881e-02, 8.971e-03, 3.368e-03));
	r += mul(s0_6, M4(-5.126e-03, 1.449e-02, -7.018e-02, 2.929e-02, 4.748e-02, -4.443e-03, -5.791e-02, -3.490e-02, 3.817e-02, 1.007e-02, -5.501e-02, -1.488e-02, -8.848e-03, 4.884e-02, -6.548e-02, 3.392e-02));
	r += mul(s0_7, M4(-4.449e-02, 7.313e-02, 3.311e-01, 3.138e-02, -6.466e-02, 5.666e-02, 1.929e-01, 8.274e-02, 3.994e-02, 2.105e-02, -1.821e-01, -1.539e-02, -9.333e-03, -4.728e-02, 6.975e-03, -3.292e-03));
	r += mul(s0_8, M4(2.038e-01, -2.356e-01, -1.987e-01, -3.746e-02, -1.499e-02, -7.007e-02, -9.546e-02, 1.905e-02, -9.802e-03, 1.990e-02, 2.140e-02, -8.164e-03, 5.109e-03, -2.081e-02, -2.386e-02, 1.183e-02));
	r += mul(s1_0, M4(-7.067e-02, -4.613e-02, -5.433e-04, -2.191e-02, -1.125e-01, -3.650e-02, -1.298e-02, -3.479e-02, -1.118e-01, -1.521e-02, -4.731e-03, -7.478e-03, 1.802e-01, 4.872e-02, -1.599e-03, -1.452e-02));
	r += mul(s1_1, M4(-2.920e-01, -1.831e-01, -1.305e-02, 4.031e-02, 1.989e-01, 3.120e-03, 2.025e-02, 5.432e-02, 2.607e-01, 2.403e-02, 1.863e-02, 8.423e-02, -3.372e-01, -1.327e-01, -1.248e-01, -1.247e-01));
	r += mul(s1_2, M4(-9.286e-02, -1.948e-01, -8.532e-03, 7.416e-03, 4.578e-02, 1.581e-01, 1.473e-03, -3.796e-02, 1.011e-01, 2.393e-01, 2.742e-02, -4.224e-02, -9.579e-03, -9.888e-02, -2.065e-03, 7.685e-03));
	r += mul(s1_3, M4(-2.056e-01, -3.479e-02, -2.666e-01, -5.344e-02, 1.579e-01, -6.091e-02, -1.655e-01, -1.575e-01, -8.230e-02, -4.748e-02, -1.304e-01, -7.186e-02, 2.953e-01, 6.950e-02, 1.865e-01, 7.567e-02));
	r += mul(s1_4, M4(3.408e-01, -1.054e-01, -2.613e-01, -6.084e-01, 3.193e-01, 6.366e-01, 4.251e-01, 4.066e-01, -3.742e-01, -8.521e-02, 5.906e-01, 1.870e-01, 2.044e-02, 2.495e-01, 1.046e-01, 3.018e-01));
	r += mul(s1_5, M4(4.748e-03, 2.086e-01, 4.231e-03, -7.764e-03, 3.933e-02, 3.446e-03, -3.431e-02, 8.415e-02, -3.798e-02, -3.428e-01, -7.206e-02, 2.392e-01, 2.157e-02, 2.692e-02, 3.313e-02, 1.841e-02));
	r += mul(s1_6, M4(1.813e-02, 2.306e-03, -3.402e-02, 1.009e-03, 4.408e-02, -2.307e-02, -3.394e-02, -3.912e-02, 3.822e-02, -1.051e-02, -1.023e-01, -4.626e-02, -4.871e-02, 6.250e-03, 1.367e-01, 3.674e-02));
	r += mul(s1_7, M4(-1.170e-02, 3.747e-02, 1.548e-01, 1.243e-01, -1.074e-01, -9.848e-03, 2.627e-01, 1.132e-01, 4.550e-02, 5.050e-02, -1.194e-01, -6.091e-02, -2.180e-02, -6.381e-02, -5.949e-02, 1.580e-02));
	r += mul(s1_8, M4(-1.146e-04, -1.852e-02, -1.515e-02, 2.488e-02, -1.877e-02, -7.739e-02, -6.812e-02, 7.656e-03, 2.688e-02, 5.650e-02, 4.285e-02, -3.270e-02, 1.163e-03, 8.328e-04, -1.998e-02, -2.282e-02));
	r += V4(-3.259e-04, -3.197e-04, 4.954e-04, 4.568e-04);
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
