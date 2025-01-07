#define MP_BLOCK_WIDTH 8
#define MP_BLOCK_HEIGHT 8
#define MP_NUM_THREADS_X 64
#define MP_NUM_THREADS_Y 1
#define MP_NUM_THREADS_Z 1

#define MF float
#define MF1 float1
#define MF2 float2
#define MF3 float3
#define MF4 float4
#define MF1x1 float1x1
#define MF1x2 float1x2
#define MF1x3 float1x3
#define MF1x4 float1x4
#define MF2x1 float2x1
#define MF2x2 float2x2
#define MF2x3 float2x3
#define MF2x4 float2x4
#define MF3x1 float3x1
#define MF3x2 float3x2
#define MF3x3 float3x3
#define MF3x4 float3x4
#define MF4x1 float4x1
#define MF4x2 float4x2
#define MF4x3 float4x3
#define MF4x4 float4x4

uint2 Rmp8x8(uint a) { return uint2(a / 8, a % 8); }
uint2 GetInputSize() { return uint2(0, 0); }
float2 GetInputPt() { return float2(0, 0); }
uint2 GetOutputSize() { return float2(0, 0); }
float2 GetOutputPt() { return float2(0, 0); }
float2 GetScale() { return float2(0, 0); }
MF2 MulAdd(MF2 x, MF2x2 y, MF2 a) { return mul(x, y) + a; }
MF3 MulAdd(MF2 x, MF2x3 y, MF3 a) { return mul(x, y) + a; }
MF4 MulAdd(MF2 x, MF2x4 y, MF4 a) { return mul(x, y) + a; }
MF2 MulAdd(MF3 x, MF3x2 y, MF2 a) { return mul(x, y) + a; }
MF3 MulAdd(MF3 x, MF3x3 y, MF3 a) { return mul(x, y) + a; }
MF4 MulAdd(MF3 x, MF3x4 y, MF4 a) { return mul(x, y) + a; }
MF2 MulAdd(MF4 x, MF4x2 y, MF2 a) { return mul(x, y) + a; }
MF3 MulAdd(MF4 x, MF4x3 y, MF3 a) { return mul(x, y) + a; }
MF4 MulAdd(MF4 x, MF4x4 y, MF4 a) { return mul(x, y) + a; }
uint GetFrameCount() { return 0; }
