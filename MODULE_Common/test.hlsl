#line 1 "C:\\Users\\LiuXu\\source\\repos\\Magpie\\MODULE_Common\\FfxEasuShader.hlsl"
cbuffer constants : register ( b0 ) { 
    int2 srcSize : packoffset ( c0 . x ) ; 
    int2 destSize : packoffset ( c0 . z ) ; 
} ; 




#line 1 "C:\\Users\\LiuXu\\source\\repos\\Magpie\\EffectCommon\\common.hlsli"















#line 18






#line 25


#line 33


static float2 maxCoord0 ; 


#line 90





#line 1 "C:\\Program Files (x86)\\Windows Kits\\10\\Include\\10.0.19041.0\\um\\d2d1effecthelpers.hlsli"


#line 27


#line 30

Texture2D < float4 > InputTexture0 : register ( t0 ) ; SamplerState InputSampler0 : register ( s0 ) ; 


#line 35


#line 38


#line 41


#line 44


#line 47


#line 50


#line 53




#line 60


#line 64


#line 69

static float4 __d2dstatic_scenePos = float4 ( 0 , 0 , 0 , 0 ) ; 


#line 75



static float4 __d2dstatic_input0 = float4 ( 0 , 0 , 0 , 0 ) ; static float4 __d2dstatic_uv0 = float4 ( 0 , 0 , 0 , 0 ) ; 


#line 82


#line 85


#line 88


#line 91


#line 94


#line 97


#line 100


#line 105


#line 109





#line 116


#line 124










#line 139






#line 148






#line 157






#line 166






#line 175






#line 184






#line 193






#line 202






#line 211






#line 221




#line 245






#line 253


#line 276


#line 279




#line 284
float4 D2D_ENTRY_Impl ( ) ; 

#line 287
float4 D2D_ENTRY ( float4 pos : SV_POSITION , float4 __d2dinput_scenePos : SCENE_POSITION , float4 __d2dinput_uv0 : TEXCOORD0 ) : SV_TARGET 
{ 
    __d2dstatic_scenePos = __d2dinput_scenePos ; __d2dstatic_uv0 = __d2dinput_uv0 ; 
    return D2D_ENTRY_Impl ( ) ; 
} 





#line 302

inline float4 D2DGetScenePosition ( ) 
{ 
    return __d2dstatic_scenePos ; 
} 












#line 95 "C:\\Users\\LiuXu\\source\\repos\\Magpie\\EffectCommon\\common.hlsli"






#line 102


#line 105












#line 118


#line 122
void InitMagpieSampleInput ( ) { 
    
    maxCoord0 = float2 ( ( srcSize . x - 1 ) * ( __d2dstatic_uv0 ) . z , ( srcSize . y - 1 ) * ( __d2dstatic_uv0 ) . w ) ; 
    
#line 145
    
    
} 

void InitMagpieSampleInputWithScale ( float2 scale ) { 
    InitMagpieSampleInput ( ) ; 
    
#line 153
    ( __d2dstatic_uv0 ) . xy /= scale ; 
} 

#line 157


#line 181



#line 10 "C:\\Users\\LiuXu\\source\\repos\\Magpie\\MODULE_Common\\FfxEasuShader.hlsl"





#line 1 "C:\\Users\\LiuXu\\source\\repos\\Magpie\\MODULE_Common\\ffx_a.hlsli"


#line 94


#line 110


#line 131
































uint AU1_AH1_AF1_x ( float a ) { return f32tof16 ( a ) ; } 


uint AU1_AH2_AF2_x ( float2 a ) { return f32tof16 ( a . x ) | ( f32tof16 ( a . y ) << 16 ) ; } 



float2 AF2_AH2_AU1_x ( uint x ) { return float2 ( f16tof32 ( x & 0xFFFF ) , f16tof32 ( x >> 16 ) ) ; } 


float AF1_x ( float a ) { return float ( a ) ; } 
float2 AF2_x ( float a ) { return float2 ( a , a ) ; } 
float3 AF3_x ( float a ) { return float3 ( a , a , a ) ; } 
float4 AF4_x ( float a ) { return float4 ( a , a , a , a ) ; } 





uint AU1_x ( uint a ) { return uint ( a ) ; } 
uint2 AU2_x ( uint a ) { return uint2 ( a , a ) ; } 
uint3 AU3_x ( uint a ) { return uint3 ( a , a , a ) ; } 
uint4 AU4_x ( uint a ) { return uint4 ( a , a , a , a ) ; } 





uint AAbsSU1 ( uint a ) { return uint ( abs ( int ( a ) ) ) ; } 
uint2 AAbsSU2 ( uint2 a ) { return uint2 ( abs ( int2 ( a ) ) ) ; } 
uint3 AAbsSU3 ( uint3 a ) { return uint3 ( abs ( int3 ( a ) ) ) ; } 
uint4 AAbsSU4 ( uint4 a ) { return uint4 ( abs ( int4 ( a ) ) ) ; } 

uint ABfe ( uint src , uint off , uint bits ) { uint mask = ( 1u << bits ) - 1 ; return ( src >> off ) & mask ; } 
uint ABfi ( uint src , uint ins , uint mask ) { return ( ins & mask ) | ( src & ( ~ mask ) ) ; } 
uint ABfiM ( uint src , uint ins , uint bits ) { uint mask = ( 1u << bits ) - 1 ; return ( ins & mask ) | ( src & ( ~ mask ) ) ; } 

float AClampF1 ( float x , float n , float m ) { return max ( n , min ( x , m ) ) ; } 
float2 AClampF2 ( float2 x , float2 n , float2 m ) { return max ( n , min ( x , m ) ) ; } 
float3 AClampF3 ( float3 x , float3 n , float3 m ) { return max ( n , min ( x , m ) ) ; } 
float4 AClampF4 ( float4 x , float4 n , float4 m ) { return max ( n , min ( x , m ) ) ; } 

float AFractF1 ( float x ) { return x - floor ( x ) ; } 
float2 AFractF2 ( float2 x ) { return x - floor ( x ) ; } 
float3 AFractF3 ( float3 x ) { return x - floor ( x ) ; } 
float4 AFractF4 ( float4 x ) { return x - floor ( x ) ; } 

float ALerpF1 ( float x , float y , float a ) { return lerp ( x , y , a ) ; } 
float2 ALerpF2 ( float2 x , float2 y , float2 a ) { return lerp ( x , y , a ) ; } 
float3 ALerpF3 ( float3 x , float3 y , float3 a ) { return lerp ( x , y , a ) ; } 
float4 ALerpF4 ( float4 x , float4 y , float4 a ) { return lerp ( x , y , a ) ; } 

float AMax3F1 ( float x , float y , float z ) { return max ( x , max ( y , z ) ) ; } 
float2 AMax3F2 ( float2 x , float2 y , float2 z ) { return max ( x , max ( y , z ) ) ; } 
float3 AMax3F3 ( float3 x , float3 y , float3 z ) { return max ( x , max ( y , z ) ) ; } 
float4 AMax3F4 ( float4 x , float4 y , float4 z ) { return max ( x , max ( y , z ) ) ; } 

uint AMax3SU1 ( uint x , uint y , uint z ) { return uint ( max ( int ( x ) , max ( int ( y ) , int ( z ) ) ) ) ; } 
uint2 AMax3SU2 ( uint2 x , uint2 y , uint2 z ) { return uint2 ( max ( int2 ( x ) , max ( int2 ( y ) , int2 ( z ) ) ) ) ; } 
uint3 AMax3SU3 ( uint3 x , uint3 y , uint3 z ) { return uint3 ( max ( int3 ( x ) , max ( int3 ( y ) , int3 ( z ) ) ) ) ; } 
uint4 AMax3SU4 ( uint4 x , uint4 y , uint4 z ) { return uint4 ( max ( int4 ( x ) , max ( int4 ( y ) , int4 ( z ) ) ) ) ; } 

uint AMax3U1 ( uint x , uint y , uint z ) { return max ( x , max ( y , z ) ) ; } 
uint2 AMax3U2 ( uint2 x , uint2 y , uint2 z ) { return max ( x , max ( y , z ) ) ; } 
uint3 AMax3U3 ( uint3 x , uint3 y , uint3 z ) { return max ( x , max ( y , z ) ) ; } 
uint4 AMax3U4 ( uint4 x , uint4 y , uint4 z ) { return max ( x , max ( y , z ) ) ; } 

uint AMaxSU1 ( uint a , uint b ) { return uint ( max ( int ( a ) , int ( b ) ) ) ; } 
uint2 AMaxSU2 ( uint2 a , uint2 b ) { return uint2 ( max ( int2 ( a ) , int2 ( b ) ) ) ; } 
uint3 AMaxSU3 ( uint3 a , uint3 b ) { return uint3 ( max ( int3 ( a ) , int3 ( b ) ) ) ; } 
uint4 AMaxSU4 ( uint4 a , uint4 b ) { return uint4 ( max ( int4 ( a ) , int4 ( b ) ) ) ; } 

float AMed3F1 ( float x , float y , float z ) { return max ( min ( x , y ) , min ( max ( x , y ) , z ) ) ; } 
float2 AMed3F2 ( float2 x , float2 y , float2 z ) { return max ( min ( x , y ) , min ( max ( x , y ) , z ) ) ; } 
float3 AMed3F3 ( float3 x , float3 y , float3 z ) { return max ( min ( x , y ) , min ( max ( x , y ) , z ) ) ; } 
float4 AMed3F4 ( float4 x , float4 y , float4 z ) { return max ( min ( x , y ) , min ( max ( x , y ) , z ) ) ; } 

float AMin3F1 ( float x , float y , float z ) { return min ( x , min ( y , z ) ) ; } 
float2 AMin3F2 ( float2 x , float2 y , float2 z ) { return min ( x , min ( y , z ) ) ; } 
float3 AMin3F3 ( float3 x , float3 y , float3 z ) { return min ( x , min ( y , z ) ) ; } 
float4 AMin3F4 ( float4 x , float4 y , float4 z ) { return min ( x , min ( y , z ) ) ; } 

uint AMin3SU1 ( uint x , uint y , uint z ) { return uint ( min ( int ( x ) , min ( int ( y ) , int ( z ) ) ) ) ; } 
uint2 AMin3SU2 ( uint2 x , uint2 y , uint2 z ) { return uint2 ( min ( int2 ( x ) , min ( int2 ( y ) , int2 ( z ) ) ) ) ; } 
uint3 AMin3SU3 ( uint3 x , uint3 y , uint3 z ) { return uint3 ( min ( int3 ( x ) , min ( int3 ( y ) , int3 ( z ) ) ) ) ; } 
uint4 AMin3SU4 ( uint4 x , uint4 y , uint4 z ) { return uint4 ( min ( int4 ( x ) , min ( int4 ( y ) , int4 ( z ) ) ) ) ; } 

uint AMin3U1 ( uint x , uint y , uint z ) { return min ( x , min ( y , z ) ) ; } 
uint2 AMin3U2 ( uint2 x , uint2 y , uint2 z ) { return min ( x , min ( y , z ) ) ; } 
uint3 AMin3U3 ( uint3 x , uint3 y , uint3 z ) { return min ( x , min ( y , z ) ) ; } 
uint4 AMin3U4 ( uint4 x , uint4 y , uint4 z ) { return min ( x , min ( y , z ) ) ; } 

uint AMinSU1 ( uint a , uint b ) { return uint ( min ( int ( a ) , int ( b ) ) ) ; } 
uint2 AMinSU2 ( uint2 a , uint2 b ) { return uint2 ( min ( int2 ( a ) , int2 ( b ) ) ) ; } 
uint3 AMinSU3 ( uint3 a , uint3 b ) { return uint3 ( min ( int3 ( a ) , int3 ( b ) ) ) ; } 
uint4 AMinSU4 ( uint4 a , uint4 b ) { return uint4 ( min ( int4 ( a ) , int4 ( b ) ) ) ; } 

float ANCosF1 ( float x ) { return cos ( x * AF1_x ( float ( 6.28318530718 ) ) ) ; } 
float2 ANCosF2 ( float2 x ) { return cos ( x * AF2_x ( float ( 6.28318530718 ) ) ) ; } 
float3 ANCosF3 ( float3 x ) { return cos ( x * AF3_x ( float ( 6.28318530718 ) ) ) ; } 
float4 ANCosF4 ( float4 x ) { return cos ( x * AF4_x ( float ( 6.28318530718 ) ) ) ; } 

float ANSinF1 ( float x ) { return sin ( x * AF1_x ( float ( 6.28318530718 ) ) ) ; } 
float2 ANSinF2 ( float2 x ) { return sin ( x * AF2_x ( float ( 6.28318530718 ) ) ) ; } 
float3 ANSinF3 ( float3 x ) { return sin ( x * AF3_x ( float ( 6.28318530718 ) ) ) ; } 
float4 ANSinF4 ( float4 x ) { return sin ( x * AF4_x ( float ( 6.28318530718 ) ) ) ; } 

float ARcpF1 ( float x ) { return rcp ( x ) ; } 
float2 ARcpF2 ( float2 x ) { return rcp ( x ) ; } 
float3 ARcpF3 ( float3 x ) { return rcp ( x ) ; } 
float4 ARcpF4 ( float4 x ) { return rcp ( x ) ; } 

float ARsqF1 ( float x ) { return rsqrt ( x ) ; } 
float2 ARsqF2 ( float2 x ) { return rsqrt ( x ) ; } 
float3 ARsqF3 ( float3 x ) { return rsqrt ( x ) ; } 
float4 ARsqF4 ( float4 x ) { return rsqrt ( x ) ; } 

float ASatF1 ( float x ) { return saturate ( x ) ; } 
float2 ASatF2 ( float2 x ) { return saturate ( x ) ; } 
float3 ASatF3 ( float3 x ) { return saturate ( x ) ; } 
float4 ASatF4 ( float4 x ) { return saturate ( x ) ; } 

uint AShrSU1 ( uint a , uint b ) { return uint ( int ( a ) >> int ( b ) ) ; } 
uint2 AShrSU2 ( uint2 a , uint2 b ) { return uint2 ( int2 ( a ) >> int2 ( b ) ) ; } 
uint3 AShrSU3 ( uint3 a , uint3 b ) { return uint3 ( int3 ( a ) >> int3 ( b ) ) ; } 
uint4 AShrSU4 ( uint4 a , uint4 b ) { return uint4 ( int4 ( a ) >> int4 ( b ) ) ; } 

#line 296


#line 312

















#line 331
min16float2 AH2_AU1_x ( uint x ) { float2 t = f16tof32 ( uint2 ( x & 0xFFFF , x >> 16 ) ) ; return min16float2 ( t ) ; } 
min16float4 AH4_AU2_x ( uint2 x ) { return min16float4 ( AH2_AU1_x ( x . x ) , AH2_AU1_x ( x . y ) ) ; } 
min16uint2 AW2_AU1_x ( uint x ) { uint2 t = uint2 ( x & 0xFFFF , x >> 16 ) ; return min16uint2 ( t ) ; } 
min16uint4 AW4_AU2_x ( uint2 x ) { return min16uint4 ( AW2_AU1_x ( x . x ) , AW2_AU1_x ( x . y ) ) ; } 





uint AU1_AH2_x ( min16float2 x ) { return f32tof16 ( x . x ) + ( f32tof16 ( x . y ) << 16 ) ; } 
uint2 AU2_AH4_x ( min16float4 x ) { return uint2 ( AU1_AH2_x ( x . xy ) , AU1_AH2_x ( x . zw ) ) ; } 
uint AU1_AW2_x ( min16uint2 x ) { return uint ( x . x ) + ( uint ( x . y ) << 16 ) ; } 
uint2 AU2_AW4_x ( min16uint4 x ) { return uint2 ( AU1_AW2_x ( x . xy ) , AU1_AW2_x ( x . zw ) ) ; } 





#line 354







#line 366







min16float AH1_x ( min16float a ) { return min16float ( a ) ; } 
min16float2 AH2_x ( min16float a ) { return min16float2 ( a , a ) ; } 
min16float3 AH3_x ( min16float a ) { return min16float3 ( a , a , a ) ; } 
min16float4 AH4_x ( min16float a ) { return min16float4 ( a , a , a , a ) ; } 





min16uint AW1_x ( min16uint a ) { return min16uint ( a ) ; } 
min16uint2 AW2_x ( min16uint a ) { return min16uint2 ( a , a ) ; } 
min16uint3 AW3_x ( min16uint a ) { return min16uint3 ( a , a , a ) ; } 
min16uint4 AW4_x ( min16uint a ) { return min16uint4 ( a , a , a , a ) ; } 





min16uint AAbsSW1 ( min16uint a ) { return min16uint ( abs ( min16int ( a ) ) ) ; } 
min16uint2 AAbsSW2 ( min16uint2 a ) { return min16uint2 ( abs ( min16int2 ( a ) ) ) ; } 
min16uint3 AAbsSW3 ( min16uint3 a ) { return min16uint3 ( abs ( min16int3 ( a ) ) ) ; } 
min16uint4 AAbsSW4 ( min16uint4 a ) { return min16uint4 ( abs ( min16int4 ( a ) ) ) ; } 

min16float AClampH1 ( min16float x , min16float n , min16float m ) { return max ( n , min ( x , m ) ) ; } 
min16float2 AClampH2 ( min16float2 x , min16float2 n , min16float2 m ) { return max ( n , min ( x , m ) ) ; } 
min16float3 AClampH3 ( min16float3 x , min16float3 n , min16float3 m ) { return max ( n , min ( x , m ) ) ; } 
min16float4 AClampH4 ( min16float4 x , min16float4 n , min16float4 m ) { return max ( n , min ( x , m ) ) ; } 

#line 402
min16float AFractH1 ( min16float x ) { return x - floor ( x ) ; } 
min16float2 AFractH2 ( min16float2 x ) { return x - floor ( x ) ; } 
min16float3 AFractH3 ( min16float3 x ) { return x - floor ( x ) ; } 
min16float4 AFractH4 ( min16float4 x ) { return x - floor ( x ) ; } 

min16float ALerpH1 ( min16float x , min16float y , min16float a ) { return lerp ( x , y , a ) ; } 
min16float2 ALerpH2 ( min16float2 x , min16float2 y , min16float2 a ) { return lerp ( x , y , a ) ; } 
min16float3 ALerpH3 ( min16float3 x , min16float3 y , min16float3 a ) { return lerp ( x , y , a ) ; } 
min16float4 ALerpH4 ( min16float4 x , min16float4 y , min16float4 a ) { return lerp ( x , y , a ) ; } 

min16float AMax3H1 ( min16float x , min16float y , min16float z ) { return max ( x , max ( y , z ) ) ; } 
min16float2 AMax3H2 ( min16float2 x , min16float2 y , min16float2 z ) { return max ( x , max ( y , z ) ) ; } 
min16float3 AMax3H3 ( min16float3 x , min16float3 y , min16float3 z ) { return max ( x , max ( y , z ) ) ; } 
min16float4 AMax3H4 ( min16float4 x , min16float4 y , min16float4 z ) { return max ( x , max ( y , z ) ) ; } 

min16uint AMaxSW1 ( min16uint a , min16uint b ) { return min16uint ( max ( int ( a ) , int ( b ) ) ) ; } 
min16uint2 AMaxSW2 ( min16uint2 a , min16uint2 b ) { return min16uint2 ( max ( int2 ( a ) , int2 ( b ) ) ) ; } 
min16uint3 AMaxSW3 ( min16uint3 a , min16uint3 b ) { return min16uint3 ( max ( int3 ( a ) , int3 ( b ) ) ) ; } 
min16uint4 AMaxSW4 ( min16uint4 a , min16uint4 b ) { return min16uint4 ( max ( int4 ( a ) , int4 ( b ) ) ) ; } 

min16float AMin3H1 ( min16float x , min16float y , min16float z ) { return min ( x , min ( y , z ) ) ; } 
min16float2 AMin3H2 ( min16float2 x , min16float2 y , min16float2 z ) { return min ( x , min ( y , z ) ) ; } 
min16float3 AMin3H3 ( min16float3 x , min16float3 y , min16float3 z ) { return min ( x , min ( y , z ) ) ; } 
min16float4 AMin3H4 ( min16float4 x , min16float4 y , min16float4 z ) { return min ( x , min ( y , z ) ) ; } 

min16uint AMinSW1 ( min16uint a , min16uint b ) { return min16uint ( min ( int ( a ) , int ( b ) ) ) ; } 
min16uint2 AMinSW2 ( min16uint2 a , min16uint2 b ) { return min16uint2 ( min ( int2 ( a ) , int2 ( b ) ) ) ; } 
min16uint3 AMinSW3 ( min16uint3 a , min16uint3 b ) { return min16uint3 ( min ( int3 ( a ) , int3 ( b ) ) ) ; } 
min16uint4 AMinSW4 ( min16uint4 a , min16uint4 b ) { return min16uint4 ( min ( int4 ( a ) , int4 ( b ) ) ) ; } 

min16float ARcpH1 ( min16float x ) { return rcp ( x ) ; } 
min16float2 ARcpH2 ( min16float2 x ) { return rcp ( x ) ; } 
min16float3 ARcpH3 ( min16float3 x ) { return rcp ( x ) ; } 
min16float4 ARcpH4 ( min16float4 x ) { return rcp ( x ) ; } 

min16float ARsqH1 ( min16float x ) { return rsqrt ( x ) ; } 
min16float2 ARsqH2 ( min16float2 x ) { return rsqrt ( x ) ; } 
min16float3 ARsqH3 ( min16float3 x ) { return rsqrt ( x ) ; } 
min16float4 ARsqH4 ( min16float4 x ) { return rsqrt ( x ) ; } 

min16float ASatH1 ( min16float x ) { return saturate ( x ) ; } 
min16float2 ASatH2 ( min16float2 x ) { return saturate ( x ) ; } 
min16float3 ASatH3 ( min16float3 x ) { return saturate ( x ) ; } 
min16float4 ASatH4 ( min16float4 x ) { return saturate ( x ) ; } 

min16uint AShrSW1 ( min16uint a , min16uint b ) { return min16uint ( min16int ( a ) >> min16int ( b ) ) ; } 
min16uint2 AShrSW2 ( min16uint2 a , min16uint2 b ) { return min16uint2 ( min16int2 ( a ) >> min16int2 ( b ) ) ; } 
min16uint3 AShrSW3 ( min16uint3 a , min16uint3 b ) { return min16uint3 ( min16int3 ( a ) >> min16int3 ( b ) ) ; } 
min16uint4 AShrSW4 ( min16uint4 a , min16uint4 b ) { return min16uint4 ( min16int4 ( a ) >> min16int4 ( b ) ) ; } 


#line 467





#line 473
float ACpySgnF1 ( float d , float s ) { return asfloat ( uint ( asuint ( float ( d ) ) | ( asuint ( float ( s ) ) & AU1_x ( uint ( 0x80000000u ) ) ) ) ) ; } 
float2 ACpySgnF2 ( float2 d , float2 s ) { return asfloat ( uint2 ( asuint ( float2 ( d ) ) | ( asuint ( float2 ( s ) ) & AU2_x ( uint ( 0x80000000u ) ) ) ) ) ; } 
float3 ACpySgnF3 ( float3 d , float3 s ) { return asfloat ( uint3 ( asuint ( float3 ( d ) ) | ( asuint ( float3 ( s ) ) & AU3_x ( uint ( 0x80000000u ) ) ) ) ) ; } 
float4 ACpySgnF4 ( float4 d , float4 s ) { return asfloat ( uint4 ( asuint ( float4 ( d ) ) | ( asuint ( float4 ( s ) ) & AU4_x ( uint ( 0x80000000u ) ) ) ) ) ; } 

#line 486
float ASignedF1 ( float m ) { return ASatF1 ( m * AF1_x ( float ( asfloat ( uint ( 0xff800000u ) ) ) ) ) ; } 
float2 ASignedF2 ( float2 m ) { return ASatF2 ( m * AF2_x ( float ( asfloat ( uint ( 0xff800000u ) ) ) ) ) ; } 
float3 ASignedF3 ( float3 m ) { return ASatF3 ( m * AF3_x ( float ( asfloat ( uint ( 0xff800000u ) ) ) ) ) ; } 
float4 ASignedF4 ( float4 m ) { return ASatF4 ( m * AF4_x ( float ( asfloat ( uint ( 0xff800000u ) ) ) ) ) ; } 

float AGtZeroF1 ( float m ) { return ASatF1 ( m * AF1_x ( float ( asfloat ( uint ( 0x7f800000u ) ) ) ) ) ; } 
float2 AGtZeroF2 ( float2 m ) { return ASatF2 ( m * AF2_x ( float ( asfloat ( uint ( 0x7f800000u ) ) ) ) ) ; } 
float3 AGtZeroF3 ( float3 m ) { return ASatF3 ( m * AF3_x ( float ( asfloat ( uint ( 0x7f800000u ) ) ) ) ) ; } 
float4 AGtZeroF4 ( float4 m ) { return ASatF4 ( m * AF4_x ( float ( asfloat ( uint ( 0x7f800000u ) ) ) ) ) ; } 



#line 500





#line 506
min16float ACpySgnH1 ( min16float d , min16float s ) { return min16float ( f16tof32 ( uint ( min16uint ( f32tof16 ( float ( d ) ) ) | ( min16uint ( f32tof16 ( float ( s ) ) ) & AW1_x ( min16uint ( 0x8000u ) ) ) ) ) ) ; } 
min16float2 ACpySgnH2 ( min16float2 d , min16float2 s ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( min16uint2 ( min16uint ( f32tof16 ( float ( ( d ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . y ) ) ) ) | ( min16uint2 ( min16uint ( f32tof16 ( float ( ( s ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . y ) ) ) ) & AW2_x ( min16uint ( 0x8000u ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( min16uint2 ( min16uint ( f32tof16 ( float ( ( d ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . y ) ) ) ) | ( min16uint2 ( min16uint ( f32tof16 ( float ( ( s ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . y ) ) ) ) & AW2_x ( min16uint ( 0x8000u ) ) ) ) . y ) ) ) ) ; } 
min16float3 ACpySgnH3 ( min16float3 d , min16float3 s ) { return min16float3 ( min16float ( f16tof32 ( uint ( ( min16uint3 ( min16uint ( f32tof16 ( float ( ( d ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . z ) ) ) ) | ( min16uint3 ( min16uint ( f32tof16 ( float ( ( s ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . z ) ) ) ) & AW3_x ( min16uint ( 0x8000u ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( min16uint3 ( min16uint ( f32tof16 ( float ( ( d ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . z ) ) ) ) | ( min16uint3 ( min16uint ( f32tof16 ( float ( ( s ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . z ) ) ) ) & AW3_x ( min16uint ( 0x8000u ) ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( min16uint3 ( min16uint ( f32tof16 ( float ( ( d ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . z ) ) ) ) | ( min16uint3 ( min16uint ( f32tof16 ( float ( ( s ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . z ) ) ) ) & AW3_x ( min16uint ( 0x8000u ) ) ) ) . z ) ) ) ) ; } 
min16float4 ACpySgnH4 ( min16float4 d , min16float4 s ) { return min16float4 ( min16float ( f16tof32 ( uint ( ( min16uint4 ( min16uint ( f32tof16 ( float ( ( d ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . w ) ) ) ) | ( min16uint4 ( min16uint ( f32tof16 ( float ( ( s ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . w ) ) ) ) & AW4_x ( min16uint ( 0x8000u ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( min16uint4 ( min16uint ( f32tof16 ( float ( ( d ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . w ) ) ) ) | ( min16uint4 ( min16uint ( f32tof16 ( float ( ( s ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . w ) ) ) ) & AW4_x ( min16uint ( 0x8000u ) ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( min16uint4 ( min16uint ( f32tof16 ( float ( ( d ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . w ) ) ) ) | ( min16uint4 ( min16uint ( f32tof16 ( float ( ( s ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . w ) ) ) ) & AW4_x ( min16uint ( 0x8000u ) ) ) ) . z ) ) ) , min16float ( f16tof32 ( uint ( ( min16uint4 ( min16uint ( f32tof16 ( float ( ( d ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( d ) . w ) ) ) ) | ( min16uint4 ( min16uint ( f32tof16 ( float ( ( s ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( s ) . w ) ) ) ) & AW4_x ( min16uint ( 0x8000u ) ) ) ) . w ) ) ) ) ; } 

min16float ASignedH1 ( min16float m ) { return ASatH1 ( m * AH1_x ( min16float ( min16float ( f16tof32 ( uint ( 0xfc00u ) ) ) ) ) ) ; } 
min16float2 ASignedH2 ( min16float2 m ) { return ASatH2 ( m * AH2_x ( min16float ( min16float ( f16tof32 ( uint ( 0xfc00u ) ) ) ) ) ) ; } 
min16float3 ASignedH3 ( min16float3 m ) { return ASatH3 ( m * AH3_x ( min16float ( min16float ( f16tof32 ( uint ( 0xfc00u ) ) ) ) ) ) ; } 
min16float4 ASignedH4 ( min16float4 m ) { return ASatH4 ( m * AH4_x ( min16float ( min16float ( f16tof32 ( uint ( 0xfc00u ) ) ) ) ) ) ; } 

min16float AGtZeroH1 ( min16float m ) { return ASatH1 ( m * AH1_x ( min16float ( min16float ( f16tof32 ( uint ( 0x7c00u ) ) ) ) ) ) ; } 
min16float2 AGtZeroH2 ( min16float2 m ) { return ASatH2 ( m * AH2_x ( min16float ( min16float ( f16tof32 ( uint ( 0x7c00u ) ) ) ) ) ) ; } 
min16float3 AGtZeroH3 ( min16float3 m ) { return ASatH3 ( m * AH3_x ( min16float ( min16float ( f16tof32 ( uint ( 0x7c00u ) ) ) ) ) ) ; } 
min16float4 AGtZeroH4 ( min16float4 m ) { return ASatH4 ( m * AH4_x ( min16float ( min16float ( f16tof32 ( uint ( 0x7c00u ) ) ) ) ) ) ; } 


#line 538
uint AFisToU1 ( uint x ) { return x ^ ( ( AShrSU1 ( x , AU1_x ( uint ( 31 ) ) ) ) | AU1_x ( uint ( 0x80000000 ) ) ) ; } 
uint AFisFromU1 ( uint x ) { return x ^ ( ( ~ AShrSU1 ( x , AU1_x ( uint ( 31 ) ) ) ) | AU1_x ( uint ( 0x80000000 ) ) ) ; } 

#line 542
uint AFisToHiU1 ( uint x ) { return x ^ ( ( AShrSU1 ( x , AU1_x ( uint ( 15 ) ) ) ) | AU1_x ( uint ( 0x80000000 ) ) ) ; } 
uint AFisFromHiU1 ( uint x ) { return x ^ ( ( ~ AShrSU1 ( x , AU1_x ( uint ( 15 ) ) ) ) | AU1_x ( uint ( 0x80000000 ) ) ) ; } 


min16uint AFisToW1 ( min16uint x ) { return x ^ ( ( AShrSW1 ( x , AW1_x ( min16uint ( 15 ) ) ) ) | AW1_x ( min16uint ( 0x8000 ) ) ) ; } 
min16uint AFisFromW1 ( min16uint x ) { return x ^ ( ( ~ AShrSW1 ( x , AW1_x ( min16uint ( 15 ) ) ) ) | AW1_x ( min16uint ( 0x8000 ) ) ) ; } 

min16uint2 AFisToW2 ( min16uint2 x ) { return x ^ ( ( AShrSW2 ( x , AW2_x ( min16uint ( 15 ) ) ) ) | AW2_x ( min16uint ( 0x8000 ) ) ) ; } 
min16uint2 AFisFromW2 ( min16uint2 x ) { return x ^ ( ( ~ AShrSW2 ( x , AW2_x ( min16uint ( 15 ) ) ) ) | AW2_x ( min16uint ( 0x8000 ) ) ) ; } 


#line 569

uint APerm0E0A ( uint2 i ) { return ( ( i . x ) & 0xffu ) | ( ( i . y << 16 ) & 0xff0000u ) ; } 
uint APerm0F0B ( uint2 i ) { return ( ( i . x >> 8 ) & 0xffu ) | ( ( i . y << 8 ) & 0xff0000u ) ; } 
uint APerm0G0C ( uint2 i ) { return ( ( i . x >> 16 ) & 0xffu ) | ( ( i . y ) & 0xff0000u ) ; } 
uint APerm0H0D ( uint2 i ) { return ( ( i . x >> 24 ) & 0xffu ) | ( ( i . y >> 8 ) & 0xff0000u ) ; } 

uint APermHGFA ( uint2 i ) { return ( ( i . x ) & 0x000000ffu ) | ( i . y & 0xffffff00u ) ; } 
uint APermHGFC ( uint2 i ) { return ( ( i . x >> 16 ) & 0x000000ffu ) | ( i . y & 0xffffff00u ) ; } 
uint APermHGAE ( uint2 i ) { return ( ( i . x << 8 ) & 0x0000ff00u ) | ( i . y & 0xffff00ffu ) ; } 
uint APermHGCE ( uint2 i ) { return ( ( i . x >> 8 ) & 0x0000ff00u ) | ( i . y & 0xffff00ffu ) ; } 
uint APermHAFE ( uint2 i ) { return ( ( i . x << 16 ) & 0x00ff0000u ) | ( i . y & 0xff00ffffu ) ; } 
uint APermHCFE ( uint2 i ) { return ( ( i . x ) & 0x00ff0000u ) | ( i . y & 0xff00ffffu ) ; } 
uint APermAGFE ( uint2 i ) { return ( ( i . x << 24 ) & 0xff000000u ) | ( i . y & 0x00ffffffu ) ; } 
uint APermCGFE ( uint2 i ) { return ( ( i . x << 8 ) & 0xff000000u ) | ( i . y & 0x00ffffffu ) ; } 

uint APermGCEA ( uint2 i ) { return ( ( i . x ) & 0x00ff00ffu ) | ( ( i . y << 8 ) & 0xff00ff00u ) ; } 
uint APermGECA ( uint2 i ) { return ( ( ( i . x ) & 0xffu ) | ( ( i . x >> 8 ) & 0xff00u ) | ( ( i . y << 16 ) & 0xff0000u ) | ( ( i . y << 8 ) & 0xff000000u ) ) ; } 


#line 646





#line 652
uint ABuc0ToU1 ( uint d , float i ) { return ( d & 0xffffff00u ) | ( ( min ( uint ( i ) , 255u ) ) & ( 0x000000ffu ) ) ; } 
uint ABuc1ToU1 ( uint d , float i ) { return ( d & 0xffff00ffu ) | ( ( min ( uint ( i ) , 255u ) << 8 ) & ( 0x0000ff00u ) ) ; } 
uint ABuc2ToU1 ( uint d , float i ) { return ( d & 0xff00ffffu ) | ( ( min ( uint ( i ) , 255u ) << 16 ) & ( 0x00ff0000u ) ) ; } 
uint ABuc3ToU1 ( uint d , float i ) { return ( d & 0x00ffffffu ) | ( ( min ( uint ( i ) , 255u ) << 24 ) & ( 0xff000000u ) ) ; } 

#line 658
float ABuc0FromU1 ( uint i ) { return float ( ( i ) & 255u ) ; } 
float ABuc1FromU1 ( uint i ) { return float ( ( i >> 8 ) & 255u ) ; } 
float ABuc2FromU1 ( uint i ) { return float ( ( i >> 16 ) & 255u ) ; } 
float ABuc3FromU1 ( uint i ) { return float ( ( i >> 24 ) & 255u ) ; } 




min16uint2 ABuc01ToW2 ( min16float2 x , min16float2 y ) { 
    x *= AH2_x ( min16float ( 1.0 / 32768.0 ) ) ; y *= AH2_x ( min16float ( 1.0 / 32768.0 ) ) ; 
    return AW2_AU1_x ( uint ( APermGCEA ( uint2 ( AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( x ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( x ) . y ) ) ) ) ) ) , AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( y ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( y ) . y ) ) ) ) ) ) ) ) ) ) ; 
} 

#line 672
uint2 ABuc0ToU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) ) . y ) ) ) ) ) ) ; 
    return uint2 ( APermHGFA ( uint2 ( d . x , b ) ) , APermHGFC ( uint2 ( d . y , b ) ) ) ; 
} 
uint2 ABuc1ToU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) ) . y ) ) ) ) ) ) ; 
    return uint2 ( APermHGAE ( uint2 ( d . x , b ) ) , APermHGCE ( uint2 ( d . y , b ) ) ) ; 
} 
uint2 ABuc2ToU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) ) . y ) ) ) ) ) ) ; 
    return uint2 ( APermHAFE ( uint2 ( d . x , b ) ) , APermHCFE ( uint2 ( d . y , b ) ) ) ; 
} 
uint2 ABuc3ToU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) ) . y ) ) ) ) ) ) ; 
    return uint2 ( APermAGFE ( uint2 ( d . x , b ) ) , APermCGFE ( uint2 ( d . y , b ) ) ) ; 
} 

#line 690
min16float2 ABuc0FromU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0E0A ( i ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0E0A ( i ) ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) ; } 
min16float2 ABuc1FromU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0F0B ( i ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0F0B ( i ) ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) ; } 
min16float2 ABuc2FromU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0G0C ( i ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0G0C ( i ) ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) ; } 
min16float2 ABuc3FromU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0H0D ( i ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0H0D ( i ) ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) ; } 


#line 726




uint ABsc0ToU1 ( uint d , float i ) { return ( d & 0xffffff00u ) | ( ( min ( uint ( i + 128.0 ) , 255u ) ) & ( 0x000000ffu ) ) ; } 
uint ABsc1ToU1 ( uint d , float i ) { return ( d & 0xffff00ffu ) | ( ( min ( uint ( i + 128.0 ) , 255u ) << 8 ) & ( 0x0000ff00u ) ) ; } 
uint ABsc2ToU1 ( uint d , float i ) { return ( d & 0xff00ffffu ) | ( ( min ( uint ( i + 128.0 ) , 255u ) << 16 ) & ( 0x00ff0000u ) ) ; } 
uint ABsc3ToU1 ( uint d , float i ) { return ( d & 0x00ffffffu ) | ( ( min ( uint ( i + 128.0 ) , 255u ) << 24 ) & ( 0xff000000u ) ) ; } 

uint ABsc0ToZbU1 ( uint d , float i ) { return ( ( d & 0xffffff00u ) | ( ( min ( uint ( trunc ( i ) + 128.0 ) , 255u ) ) & ( 0x000000ffu ) ) ) ^ 0x00000080u ; } 
uint ABsc1ToZbU1 ( uint d , float i ) { return ( ( d & 0xffff00ffu ) | ( ( min ( uint ( trunc ( i ) + 128.0 ) , 255u ) << 8 ) & ( 0x0000ff00u ) ) ) ^ 0x00008000u ; } 
uint ABsc2ToZbU1 ( uint d , float i ) { return ( ( d & 0xff00ffffu ) | ( ( min ( uint ( trunc ( i ) + 128.0 ) , 255u ) << 16 ) & ( 0x00ff0000u ) ) ) ^ 0x00800000u ; } 
uint ABsc3ToZbU1 ( uint d , float i ) { return ( ( d & 0x00ffffffu ) | ( ( min ( uint ( trunc ( i ) + 128.0 ) , 255u ) << 24 ) & ( 0xff000000u ) ) ) ^ 0x80000000u ; } 

float ABsc0FromU1 ( uint i ) { return float ( ( i ) & 255u ) - 128.0 ; } 
float ABsc1FromU1 ( uint i ) { return float ( ( i >> 8 ) & 255u ) - 128.0 ; } 
float ABsc2FromU1 ( uint i ) { return float ( ( i >> 16 ) & 255u ) - 128.0 ; } 
float ABsc3FromU1 ( uint i ) { return float ( ( i >> 24 ) & 255u ) - 128.0 ; } 

float ABsc0FromZbU1 ( uint i ) { return float ( ( ( i ) & 255u ) ^ 0x80u ) - 128.0 ; } 
float ABsc1FromZbU1 ( uint i ) { return float ( ( ( i >> 8 ) & 255u ) ^ 0x80u ) - 128.0 ; } 
float ABsc2FromZbU1 ( uint i ) { return float ( ( ( i >> 16 ) & 255u ) ^ 0x80u ) - 128.0 ; } 
float ABsc3FromZbU1 ( uint i ) { return float ( ( ( i >> 24 ) & 255u ) ^ 0x80u ) - 128.0 ; } 




min16uint2 ABsc01ToW2 ( min16float2 x , min16float2 y ) { 
    x = x * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ; y = y * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ; 
    return AW2_AU1_x ( uint ( APermGCEA ( uint2 ( AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( x ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( x ) . y ) ) ) ) ) ) , AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( y ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( y ) . y ) ) ) ) ) ) ) ) ) ) ; 
} 

uint2 ABsc0ToU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . y ) ) ) ) ) ) ; 
    return uint2 ( APermHGFA ( uint2 ( d . x , b ) ) , APermHGFC ( uint2 ( d . y , b ) ) ) ; 
} 
uint2 ABsc1ToU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . y ) ) ) ) ) ) ; 
    return uint2 ( APermHGAE ( uint2 ( d . x , b ) ) , APermHGCE ( uint2 ( d . y , b ) ) ) ; 
} 
uint2 ABsc2ToU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . y ) ) ) ) ) ) ; 
    return uint2 ( APermHAFE ( uint2 ( d . x , b ) ) , APermHCFE ( uint2 ( d . y , b ) ) ) ; 
} 
uint2 ABsc3ToU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . y ) ) ) ) ) ) ; 
    return uint2 ( APermAGFE ( uint2 ( d . x , b ) ) , APermCGFE ( uint2 ( d . y , b ) ) ) ; 
} 

uint2 ABsc0ToZbU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . y ) ) ) ) ) ) ^ 0x00800080u ; 
    return uint2 ( APermHGFA ( uint2 ( d . x , b ) ) , APermHGFC ( uint2 ( d . y , b ) ) ) ; 
} 
uint2 ABsc1ToZbU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . y ) ) ) ) ) ) ^ 0x00800080u ; 
    return uint2 ( APermHGAE ( uint2 ( d . x , b ) ) , APermHGCE ( uint2 ( d . y , b ) ) ) ; 
} 
uint2 ABsc2ToZbU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . y ) ) ) ) ) ) ^ 0x00800080u ; 
    return uint2 ( APermHAFE ( uint2 ( d . x , b ) ) , APermHCFE ( uint2 ( d . y , b ) ) ) ; 
} 
uint2 ABsc3ToZbU2 ( uint2 d , min16float2 i ) { 
    uint b = AU1_AW2_x ( min16uint2 ( min16uint2 ( min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( i * AH2_x ( min16float ( 1.0 / 32768.0 ) ) + AH2_x ( min16float ( 0.25 / 32768.0 ) ) ) . y ) ) ) ) ) ) ^ 0x00800080u ; 
    return uint2 ( APermAGFE ( uint2 ( d . x , b ) ) , APermCGFE ( uint2 ( d . y , b ) ) ) ; 
} 

min16float2 ABsc0FromU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0E0A ( i ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0E0A ( i ) ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) - AH2_x ( min16float ( 0.25 ) ) ; } 
min16float2 ABsc1FromU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0F0B ( i ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0F0B ( i ) ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) - AH2_x ( min16float ( 0.25 ) ) ; } 
min16float2 ABsc2FromU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0G0C ( i ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0G0C ( i ) ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) - AH2_x ( min16float ( 0.25 ) ) ; } 
min16float2 ABsc3FromU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0H0D ( i ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0H0D ( i ) ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) - AH2_x ( min16float ( 0.25 ) ) ; } 

min16float2 ABsc0FromZbU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0E0A ( i ) ^ 0x00800080u ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0E0A ( i ) ^ 0x00800080u ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) - AH2_x ( min16float ( 0.25 ) ) ; } 
min16float2 ABsc1FromZbU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0F0B ( i ) ^ 0x00800080u ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0F0B ( i ) ^ 0x00800080u ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) - AH2_x ( min16float ( 0.25 ) ) ; } 
min16float2 ABsc2FromZbU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0G0C ( i ) ^ 0x00800080u ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0G0C ( i ) ^ 0x00800080u ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) - AH2_x ( min16float ( 0.25 ) ) ; } 
min16float2 ABsc3FromZbU2 ( uint2 i ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0H0D ( i ) ^ 0x00800080u ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_AU1_x ( uint ( APerm0H0D ( i ) ^ 0x00800080u ) ) ) . y ) ) ) ) * AH2_x ( min16float ( 32768.0 ) ) - AH2_x ( min16float ( 0.25 ) ) ; } 


#line 818


#line 821
min16float APrxLoSqrtH1 ( min16float a ) { return min16float ( f16tof32 ( uint ( ( min16uint ( f32tof16 ( float ( a ) ) ) >> AW1_x ( min16uint ( 1 ) ) ) + AW1_x ( min16uint ( 0x1de2 ) ) ) ) ) ; } 
min16float2 APrxLoSqrtH2 ( min16float2 a ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( ( min16uint2 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) ) >> AW2_x ( min16uint ( 1 ) ) ) + AW2_x ( min16uint ( 0x1de2 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( ( min16uint2 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) ) >> AW2_x ( min16uint ( 1 ) ) ) + AW2_x ( min16uint ( 0x1de2 ) ) ) . y ) ) ) ) ; } 
min16float3 APrxLoSqrtH3 ( min16float3 a ) { return min16float3 ( min16float ( f16tof32 ( uint ( ( ( min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) >> AW3_x ( min16uint ( 1 ) ) ) + AW3_x ( min16uint ( 0x1de2 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( ( min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) >> AW3_x ( min16uint ( 1 ) ) ) + AW3_x ( min16uint ( 0x1de2 ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( ( min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) >> AW3_x ( min16uint ( 1 ) ) ) + AW3_x ( min16uint ( 0x1de2 ) ) ) . z ) ) ) ) ; } 
min16float4 APrxLoSqrtH4 ( min16float4 a ) { return min16float4 ( min16float ( f16tof32 ( uint ( ( ( min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) >> AW4_x ( min16uint ( 1 ) ) ) + AW4_x ( min16uint ( 0x1de2 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( ( min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) >> AW4_x ( min16uint ( 1 ) ) ) + AW4_x ( min16uint ( 0x1de2 ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( ( min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) >> AW4_x ( min16uint ( 1 ) ) ) + AW4_x ( min16uint ( 0x1de2 ) ) ) . z ) ) ) , min16float ( f16tof32 ( uint ( ( ( min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) >> AW4_x ( min16uint ( 1 ) ) ) + AW4_x ( min16uint ( 0x1de2 ) ) ) . w ) ) ) ) ; } 

#line 828
min16float APrxLoRcpH1 ( min16float a ) { return min16float ( f16tof32 ( uint ( AW1_x ( min16uint ( 0x7784 ) ) - min16uint ( f32tof16 ( float ( a ) ) ) ) ) ) ; } 
min16float2 APrxLoRcpH2 ( min16float2 a ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_x ( min16uint ( 0x7784 ) ) - min16uint2 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_x ( min16uint ( 0x7784 ) ) - min16uint2 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) ) ) . y ) ) ) ) ; } 
min16float3 APrxLoRcpH3 ( min16float3 a ) { return min16float3 ( min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 0x7784 ) ) - min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 0x7784 ) ) - min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 0x7784 ) ) - min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) ) . z ) ) ) ) ; } 
min16float4 APrxLoRcpH4 ( min16float4 a ) { return min16float4 ( min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x7784 ) ) - min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x7784 ) ) - min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x7784 ) ) - min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) ) . z ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x7784 ) ) - min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) ) . w ) ) ) ) ; } 

#line 834
min16float APrxMedRcpH1 ( min16float a ) { min16float b = min16float ( f16tof32 ( uint ( AW1_x ( min16uint ( 0x778d ) ) - min16uint ( f32tof16 ( float ( a ) ) ) ) ) ) ; return b * ( - b * a + AH1_x ( min16float ( 2.0 ) ) ) ; } 
min16float2 APrxMedRcpH2 ( min16float2 a ) { min16float2 b = min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_x ( min16uint ( 0x778d ) ) - min16uint2 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_x ( min16uint ( 0x778d ) ) - min16uint2 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) ) ) . y ) ) ) ) ; return b * ( - b * a + AH2_x ( min16float ( 2.0 ) ) ) ; } 
min16float3 APrxMedRcpH3 ( min16float3 a ) { min16float3 b = min16float3 ( min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 0x778d ) ) - min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 0x778d ) ) - min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 0x778d ) ) - min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) ) . z ) ) ) ) ; return b * ( - b * a + AH3_x ( min16float ( 2.0 ) ) ) ; } 
min16float4 APrxMedRcpH4 ( min16float4 a ) { min16float4 b = min16float4 ( min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x778d ) ) - min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x778d ) ) - min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x778d ) ) - min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) ) . z ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x778d ) ) - min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) ) . w ) ) ) ) ; return b * ( - b * a + AH4_x ( min16float ( 2.0 ) ) ) ; } 

#line 840
min16float APrxLoRsqH1 ( min16float a ) { return min16float ( f16tof32 ( uint ( AW1_x ( min16uint ( 0x59a3 ) ) - ( min16uint ( f32tof16 ( float ( a ) ) ) >> AW1_x ( min16uint ( 1 ) ) ) ) ) ) ; } 
min16float2 APrxLoRsqH2 ( min16float2 a ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_x ( min16uint ( 0x59a3 ) ) - ( min16uint2 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) ) >> AW2_x ( min16uint ( 1 ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_x ( min16uint ( 0x59a3 ) ) - ( min16uint2 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) ) >> AW2_x ( min16uint ( 1 ) ) ) ) . y ) ) ) ) ; } 
min16float3 APrxLoRsqH3 ( min16float3 a ) { return min16float3 ( min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 0x59a3 ) ) - ( min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) >> AW3_x ( min16uint ( 1 ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 0x59a3 ) ) - ( min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) >> AW3_x ( min16uint ( 1 ) ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 0x59a3 ) ) - ( min16uint3 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) ) >> AW3_x ( min16uint ( 1 ) ) ) ) . z ) ) ) ) ; } 
min16float4 APrxLoRsqH4 ( min16float4 a ) { return min16float4 ( min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x59a3 ) ) - ( min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) >> AW4_x ( min16uint ( 1 ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x59a3 ) ) - ( min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) >> AW4_x ( min16uint ( 1 ) ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x59a3 ) ) - ( min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) >> AW4_x ( min16uint ( 1 ) ) ) ) . z ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 0x59a3 ) ) - ( min16uint4 ( min16uint ( f32tof16 ( float ( ( a ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( a ) . w ) ) ) ) >> AW4_x ( min16uint ( 1 ) ) ) ) . w ) ) ) ) ; } 


#line 862
float APrxLoSqrtF1 ( float a ) { return asfloat ( uint ( ( asuint ( float ( a ) ) >> AU1_x ( uint ( 1 ) ) ) + AU1_x ( uint ( 0x1fbc4639 ) ) ) ) ; } 
float APrxLoRcpF1 ( float a ) { return asfloat ( uint ( AU1_x ( uint ( 0x7ef07ebb ) ) - asuint ( float ( a ) ) ) ) ; } 
float APrxMedRcpF1 ( float a ) { float b = asfloat ( uint ( AU1_x ( uint ( 0x7ef19fff ) ) - asuint ( float ( a ) ) ) ) ; return b * ( - b * a + AF1_x ( float ( 2.0 ) ) ) ; } 
float APrxLoRsqF1 ( float a ) { return asfloat ( uint ( AU1_x ( uint ( 0x5f347d74 ) ) - ( asuint ( float ( a ) ) >> AU1_x ( uint ( 1 ) ) ) ) ) ; } 

float2 APrxLoSqrtF2 ( float2 a ) { return asfloat ( uint2 ( ( asuint ( float2 ( a ) ) >> AU2_x ( uint ( 1 ) ) ) + AU2_x ( uint ( 0x1fbc4639 ) ) ) ) ; } 
float2 APrxLoRcpF2 ( float2 a ) { return asfloat ( uint2 ( AU2_x ( uint ( 0x7ef07ebb ) ) - asuint ( float2 ( a ) ) ) ) ; } 
float2 APrxMedRcpF2 ( float2 a ) { float2 b = asfloat ( uint2 ( AU2_x ( uint ( 0x7ef19fff ) ) - asuint ( float2 ( a ) ) ) ) ; return b * ( - b * a + AF2_x ( float ( 2.0 ) ) ) ; } 
float2 APrxLoRsqF2 ( float2 a ) { return asfloat ( uint2 ( AU2_x ( uint ( 0x5f347d74 ) ) - ( asuint ( float2 ( a ) ) >> AU2_x ( uint ( 1 ) ) ) ) ) ; } 

float3 APrxLoSqrtF3 ( float3 a ) { return asfloat ( uint3 ( ( asuint ( float3 ( a ) ) >> AU3_x ( uint ( 1 ) ) ) + AU3_x ( uint ( 0x1fbc4639 ) ) ) ) ; } 
float3 APrxLoRcpF3 ( float3 a ) { return asfloat ( uint3 ( AU3_x ( uint ( 0x7ef07ebb ) ) - asuint ( float3 ( a ) ) ) ) ; } 
float3 APrxMedRcpF3 ( float3 a ) { float3 b = asfloat ( uint3 ( AU3_x ( uint ( 0x7ef19fff ) ) - asuint ( float3 ( a ) ) ) ) ; return b * ( - b * a + AF3_x ( float ( 2.0 ) ) ) ; } 
float3 APrxLoRsqF3 ( float3 a ) { return asfloat ( uint3 ( AU3_x ( uint ( 0x5f347d74 ) ) - ( asuint ( float3 ( a ) ) >> AU3_x ( uint ( 1 ) ) ) ) ) ; } 

float4 APrxLoSqrtF4 ( float4 a ) { return asfloat ( uint4 ( ( asuint ( float4 ( a ) ) >> AU4_x ( uint ( 1 ) ) ) + AU4_x ( uint ( 0x1fbc4639 ) ) ) ) ; } 
float4 APrxLoRcpF4 ( float4 a ) { return asfloat ( uint4 ( AU4_x ( uint ( 0x7ef07ebb ) ) - asuint ( float4 ( a ) ) ) ) ; } 
float4 APrxMedRcpF4 ( float4 a ) { float4 b = asfloat ( uint4 ( AU4_x ( uint ( 0x7ef19fff ) ) - asuint ( float4 ( a ) ) ) ) ; return b * ( - b * a + AF4_x ( float ( 2.0 ) ) ) ; } 
float4 APrxLoRsqF4 ( float4 a ) { return asfloat ( uint4 ( AU4_x ( uint ( 0x5f347d74 ) ) - ( asuint ( float4 ( a ) ) >> AU4_x ( uint ( 1 ) ) ) ) ) ; } 

#line 891
float Quart ( float a ) { a = a * a ; return a * a ; } 
float Oct ( float a ) { a = a * a ; a = a * a ; return a * a ; } 
float2 Quart ( float2 a ) { a = a * a ; return a * a ; } 
float2 Oct ( float2 a ) { a = a * a ; a = a * a ; return a * a ; } 
float3 Quart ( float3 a ) { a = a * a ; return a * a ; } 
float3 Oct ( float3 a ) { a = a * a ; a = a * a ; return a * a ; } 
float4 Quart ( float4 a ) { a = a * a ; return a * a ; } 
float4 Oct ( float4 a ) { a = a * a ; a = a * a ; return a * a ; } 

float APrxPQToGamma2 ( float a ) { return Quart ( a ) ; } 
float APrxPQToLinear ( float a ) { return Oct ( a ) ; } 
float APrxLoGamma2ToPQ ( float a ) { return asfloat ( uint ( ( asuint ( float ( a ) ) >> AU1_x ( uint ( 2 ) ) ) + AU1_x ( uint ( 0x2F9A4E46 ) ) ) ) ; } 
float APrxMedGamma2ToPQ ( float a ) { float b = asfloat ( uint ( ( asuint ( float ( a ) ) >> AU1_x ( uint ( 2 ) ) ) + AU1_x ( uint ( 0x2F9A4E46 ) ) ) ) ; float b4 = Quart ( b ) ; return b - b * ( b4 - a ) / ( AF1_x ( float ( 4.0 ) ) * b4 ) ; } 
float APrxHighGamma2ToPQ ( float a ) { return sqrt ( sqrt ( a ) ) ; } 
float APrxLoLinearToPQ ( float a ) { return asfloat ( uint ( ( asuint ( float ( a ) ) >> AU1_x ( uint ( 3 ) ) ) + AU1_x ( uint ( 0x378D8723 ) ) ) ) ; } 
float APrxMedLinearToPQ ( float a ) { float b = asfloat ( uint ( ( asuint ( float ( a ) ) >> AU1_x ( uint ( 3 ) ) ) + AU1_x ( uint ( 0x378D8723 ) ) ) ) ; float b8 = Oct ( b ) ; return b - b * ( b8 - a ) / ( AF1_x ( float ( 8.0 ) ) * b8 ) ; } 
float APrxHighLinearToPQ ( float a ) { return sqrt ( sqrt ( sqrt ( a ) ) ) ; } 

float2 APrxPQToGamma2 ( float2 a ) { return Quart ( a ) ; } 
float2 APrxPQToLinear ( float2 a ) { return Oct ( a ) ; } 
float2 APrxLoGamma2ToPQ ( float2 a ) { return asfloat ( uint2 ( ( asuint ( float2 ( a ) ) >> AU2_x ( uint ( 2 ) ) ) + AU2_x ( uint ( 0x2F9A4E46 ) ) ) ) ; } 
float2 APrxMedGamma2ToPQ ( float2 a ) { float2 b = asfloat ( uint2 ( ( asuint ( float2 ( a ) ) >> AU2_x ( uint ( 2 ) ) ) + AU2_x ( uint ( 0x2F9A4E46 ) ) ) ) ; float2 b4 = Quart ( b ) ; return b - b * ( b4 - a ) / ( AF1_x ( float ( 4.0 ) ) * b4 ) ; } 
float2 APrxHighGamma2ToPQ ( float2 a ) { return sqrt ( sqrt ( a ) ) ; } 
float2 APrxLoLinearToPQ ( float2 a ) { return asfloat ( uint2 ( ( asuint ( float2 ( a ) ) >> AU2_x ( uint ( 3 ) ) ) + AU2_x ( uint ( 0x378D8723 ) ) ) ) ; } 
float2 APrxMedLinearToPQ ( float2 a ) { float2 b = asfloat ( uint2 ( ( asuint ( float2 ( a ) ) >> AU2_x ( uint ( 3 ) ) ) + AU2_x ( uint ( 0x378D8723 ) ) ) ) ; float2 b8 = Oct ( b ) ; return b - b * ( b8 - a ) / ( AF1_x ( float ( 8.0 ) ) * b8 ) ; } 
float2 APrxHighLinearToPQ ( float2 a ) { return sqrt ( sqrt ( sqrt ( a ) ) ) ; } 

float3 APrxPQToGamma2 ( float3 a ) { return Quart ( a ) ; } 
float3 APrxPQToLinear ( float3 a ) { return Oct ( a ) ; } 
float3 APrxLoGamma2ToPQ ( float3 a ) { return asfloat ( uint3 ( ( asuint ( float3 ( a ) ) >> AU3_x ( uint ( 2 ) ) ) + AU3_x ( uint ( 0x2F9A4E46 ) ) ) ) ; } 
float3 APrxMedGamma2ToPQ ( float3 a ) { float3 b = asfloat ( uint3 ( ( asuint ( float3 ( a ) ) >> AU3_x ( uint ( 2 ) ) ) + AU3_x ( uint ( 0x2F9A4E46 ) ) ) ) ; float3 b4 = Quart ( b ) ; return b - b * ( b4 - a ) / ( AF1_x ( float ( 4.0 ) ) * b4 ) ; } 
float3 APrxHighGamma2ToPQ ( float3 a ) { return sqrt ( sqrt ( a ) ) ; } 
float3 APrxLoLinearToPQ ( float3 a ) { return asfloat ( uint3 ( ( asuint ( float3 ( a ) ) >> AU3_x ( uint ( 3 ) ) ) + AU3_x ( uint ( 0x378D8723 ) ) ) ) ; } 
float3 APrxMedLinearToPQ ( float3 a ) { float3 b = asfloat ( uint3 ( ( asuint ( float3 ( a ) ) >> AU3_x ( uint ( 3 ) ) ) + AU3_x ( uint ( 0x378D8723 ) ) ) ) ; float3 b8 = Oct ( b ) ; return b - b * ( b8 - a ) / ( AF1_x ( float ( 8.0 ) ) * b8 ) ; } 
float3 APrxHighLinearToPQ ( float3 a ) { return sqrt ( sqrt ( sqrt ( a ) ) ) ; } 

float4 APrxPQToGamma2 ( float4 a ) { return Quart ( a ) ; } 
float4 APrxPQToLinear ( float4 a ) { return Oct ( a ) ; } 
float4 APrxLoGamma2ToPQ ( float4 a ) { return asfloat ( uint4 ( ( asuint ( float4 ( a ) ) >> AU4_x ( uint ( 2 ) ) ) + AU4_x ( uint ( 0x2F9A4E46 ) ) ) ) ; } 
float4 APrxMedGamma2ToPQ ( float4 a ) { float4 b = asfloat ( uint4 ( ( asuint ( float4 ( a ) ) >> AU4_x ( uint ( 2 ) ) ) + AU4_x ( uint ( 0x2F9A4E46 ) ) ) ) ; float4 b4 = Quart ( b ) ; return b - b * ( b4 - a ) / ( AF1_x ( float ( 4.0 ) ) * b4 ) ; } 
float4 APrxHighGamma2ToPQ ( float4 a ) { return sqrt ( sqrt ( a ) ) ; } 
float4 APrxLoLinearToPQ ( float4 a ) { return asfloat ( uint4 ( ( asuint ( float4 ( a ) ) >> AU4_x ( uint ( 3 ) ) ) + AU4_x ( uint ( 0x378D8723 ) ) ) ) ; } 
float4 APrxMedLinearToPQ ( float4 a ) { float4 b = asfloat ( uint4 ( ( asuint ( float4 ( a ) ) >> AU4_x ( uint ( 3 ) ) ) + AU4_x ( uint ( 0x378D8723 ) ) ) ) ; float4 b8 = Oct ( b ) ; return b - b * ( b8 - a ) / ( AF1_x ( float ( 8.0 ) ) * b8 ) ; } 
float4 APrxHighLinearToPQ ( float4 a ) { return sqrt ( sqrt ( sqrt ( a ) ) ) ; } 

#line 944


#line 947
float APSinF1 ( float x ) { return x * abs ( x ) - x ; } 
float2 APSinF2 ( float2 x ) { return x * abs ( x ) - x ; } 
float APCosF1 ( float x ) { x = AFractF1 ( x * AF1_x ( float ( 0.5 ) ) + AF1_x ( float ( 0.75 ) ) ) ; x = x * AF1_x ( float ( 2.0 ) ) - AF1_x ( float ( 1.0 ) ) ; return APSinF1 ( x ) ; } 
float2 APCosF2 ( float2 x ) { x = AFractF2 ( x * AF2_x ( float ( 0.5 ) ) + AF2_x ( float ( 0.75 ) ) ) ; x = x * AF2_x ( float ( 2.0 ) ) - AF2_x ( float ( 1.0 ) ) ; return APSinF2 ( x ) ; } 
float2 APSinCosF1 ( float x ) { float y = AFractF1 ( x * AF1_x ( float ( 0.5 ) ) + AF1_x ( float ( 0.75 ) ) ) ; y = y * AF1_x ( float ( 2.0 ) ) - AF1_x ( float ( 1.0 ) ) ; return APSinF2 ( float2 ( x , y ) ) ; } 




#line 958
min16float APSinH1 ( min16float x ) { return x * abs ( x ) - x ; } 
min16float2 APSinH2 ( min16float2 x ) { return x * abs ( x ) - x ; } 
min16float APCosH1 ( min16float x ) { x = AFractH1 ( x * AH1_x ( min16float ( 0.5 ) ) + AH1_x ( min16float ( 0.75 ) ) ) ; x = x * AH1_x ( min16float ( 2.0 ) ) - AH1_x ( min16float ( 1.0 ) ) ; return APSinH1 ( x ) ; } 
min16float2 APCosH2 ( min16float2 x ) { x = AFractH2 ( x * AH2_x ( min16float ( 0.5 ) ) + AH2_x ( min16float ( 0.75 ) ) ) ; x = x * AH2_x ( min16float ( 2.0 ) ) - AH2_x ( min16float ( 1.0 ) ) ; return APSinH2 ( x ) ; } 
min16float2 APSinCosH1 ( min16float x ) { min16float y = AFractH1 ( x * AH1_x ( min16float ( 0.5 ) ) + AH1_x ( min16float ( 0.75 ) ) ) ; y = y * AH1_x ( min16float ( 2.0 ) ) - AH1_x ( min16float ( 1.0 ) ) ; return APSinH2 ( min16float2 ( x , y ) ) ; } 


#line 987

uint AZolAndU1 ( uint x , uint y ) { return min ( x , y ) ; } 
uint2 AZolAndU2 ( uint2 x , uint2 y ) { return min ( x , y ) ; } 
uint3 AZolAndU3 ( uint3 x , uint3 y ) { return min ( x , y ) ; } 
uint4 AZolAndU4 ( uint4 x , uint4 y ) { return min ( x , y ) ; } 

uint AZolNotU1 ( uint x ) { return x ^ AU1_x ( uint ( 1 ) ) ; } 
uint2 AZolNotU2 ( uint2 x ) { return x ^ AU2_x ( uint ( 1 ) ) ; } 
uint3 AZolNotU3 ( uint3 x ) { return x ^ AU3_x ( uint ( 1 ) ) ; } 
uint4 AZolNotU4 ( uint4 x ) { return x ^ AU4_x ( uint ( 1 ) ) ; } 

uint AZolOrU1 ( uint x , uint y ) { return max ( x , y ) ; } 
uint2 AZolOrU2 ( uint2 x , uint2 y ) { return max ( x , y ) ; } 
uint3 AZolOrU3 ( uint3 x , uint3 y ) { return max ( x , y ) ; } 
uint4 AZolOrU4 ( uint4 x , uint4 y ) { return max ( x , y ) ; } 

uint AZolF1ToU1 ( float x ) { return uint ( x ) ; } 
uint2 AZolF2ToU2 ( float2 x ) { return uint2 ( x ) ; } 
uint3 AZolF3ToU3 ( float3 x ) { return uint3 ( x ) ; } 
uint4 AZolF4ToU4 ( float4 x ) { return uint4 ( x ) ; } 

#line 1009
uint AZolNotF1ToU1 ( float x ) { return uint ( AF1_x ( float ( 1.0 ) ) - x ) ; } 
uint2 AZolNotF2ToU2 ( float2 x ) { return uint2 ( AF2_x ( float ( 1.0 ) ) - x ) ; } 
uint3 AZolNotF3ToU3 ( float3 x ) { return uint3 ( AF3_x ( float ( 1.0 ) ) - x ) ; } 
uint4 AZolNotF4ToU4 ( float4 x ) { return uint4 ( AF4_x ( float ( 1.0 ) ) - x ) ; } 

float AZolU1ToF1 ( uint x ) { return float ( x ) ; } 
float2 AZolU2ToF2 ( uint2 x ) { return float2 ( x ) ; } 
float3 AZolU3ToF3 ( uint3 x ) { return float3 ( x ) ; } 
float4 AZolU4ToF4 ( uint4 x ) { return float4 ( x ) ; } 

float AZolAndF1 ( float x , float y ) { return min ( x , y ) ; } 
float2 AZolAndF2 ( float2 x , float2 y ) { return min ( x , y ) ; } 
float3 AZolAndF3 ( float3 x , float3 y ) { return min ( x , y ) ; } 
float4 AZolAndF4 ( float4 x , float4 y ) { return min ( x , y ) ; } 

float ASolAndNotF1 ( float x , float y ) { return ( - x ) * y + AF1_x ( float ( 1.0 ) ) ; } 
float2 ASolAndNotF2 ( float2 x , float2 y ) { return ( - x ) * y + AF2_x ( float ( 1.0 ) ) ; } 
float3 ASolAndNotF3 ( float3 x , float3 y ) { return ( - x ) * y + AF3_x ( float ( 1.0 ) ) ; } 
float4 ASolAndNotF4 ( float4 x , float4 y ) { return ( - x ) * y + AF4_x ( float ( 1.0 ) ) ; } 

float AZolAndOrF1 ( float x , float y , float z ) { return ASatF1 ( x * y + z ) ; } 
float2 AZolAndOrF2 ( float2 x , float2 y , float2 z ) { return ASatF2 ( x * y + z ) ; } 
float3 AZolAndOrF3 ( float3 x , float3 y , float3 z ) { return ASatF3 ( x * y + z ) ; } 
float4 AZolAndOrF4 ( float4 x , float4 y , float4 z ) { return ASatF4 ( x * y + z ) ; } 

float AZolGtZeroF1 ( float x ) { return ASatF1 ( x * AF1_x ( float ( asfloat ( uint ( 0x7f800000u ) ) ) ) ) ; } 
float2 AZolGtZeroF2 ( float2 x ) { return ASatF2 ( x * AF2_x ( float ( asfloat ( uint ( 0x7f800000u ) ) ) ) ) ; } 
float3 AZolGtZeroF3 ( float3 x ) { return ASatF3 ( x * AF3_x ( float ( asfloat ( uint ( 0x7f800000u ) ) ) ) ) ; } 
float4 AZolGtZeroF4 ( float4 x ) { return ASatF4 ( x * AF4_x ( float ( asfloat ( uint ( 0x7f800000u ) ) ) ) ) ; } 

float AZolNotF1 ( float x ) { return AF1_x ( float ( 1.0 ) ) - x ; } 
float2 AZolNotF2 ( float2 x ) { return AF2_x ( float ( 1.0 ) ) - x ; } 
float3 AZolNotF3 ( float3 x ) { return AF3_x ( float ( 1.0 ) ) - x ; } 
float4 AZolNotF4 ( float4 x ) { return AF4_x ( float ( 1.0 ) ) - x ; } 

float AZolOrF1 ( float x , float y ) { return max ( x , y ) ; } 
float2 AZolOrF2 ( float2 x , float2 y ) { return max ( x , y ) ; } 
float3 AZolOrF3 ( float3 x , float3 y ) { return max ( x , y ) ; } 
float4 AZolOrF4 ( float4 x , float4 y ) { return max ( x , y ) ; } 

float AZolSelF1 ( float x , float y , float z ) { float r = ( - x ) * z + z ; return x * y + r ; } 
float2 AZolSelF2 ( float2 x , float2 y , float2 z ) { float2 r = ( - x ) * z + z ; return x * y + r ; } 
float3 AZolSelF3 ( float3 x , float3 y , float3 z ) { float3 r = ( - x ) * z + z ; return x * y + r ; } 
float4 AZolSelF4 ( float4 x , float4 y , float4 z ) { float4 r = ( - x ) * z + z ; return x * y + r ; } 

float AZolSignedF1 ( float x ) { return ASatF1 ( x * AF1_x ( float ( asfloat ( uint ( 0xff800000u ) ) ) ) ) ; } 
float2 AZolSignedF2 ( float2 x ) { return ASatF2 ( x * AF2_x ( float ( asfloat ( uint ( 0xff800000u ) ) ) ) ) ; } 
float3 AZolSignedF3 ( float3 x ) { return ASatF3 ( x * AF3_x ( float ( asfloat ( uint ( 0xff800000u ) ) ) ) ) ; } 
float4 AZolSignedF4 ( float4 x ) { return ASatF4 ( x * AF4_x ( float ( asfloat ( uint ( 0xff800000u ) ) ) ) ) ; } 

float AZolZeroPassF1 ( float x , float y ) { return asfloat ( uint ( ( asuint ( float ( x ) ) != AU1_x ( uint ( 0 ) ) ) ? AU1_x ( uint ( 0 ) ) : asuint ( float ( y ) ) ) ) ; } 
float2 AZolZeroPassF2 ( float2 x , float2 y ) { return asfloat ( uint2 ( ( asuint ( float2 ( x ) ) != AU2_x ( uint ( 0 ) ) ) ? AU2_x ( uint ( 0 ) ) : asuint ( float2 ( y ) ) ) ) ; } 
float3 AZolZeroPassF3 ( float3 x , float3 y ) { return asfloat ( uint3 ( ( asuint ( float3 ( x ) ) != AU3_x ( uint ( 0 ) ) ) ? AU3_x ( uint ( 0 ) ) : asuint ( float3 ( y ) ) ) ) ; } 
float4 AZolZeroPassF4 ( float4 x , float4 y ) { return asfloat ( uint4 ( ( asuint ( float4 ( x ) ) != AU4_x ( uint ( 0 ) ) ) ? AU4_x ( uint ( 0 ) ) : asuint ( float4 ( y ) ) ) ) ; } 



min16uint AZolAndW1 ( min16uint x , min16uint y ) { return min ( x , y ) ; } 
min16uint2 AZolAndW2 ( min16uint2 x , min16uint2 y ) { return min ( x , y ) ; } 
min16uint3 AZolAndW3 ( min16uint3 x , min16uint3 y ) { return min ( x , y ) ; } 
min16uint4 AZolAndW4 ( min16uint4 x , min16uint4 y ) { return min ( x , y ) ; } 

min16uint AZolNotW1 ( min16uint x ) { return x ^ AW1_x ( min16uint ( 1 ) ) ; } 
min16uint2 AZolNotW2 ( min16uint2 x ) { return x ^ AW2_x ( min16uint ( 1 ) ) ; } 
min16uint3 AZolNotW3 ( min16uint3 x ) { return x ^ AW3_x ( min16uint ( 1 ) ) ; } 
min16uint4 AZolNotW4 ( min16uint4 x ) { return x ^ AW4_x ( min16uint ( 1 ) ) ; } 

min16uint AZolOrW1 ( min16uint x , min16uint y ) { return max ( x , y ) ; } 
min16uint2 AZolOrW2 ( min16uint2 x , min16uint2 y ) { return max ( x , y ) ; } 
min16uint3 AZolOrW3 ( min16uint3 x , min16uint3 y ) { return max ( x , y ) ; } 
min16uint4 AZolOrW4 ( min16uint4 x , min16uint4 y ) { return max ( x , y ) ; } 

#line 1082
min16uint AZolH1ToW1 ( min16float x ) { return min16uint ( f32tof16 ( float ( x * min16float ( f16tof32 ( uint ( AW1_x ( min16uint ( 1 ) ) ) ) ) ) ) ) ; } 
min16uint2 AZolH2ToW2 ( min16float2 x ) { return min16uint2 ( min16uint ( f32tof16 ( float ( ( x * min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_x ( min16uint ( 1 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_x ( min16uint ( 1 ) ) ) . y ) ) ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( x * min16float2 ( min16float ( f16tof32 ( uint ( ( AW2_x ( min16uint ( 1 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW2_x ( min16uint ( 1 ) ) ) . y ) ) ) ) ) . y ) ) ) ) ; } 
min16uint3 AZolH3ToW3 ( min16float3 x ) { return min16uint3 ( min16uint ( f32tof16 ( float ( ( x * min16float3 ( min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 1 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 1 ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 1 ) ) ) . z ) ) ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( x * min16float3 ( min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 1 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 1 ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 1 ) ) ) . z ) ) ) ) ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( x * min16float3 ( min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 1 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 1 ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW3_x ( min16uint ( 1 ) ) ) . z ) ) ) ) ) . z ) ) ) ) ; } 
min16uint4 AZolH4ToW4 ( min16float4 x ) { return min16uint4 ( min16uint ( f32tof16 ( float ( ( x * min16float4 ( min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . z ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . w ) ) ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( x * min16float4 ( min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . z ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . w ) ) ) ) ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( x * min16float4 ( min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . z ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . w ) ) ) ) ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( x * min16float4 ( min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . z ) ) ) , min16float ( f16tof32 ( uint ( ( AW4_x ( min16uint ( 1 ) ) ) . w ) ) ) ) ) . w ) ) ) ) ; } 

#line 1088
min16float AZolW1ToH1 ( min16uint x ) { return min16float ( f16tof32 ( uint ( x * min16uint ( f32tof16 ( float ( AH1_x ( min16float ( 1.0 ) ) ) ) ) ) ) ) ; } 
min16float2 AZolW2ToH2 ( min16uint2 x ) { return min16float2 ( min16float ( f16tof32 ( uint ( ( x * min16uint2 ( min16uint ( f32tof16 ( float ( ( AH2_x ( min16float ( 1.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( AH2_x ( min16float ( 1.0 ) ) ) . y ) ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( x * min16uint2 ( min16uint ( f32tof16 ( float ( ( AH2_x ( min16float ( 1.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( AH2_x ( min16float ( 1.0 ) ) ) . y ) ) ) ) ) . y ) ) ) ) ; } 
min16float3 AZolW1ToH3 ( min16uint3 x ) { return min16float3 ( min16float ( f16tof32 ( uint ( ( x * min16uint3 ( min16uint ( f32tof16 ( float ( ( AH3_x ( min16float ( 1.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( AH3_x ( min16float ( 1.0 ) ) ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( AH3_x ( min16float ( 1.0 ) ) ) . z ) ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( x * min16uint3 ( min16uint ( f32tof16 ( float ( ( AH3_x ( min16float ( 1.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( AH3_x ( min16float ( 1.0 ) ) ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( AH3_x ( min16float ( 1.0 ) ) ) . z ) ) ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( x * min16uint3 ( min16uint ( f32tof16 ( float ( ( AH3_x ( min16float ( 1.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( AH3_x ( min16float ( 1.0 ) ) ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( AH3_x ( min16float ( 1.0 ) ) ) . z ) ) ) ) ) . z ) ) ) ) ; } 
min16float4 AZolW2ToH4 ( min16uint4 x ) { return min16float4 ( min16float ( f16tof32 ( uint ( ( x * min16uint4 ( min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . w ) ) ) ) ) . x ) ) ) , min16float ( f16tof32 ( uint ( ( x * min16uint4 ( min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . w ) ) ) ) ) . y ) ) ) , min16float ( f16tof32 ( uint ( ( x * min16uint4 ( min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . w ) ) ) ) ) . z ) ) ) , min16float ( f16tof32 ( uint ( ( x * min16uint4 ( min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . x ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . y ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . z ) ) ) , min16uint ( f32tof16 ( float ( ( AH4_x ( min16float ( 1.0 ) ) ) . w ) ) ) ) ) . w ) ) ) ) ; } 

min16float AZolAndH1 ( min16float x , min16float y ) { return min ( x , y ) ; } 
min16float2 AZolAndH2 ( min16float2 x , min16float2 y ) { return min ( x , y ) ; } 
min16float3 AZolAndH3 ( min16float3 x , min16float3 y ) { return min ( x , y ) ; } 
min16float4 AZolAndH4 ( min16float4 x , min16float4 y ) { return min ( x , y ) ; } 

min16float ASolAndNotH1 ( min16float x , min16float y ) { return ( - x ) * y + AH1_x ( min16float ( 1.0 ) ) ; } 
min16float2 ASolAndNotH2 ( min16float2 x , min16float2 y ) { return ( - x ) * y + AH2_x ( min16float ( 1.0 ) ) ; } 
min16float3 ASolAndNotH3 ( min16float3 x , min16float3 y ) { return ( - x ) * y + AH3_x ( min16float ( 1.0 ) ) ; } 
min16float4 ASolAndNotH4 ( min16float4 x , min16float4 y ) { return ( - x ) * y + AH4_x ( min16float ( 1.0 ) ) ; } 

min16float AZolAndOrH1 ( min16float x , min16float y , min16float z ) { return ASatH1 ( x * y + z ) ; } 
min16float2 AZolAndOrH2 ( min16float2 x , min16float2 y , min16float2 z ) { return ASatH2 ( x * y + z ) ; } 
min16float3 AZolAndOrH3 ( min16float3 x , min16float3 y , min16float3 z ) { return ASatH3 ( x * y + z ) ; } 
min16float4 AZolAndOrH4 ( min16float4 x , min16float4 y , min16float4 z ) { return ASatH4 ( x * y + z ) ; } 

min16float AZolGtZeroH1 ( min16float x ) { return ASatH1 ( x * AH1_x ( min16float ( min16float ( f16tof32 ( uint ( 0x7c00u ) ) ) ) ) ) ; } 
min16float2 AZolGtZeroH2 ( min16float2 x ) { return ASatH2 ( x * AH2_x ( min16float ( min16float ( f16tof32 ( uint ( 0x7c00u ) ) ) ) ) ) ; } 
min16float3 AZolGtZeroH3 ( min16float3 x ) { return ASatH3 ( x * AH3_x ( min16float ( min16float ( f16tof32 ( uint ( 0x7c00u ) ) ) ) ) ) ; } 
min16float4 AZolGtZeroH4 ( min16float4 x ) { return ASatH4 ( x * AH4_x ( min16float ( min16float ( f16tof32 ( uint ( 0x7c00u ) ) ) ) ) ) ; } 

min16float AZolNotH1 ( min16float x ) { return AH1_x ( min16float ( 1.0 ) ) - x ; } 
min16float2 AZolNotH2 ( min16float2 x ) { return AH2_x ( min16float ( 1.0 ) ) - x ; } 
min16float3 AZolNotH3 ( min16float3 x ) { return AH3_x ( min16float ( 1.0 ) ) - x ; } 
min16float4 AZolNotH4 ( min16float4 x ) { return AH4_x ( min16float ( 1.0 ) ) - x ; } 

min16float AZolOrH1 ( min16float x , min16float y ) { return max ( x , y ) ; } 
min16float2 AZolOrH2 ( min16float2 x , min16float2 y ) { return max ( x , y ) ; } 
min16float3 AZolOrH3 ( min16float3 x , min16float3 y ) { return max ( x , y ) ; } 
min16float4 AZolOrH4 ( min16float4 x , min16float4 y ) { return max ( x , y ) ; } 

min16float AZolSelH1 ( min16float x , min16float y , min16float z ) { min16float r = ( - x ) * z + z ; return x * y + r ; } 
min16float2 AZolSelH2 ( min16float2 x , min16float2 y , min16float2 z ) { min16float2 r = ( - x ) * z + z ; return x * y + r ; } 
min16float3 AZolSelH3 ( min16float3 x , min16float3 y , min16float3 z ) { min16float3 r = ( - x ) * z + z ; return x * y + r ; } 
min16float4 AZolSelH4 ( min16float4 x , min16float4 y , min16float4 z ) { min16float4 r = ( - x ) * z + z ; return x * y + r ; } 

min16float AZolSignedH1 ( min16float x ) { return ASatH1 ( x * AH1_x ( min16float ( min16float ( f16tof32 ( uint ( 0xfc00u ) ) ) ) ) ) ; } 
min16float2 AZolSignedH2 ( min16float2 x ) { return ASatH2 ( x * AH2_x ( min16float ( min16float ( f16tof32 ( uint ( 0xfc00u ) ) ) ) ) ) ; } 
min16float3 AZolSignedH3 ( min16float3 x ) { return ASatH3 ( x * AH3_x ( min16float ( min16float ( f16tof32 ( uint ( 0xfc00u ) ) ) ) ) ) ; } 
min16float4 AZolSignedH4 ( min16float4 x ) { return ASatH4 ( x * AH4_x ( min16float ( min16float ( f16tof32 ( uint ( 0xfc00u ) ) ) ) ) ) ; } 


#line 1185

float ATo709F1 ( float c ) { 
    float3 j = float3 ( 0.018 * 4.5 , 4.5 , 0.45 ) ; float2 k = float2 ( 1.099 , - 0.099 ) ; 
    return clamp ( j . x , c * j . y , pow ( c , j . z ) * k . x + k . y ) ; 
} 
float2 ATo709F2 ( float2 c ) { 
    float3 j = float3 ( 0.018 * 4.5 , 4.5 , 0.45 ) ; float2 k = float2 ( 1.099 , - 0.099 ) ; 
    return clamp ( j . xx , c * j . yy , pow ( c , j . zz ) * k . xx + k . yy ) ; 
} 
float3 ATo709F3 ( float3 c ) { 
    float3 j = float3 ( 0.018 * 4.5 , 4.5 , 0.45 ) ; float2 k = float2 ( 1.099 , - 0.099 ) ; 
    return clamp ( j . xxx , c * j . yyy , pow ( c , j . zzz ) * k . xxx + k . yyy ) ; 
} 

#line 1200
float AToGammaF1 ( float c , float rcpX ) { return pow ( c , AF1_x ( float ( rcpX ) ) ) ; } 
float2 AToGammaF2 ( float2 c , float rcpX ) { return pow ( c , AF2_x ( float ( rcpX ) ) ) ; } 
float3 AToGammaF3 ( float3 c , float rcpX ) { return pow ( c , AF3_x ( float ( rcpX ) ) ) ; } 

float AToPqF1 ( float x ) { 
    float p = pow ( x , AF1_x ( float ( 0.159302 ) ) ) ; 
    return pow ( ( AF1_x ( float ( 0.835938 ) ) + AF1_x ( float ( 18.8516 ) ) * p ) / ( AF1_x ( float ( 1.0 ) ) + AF1_x ( float ( 18.6875 ) ) * p ) , AF1_x ( float ( 78.8438 ) ) ) ; 
} 
float2 AToPqF1 ( float2 x ) { 
    float2 p = pow ( x , AF2_x ( float ( 0.159302 ) ) ) ; 
    return pow ( ( AF2_x ( float ( 0.835938 ) ) + AF2_x ( float ( 18.8516 ) ) * p ) / ( AF2_x ( float ( 1.0 ) ) + AF2_x ( float ( 18.6875 ) ) * p ) , AF2_x ( float ( 78.8438 ) ) ) ; 
} 
float3 AToPqF1 ( float3 x ) { 
    float3 p = pow ( x , AF3_x ( float ( 0.159302 ) ) ) ; 
    return pow ( ( AF3_x ( float ( 0.835938 ) ) + AF3_x ( float ( 18.8516 ) ) * p ) / ( AF3_x ( float ( 1.0 ) ) + AF3_x ( float ( 18.6875 ) ) * p ) , AF3_x ( float ( 78.8438 ) ) ) ; 
} 

float AToSrgbF1 ( float c ) { 
    float3 j = float3 ( 0.0031308 * 12.92 , 12.92 , 1.0 / 2.4 ) ; float2 k = float2 ( 1.055 , - 0.055 ) ; 
    return clamp ( j . x , c * j . y , pow ( c , j . z ) * k . x + k . y ) ; 
} 
float2 AToSrgbF2 ( float2 c ) { 
    float3 j = float3 ( 0.0031308 * 12.92 , 12.92 , 1.0 / 2.4 ) ; float2 k = float2 ( 1.055 , - 0.055 ) ; 
    return clamp ( j . xx , c * j . yy , pow ( c , j . zz ) * k . xx + k . yy ) ; 
} 
float3 AToSrgbF3 ( float3 c ) { 
    float3 j = float3 ( 0.0031308 * 12.92 , 12.92 , 1.0 / 2.4 ) ; float2 k = float2 ( 1.055 , - 0.055 ) ; 
    return clamp ( j . xxx , c * j . yyy , pow ( c , j . zzz ) * k . xxx + k . yyy ) ; 
} 

float AToTwoF1 ( float c ) { return sqrt ( c ) ; } 
float2 AToTwoF2 ( float2 c ) { return sqrt ( c ) ; } 
float3 AToTwoF3 ( float3 c ) { return sqrt ( c ) ; } 

float AToThreeF1 ( float c ) { return pow ( c , AF1_x ( float ( 1.0 / 3.0 ) ) ) ; } 
float2 AToThreeF2 ( float2 c ) { return pow ( c , AF2_x ( float ( 1.0 / 3.0 ) ) ) ; } 
float3 AToThreeF3 ( float3 c ) { return pow ( c , AF3_x ( float ( 1.0 / 3.0 ) ) ) ; } 




float AFrom709F1 ( float c ) { 
    float3 j = float3 ( 0.081 / 4.5 , 1.0 / 4.5 , 1.0 / 0.45 ) ; float2 k = float2 ( 1.0 / 1.099 , 0.099 / 1.099 ) ; 
    return AZolSelF1 ( AZolSignedF1 ( c - j . x ) , c * j . y , pow ( c * k . x + k . y , j . z ) ) ; 
} 
float2 AFrom709F2 ( float2 c ) { 
    float3 j = float3 ( 0.081 / 4.5 , 1.0 / 4.5 , 1.0 / 0.45 ) ; float2 k = float2 ( 1.0 / 1.099 , 0.099 / 1.099 ) ; 
    return AZolSelF2 ( AZolSignedF2 ( c - j . xx ) , c * j . yy , pow ( c * k . xx + k . yy , j . zz ) ) ; 
} 
float3 AFrom709F3 ( float3 c ) { 
    float3 j = float3 ( 0.081 / 4.5 , 1.0 / 4.5 , 1.0 / 0.45 ) ; float2 k = float2 ( 1.0 / 1.099 , 0.099 / 1.099 ) ; 
    return AZolSelF3 ( AZolSignedF3 ( c - j . xxx ) , c * j . yyy , pow ( c * k . xxx + k . yyy , j . zzz ) ) ; 
} 

float AFromGammaF1 ( float c , float x ) { return pow ( c , AF1_x ( float ( x ) ) ) ; } 
float2 AFromGammaF2 ( float2 c , float x ) { return pow ( c , AF2_x ( float ( x ) ) ) ; } 
float3 AFromGammaF3 ( float3 c , float x ) { return pow ( c , AF3_x ( float ( x ) ) ) ; } 

float AFromPqF1 ( float x ) { 
    float p = pow ( x , AF1_x ( float ( 0.0126833 ) ) ) ; 
    return pow ( ASatF1 ( p - AF1_x ( float ( 0.835938 ) ) ) / ( AF1_x ( float ( 18.8516 ) ) - AF1_x ( float ( 18.6875 ) ) * p ) , AF1_x ( float ( 6.27739 ) ) ) ; 
} 
float2 AFromPqF1 ( float2 x ) { 
    float2 p = pow ( x , AF2_x ( float ( 0.0126833 ) ) ) ; 
    return pow ( ASatF2 ( p - AF2_x ( float ( 0.835938 ) ) ) / ( AF2_x ( float ( 18.8516 ) ) - AF2_x ( float ( 18.6875 ) ) * p ) , AF2_x ( float ( 6.27739 ) ) ) ; 
} 
float3 AFromPqF1 ( float3 x ) { 
    float3 p = pow ( x , AF3_x ( float ( 0.0126833 ) ) ) ; 
    return pow ( ASatF3 ( p - AF3_x ( float ( 0.835938 ) ) ) / ( AF3_x ( float ( 18.8516 ) ) - AF3_x ( float ( 18.6875 ) ) * p ) , AF3_x ( float ( 6.27739 ) ) ) ; 
} 

#line 1272
float AFromSrgbF1 ( float c ) { 
    float3 j = float3 ( 0.04045 / 12.92 , 1.0 / 12.92 , 2.4 ) ; float2 k = float2 ( 1.0 / 1.055 , 0.055 / 1.055 ) ; 
    return AZolSelF1 ( AZolSignedF1 ( c - j . x ) , c * j . y , pow ( c * k . x + k . y , j . z ) ) ; 
} 
float2 AFromSrgbF2 ( float2 c ) { 
    float3 j = float3 ( 0.04045 / 12.92 , 1.0 / 12.92 , 2.4 ) ; float2 k = float2 ( 1.0 / 1.055 , 0.055 / 1.055 ) ; 
    return AZolSelF2 ( AZolSignedF2 ( c - j . xx ) , c * j . yy , pow ( c * k . xx + k . yy , j . zz ) ) ; 
} 
float3 AFromSrgbF3 ( float3 c ) { 
    float3 j = float3 ( 0.04045 / 12.92 , 1.0 / 12.92 , 2.4 ) ; float2 k = float2 ( 1.0 / 1.055 , 0.055 / 1.055 ) ; 
    return AZolSelF3 ( AZolSignedF3 ( c - j . xxx ) , c * j . yyy , pow ( c * k . xxx + k . yyy , j . zzz ) ) ; 
} 

float AFromTwoF1 ( float c ) { return c * c ; } 
float2 AFromTwoF2 ( float2 c ) { return c * c ; } 
float3 AFromTwoF3 ( float3 c ) { return c * c ; } 

float AFromThreeF1 ( float c ) { return c * c * c ; } 
float2 AFromThreeF2 ( float2 c ) { return c * c * c ; } 
float3 AFromThreeF3 ( float3 c ) { return c * c * c ; } 



min16float ATo709H1 ( min16float c ) { 
    min16float3 j = min16float3 ( 0.018 * 4.5 , 4.5 , 0.45 ) ; min16float2 k = min16float2 ( 1.099 , - 0.099 ) ; 
    return clamp ( j . x , c * j . y , pow ( c , j . z ) * k . x + k . y ) ; 
} 
min16float2 ATo709H2 ( min16float2 c ) { 
    min16float3 j = min16float3 ( 0.018 * 4.5 , 4.5 , 0.45 ) ; min16float2 k = min16float2 ( 1.099 , - 0.099 ) ; 
    return clamp ( j . xx , c * j . yy , pow ( c , j . zz ) * k . xx + k . yy ) ; 
} 
min16float3 ATo709H3 ( min16float3 c ) { 
    min16float3 j = min16float3 ( 0.018 * 4.5 , 4.5 , 0.45 ) ; min16float2 k = min16float2 ( 1.099 , - 0.099 ) ; 
    return clamp ( j . xxx , c * j . yyy , pow ( c , j . zzz ) * k . xxx + k . yyy ) ; 
} 

min16float AToGammaH1 ( min16float c , min16float rcpX ) { return pow ( c , AH1_x ( min16float ( rcpX ) ) ) ; } 
min16float2 AToGammaH2 ( min16float2 c , min16float rcpX ) { return pow ( c , AH2_x ( min16float ( rcpX ) ) ) ; } 
min16float3 AToGammaH3 ( min16float3 c , min16float rcpX ) { return pow ( c , AH3_x ( min16float ( rcpX ) ) ) ; } 

min16float AToSrgbH1 ( min16float c ) { 
    min16float3 j = min16float3 ( 0.0031308 * 12.92 , 12.92 , 1.0 / 2.4 ) ; min16float2 k = min16float2 ( 1.055 , - 0.055 ) ; 
    return clamp ( j . x , c * j . y , pow ( c , j . z ) * k . x + k . y ) ; 
} 
min16float2 AToSrgbH2 ( min16float2 c ) { 
    min16float3 j = min16float3 ( 0.0031308 * 12.92 , 12.92 , 1.0 / 2.4 ) ; min16float2 k = min16float2 ( 1.055 , - 0.055 ) ; 
    return clamp ( j . xx , c * j . yy , pow ( c , j . zz ) * k . xx + k . yy ) ; 
} 
min16float3 AToSrgbH3 ( min16float3 c ) { 
    min16float3 j = min16float3 ( 0.0031308 * 12.92 , 12.92 , 1.0 / 2.4 ) ; min16float2 k = min16float2 ( 1.055 , - 0.055 ) ; 
    return clamp ( j . xxx , c * j . yyy , pow ( c , j . zzz ) * k . xxx + k . yyy ) ; 
} 

min16float AToTwoH1 ( min16float c ) { return sqrt ( c ) ; } 
min16float2 AToTwoH2 ( min16float2 c ) { return sqrt ( c ) ; } 
min16float3 AToTwoH3 ( min16float3 c ) { return sqrt ( c ) ; } 

min16float AToThreeF1 ( min16float c ) { return pow ( c , AH1_x ( min16float ( 1.0 / 3.0 ) ) ) ; } 
min16float2 AToThreeF2 ( min16float2 c ) { return pow ( c , AH2_x ( min16float ( 1.0 / 3.0 ) ) ) ; } 
min16float3 AToThreeF3 ( min16float3 c ) { return pow ( c , AH3_x ( min16float ( 1.0 / 3.0 ) ) ) ; } 



min16float AFrom709H1 ( min16float c ) { 
    min16float3 j = min16float3 ( 0.081 / 4.5 , 1.0 / 4.5 , 1.0 / 0.45 ) ; min16float2 k = min16float2 ( 1.0 / 1.099 , 0.099 / 1.099 ) ; 
    return AZolSelH1 ( AZolSignedH1 ( c - j . x ) , c * j . y , pow ( c * k . x + k . y , j . z ) ) ; 
} 
min16float2 AFrom709H2 ( min16float2 c ) { 
    min16float3 j = min16float3 ( 0.081 / 4.5 , 1.0 / 4.5 , 1.0 / 0.45 ) ; min16float2 k = min16float2 ( 1.0 / 1.099 , 0.099 / 1.099 ) ; 
    return AZolSelH2 ( AZolSignedH2 ( c - j . xx ) , c * j . yy , pow ( c * k . xx + k . yy , j . zz ) ) ; 
} 
min16float3 AFrom709H3 ( min16float3 c ) { 
    min16float3 j = min16float3 ( 0.081 / 4.5 , 1.0 / 4.5 , 1.0 / 0.45 ) ; min16float2 k = min16float2 ( 1.0 / 1.099 , 0.099 / 1.099 ) ; 
    return AZolSelH3 ( AZolSignedH3 ( c - j . xxx ) , c * j . yyy , pow ( c * k . xxx + k . yyy , j . zzz ) ) ; 
} 

min16float AFromGammaH1 ( min16float c , min16float x ) { return pow ( c , AH1_x ( min16float ( x ) ) ) ; } 
min16float2 AFromGammaH2 ( min16float2 c , min16float x ) { return pow ( c , AH2_x ( min16float ( x ) ) ) ; } 
min16float3 AFromGammaH3 ( min16float3 c , min16float x ) { return pow ( c , AH3_x ( min16float ( x ) ) ) ; } 

min16float AHromSrgbF1 ( min16float c ) { 
    min16float3 j = min16float3 ( 0.04045 / 12.92 , 1.0 / 12.92 , 2.4 ) ; min16float2 k = min16float2 ( 1.0 / 1.055 , 0.055 / 1.055 ) ; 
    return AZolSelH1 ( AZolSignedH1 ( c - j . x ) , c * j . y , pow ( c * k . x + k . y , j . z ) ) ; 
} 
min16float2 AHromSrgbF2 ( min16float2 c ) { 
    min16float3 j = min16float3 ( 0.04045 / 12.92 , 1.0 / 12.92 , 2.4 ) ; min16float2 k = min16float2 ( 1.0 / 1.055 , 0.055 / 1.055 ) ; 
    return AZolSelH2 ( AZolSignedH2 ( c - j . xx ) , c * j . yy , pow ( c * k . xx + k . yy , j . zz ) ) ; 
} 
min16float3 AHromSrgbF3 ( min16float3 c ) { 
    min16float3 j = min16float3 ( 0.04045 / 12.92 , 1.0 / 12.92 , 2.4 ) ; min16float2 k = min16float2 ( 1.0 / 1.055 , 0.055 / 1.055 ) ; 
    return AZolSelH3 ( AZolSignedH3 ( c - j . xxx ) , c * j . yyy , pow ( c * k . xxx + k . yyy , j . zzz ) ) ; 
} 

min16float AFromTwoH1 ( min16float c ) { return c * c ; } 
min16float2 AFromTwoH2 ( min16float2 c ) { return c * c ; } 
min16float3 AFromTwoH3 ( min16float3 c ) { return c * c ; } 

min16float AFromThreeH1 ( min16float c ) { return c * c * c ; } 
min16float2 AFromThreeH2 ( min16float2 c ) { return c * c * c ; } 
min16float3 AFromThreeH3 ( min16float3 c ) { return c * c * c ; } 


#line 1384
uint2 ARmp8x8 ( uint a ) { return uint2 ( ABfe ( a , 1u , 3u ) , ABfiM ( ABfe ( a , 3u , 3u ) , a , 1u ) ) ; } 

#line 1402
uint2 ARmpRed8x8 ( uint a ) { return uint2 ( ABfiM ( ABfe ( a , 2u , 3u ) , a , 1u ) , ABfiM ( ABfe ( a , 3u , 3u ) , ABfe ( a , 1u , 2u ) , 2u ) ) ; } 


min16uint2 ARmp8x8H ( uint a ) { return min16uint2 ( ABfe ( a , 1u , 3u ) , ABfiM ( ABfe ( a , 3u , 3u ) , a , 1u ) ) ; } 
min16uint2 ARmpRed8x8H ( uint a ) { return min16uint2 ( ABfiM ( ABfe ( a , 2u , 3u ) , a , 1u ) , ABfiM ( ABfe ( a , 3u , 3u ) , ABfe ( a , 1u , 2u ) , 2u ) ) ; } 



#line 1492





#line 1502














































































#line 1585






































#line 1628



#line 1640
float2 opAAbsF2 ( out float2 d , in float2 a ) { d = abs ( a ) ; return d ; } 
float3 opAAbsF3 ( out float3 d , in float3 a ) { d = abs ( a ) ; return d ; } 
float4 opAAbsF4 ( out float4 d , in float4 a ) { d = abs ( a ) ; return d ; } 

float2 opAAddF2 ( out float2 d , in float2 a , in float2 b ) { d = a + b ; return d ; } 
float3 opAAddF3 ( out float3 d , in float3 a , in float3 b ) { d = a + b ; return d ; } 
float4 opAAddF4 ( out float4 d , in float4 a , in float4 b ) { d = a + b ; return d ; } 

float2 opAAddOneF2 ( out float2 d , in float2 a , float b ) { d = a + AF2_x ( float ( b ) ) ; return d ; } 
float3 opAAddOneF3 ( out float3 d , in float3 a , float b ) { d = a + AF3_x ( float ( b ) ) ; return d ; } 
float4 opAAddOneF4 ( out float4 d , in float4 a , float b ) { d = a + AF4_x ( float ( b ) ) ; return d ; } 

float2 opACpyF2 ( out float2 d , in float2 a ) { d = a ; return d ; } 
float3 opACpyF3 ( out float3 d , in float3 a ) { d = a ; return d ; } 
float4 opACpyF4 ( out float4 d , in float4 a ) { d = a ; return d ; } 

float2 opALerpF2 ( out float2 d , in float2 a , in float2 b , in float2 c ) { d = ALerpF2 ( a , b , c ) ; return d ; } 
float3 opALerpF3 ( out float3 d , in float3 a , in float3 b , in float3 c ) { d = ALerpF3 ( a , b , c ) ; return d ; } 
float4 opALerpF4 ( out float4 d , in float4 a , in float4 b , in float4 c ) { d = ALerpF4 ( a , b , c ) ; return d ; } 

float2 opALerpOneF2 ( out float2 d , in float2 a , in float2 b , float c ) { d = ALerpF2 ( a , b , AF2_x ( float ( c ) ) ) ; return d ; } 
float3 opALerpOneF3 ( out float3 d , in float3 a , in float3 b , float c ) { d = ALerpF3 ( a , b , AF3_x ( float ( c ) ) ) ; return d ; } 
float4 opALerpOneF4 ( out float4 d , in float4 a , in float4 b , float c ) { d = ALerpF4 ( a , b , AF4_x ( float ( c ) ) ) ; return d ; } 

float2 opAMaxF2 ( out float2 d , in float2 a , in float2 b ) { d = max ( a , b ) ; return d ; } 
float3 opAMaxF3 ( out float3 d , in float3 a , in float3 b ) { d = max ( a , b ) ; return d ; } 
float4 opAMaxF4 ( out float4 d , in float4 a , in float4 b ) { d = max ( a , b ) ; return d ; } 

float2 opAMinF2 ( out float2 d , in float2 a , in float2 b ) { d = min ( a , b ) ; return d ; } 
float3 opAMinF3 ( out float3 d , in float3 a , in float3 b ) { d = min ( a , b ) ; return d ; } 
float4 opAMinF4 ( out float4 d , in float4 a , in float4 b ) { d = min ( a , b ) ; return d ; } 

float2 opAMulF2 ( out float2 d , in float2 a , in float2 b ) { d = a * b ; return d ; } 
float3 opAMulF3 ( out float3 d , in float3 a , in float3 b ) { d = a * b ; return d ; } 
float4 opAMulF4 ( out float4 d , in float4 a , in float4 b ) { d = a * b ; return d ; } 

float2 opAMulOneF2 ( out float2 d , in float2 a , float b ) { d = a * AF2_x ( float ( b ) ) ; return d ; } 
float3 opAMulOneF3 ( out float3 d , in float3 a , float b ) { d = a * AF3_x ( float ( b ) ) ; return d ; } 
float4 opAMulOneF4 ( out float4 d , in float4 a , float b ) { d = a * AF4_x ( float ( b ) ) ; return d ; } 

float2 opANegF2 ( out float2 d , in float2 a ) { d = - a ; return d ; } 
float3 opANegF3 ( out float3 d , in float3 a ) { d = - a ; return d ; } 
float4 opANegF4 ( out float4 d , in float4 a ) { d = - a ; return d ; } 

float2 opARcpF2 ( out float2 d , in float2 a ) { d = ARcpF2 ( a ) ; return d ; } 
float3 opARcpF3 ( out float3 d , in float3 a ) { d = ARcpF3 ( a ) ; return d ; } 
float4 opARcpF4 ( out float4 d , in float4 a ) { d = ARcpF4 ( a ) ; return d ; } 



#line 1688


#line 14 "C:\\Users\\LiuXu\\source\\repos\\Magpie\\MODULE_Common\\FfxEasuShader.hlsl"



min16float4 FsrEasuRH ( float2 p ) { 
    return min16float4 ( ( InputTexture0 . GatherRed ( InputSampler0 , ( p ) , ( 0 ) ) ) ) ; 
} 
min16float4 FsrEasuGH ( float2 p ) { 
    return min16float4 ( ( InputTexture0 . GatherGreen ( InputSampler0 , ( p ) , ( 0 ) ) ) ) ; 
} 
min16float4 FsrEasuBH ( float2 p ) { 
    return min16float4 ( ( InputTexture0 . GatherBlue ( InputSampler0 , ( p ) , ( 0 ) ) ) ) ; 
} 

#line 28


#line 1 "C:\\Users\\LiuXu\\source\\repos\\Magpie\\MODULE_Common\\ffx_fsr1.hlsli"


#line 156
void FsrEasuCon ( 
out uint4 con0 , 
out uint4 con1 , 
out uint4 con2 , 
out uint4 con3 , 

float inputViewportInPixelsX , 
float inputViewportInPixelsY , 

float inputSizeInPixelsX , 
float inputSizeInPixelsY , 

float outputSizeInPixelsX , 
float outputSizeInPixelsY ) { 
    
    con0 [ 0 ] = asuint ( float ( inputViewportInPixelsX * ARcpF1 ( outputSizeInPixelsX ) ) ) ; 
    con0 [ 1 ] = asuint ( float ( inputViewportInPixelsY * ARcpF1 ( outputSizeInPixelsY ) ) ) ; 
    con0 [ 2 ] = asuint ( float ( AF1_x ( float ( 0.5 ) ) * inputViewportInPixelsX * ARcpF1 ( outputSizeInPixelsX ) - AF1_x ( float ( 0.5 ) ) ) ) ; 
    con0 [ 3 ] = asuint ( float ( AF1_x ( float ( 0.5 ) ) * inputViewportInPixelsY * ARcpF1 ( outputSizeInPixelsY ) - AF1_x ( float ( 0.5 ) ) ) ) ; 
    
#line 177
    con1 [ 0 ] = asuint ( float ( ARcpF1 ( inputSizeInPixelsX ) ) ) ; 
    con1 [ 1 ] = asuint ( float ( ARcpF1 ( inputSizeInPixelsY ) ) ) ; 
    
#line 193
    con1 [ 2 ] = asuint ( float ( AF1_x ( float ( 1.0 ) ) * ARcpF1 ( inputSizeInPixelsX ) ) ) ; 
    con1 [ 3 ] = asuint ( float ( AF1_x ( float ( - 1.0 ) ) * ARcpF1 ( inputSizeInPixelsY ) ) ) ; 
    
    con2 [ 0 ] = asuint ( float ( AF1_x ( float ( - 1.0 ) ) * ARcpF1 ( inputSizeInPixelsX ) ) ) ; 
    con2 [ 1 ] = asuint ( float ( AF1_x ( float ( 2.0 ) ) * ARcpF1 ( inputSizeInPixelsY ) ) ) ; 
    con2 [ 2 ] = asuint ( float ( AF1_x ( float ( 1.0 ) ) * ARcpF1 ( inputSizeInPixelsX ) ) ) ; 
    con2 [ 3 ] = asuint ( float ( AF1_x ( float ( 2.0 ) ) * ARcpF1 ( inputSizeInPixelsY ) ) ) ; 
    con3 [ 0 ] = asuint ( float ( AF1_x ( float ( 0.0 ) ) * ARcpF1 ( inputSizeInPixelsX ) ) ) ; 
    con3 [ 1 ] = asuint ( float ( AF1_x ( float ( 4.0 ) ) * ARcpF1 ( inputSizeInPixelsY ) ) ) ; 
    con3 [ 2 ] = con3 [ 3 ] = 0 ; 
} 

#line 206
void FsrEasuConOffset ( 
out uint4 con0 , 
out uint4 con1 , 
out uint4 con2 , 
out uint4 con3 , 

float inputViewportInPixelsX , 
float inputViewportInPixelsY , 

float inputSizeInPixelsX , 
float inputSizeInPixelsY , 

float outputSizeInPixelsX , 
float outputSizeInPixelsY , 

float inputOffsetInPixelsX , 
float inputOffsetInPixelsY ) { 
    FsrEasuCon ( con0 , con1 , con2 , con3 , inputViewportInPixelsX , inputViewportInPixelsY , inputSizeInPixelsX , inputSizeInPixelsY , outputSizeInPixelsX , outputSizeInPixelsY ) ; 
    con0 [ 2 ] = asuint ( float ( AF1_x ( float ( 0.5 ) ) * inputViewportInPixelsX * ARcpF1 ( outputSizeInPixelsX ) - AF1_x ( float ( 0.5 ) ) + inputOffsetInPixelsX ) ) ; 
    con0 [ 3 ] = asuint ( float ( AF1_x ( float ( 0.5 ) ) * inputViewportInPixelsY * ARcpF1 ( outputSizeInPixelsY ) - AF1_x ( float ( 0.5 ) ) + inputOffsetInPixelsY ) ) ; 
} 

#line 442


#line 449


min16float4 FsrEasuRH ( float2 p ) ; 
min16float4 FsrEasuGH ( float2 p ) ; 
min16float4 FsrEasuBH ( float2 p ) ; 

#line 456
void FsrEasuTapH ( 
inout min16float2 aCR , inout min16float2 aCG , inout min16float2 aCB , 
inout min16float2 aW , 
min16float2 offX , min16float2 offY , 
min16float2 dir , 
min16float2 len , 
min16float lob , 
min16float clp , 
min16float2 cR , min16float2 cG , min16float2 cB ) { 
    min16float2 vX , vY ; 
    vX = offX * dir . xx + offY * dir . yy ; 
    vY = offX * ( - dir . yy ) + offY * dir . xx ; 
    vX *= len . x ; vY *= len . y ; 
    min16float2 d2 = vX * vX + vY * vY ; 
    d2 = min ( d2 , AH2_x ( min16float ( clp ) ) ) ; 
    min16float2 wB = AH2_x ( min16float ( 2.0 / 5.0 ) ) * d2 + AH2_x ( min16float ( - 1.0 ) ) ; 
    min16float2 wA = AH2_x ( min16float ( lob ) ) * d2 + AH2_x ( min16float ( - 1.0 ) ) ; 
    wB *= wB ; 
    wA *= wA ; 
    wB = AH2_x ( min16float ( 25.0 / 16.0 ) ) * wB + AH2_x ( min16float ( - ( 25.0 / 16.0 - 1.0 ) ) ) ; 
    min16float2 w = wB * wA ; 
    aCR += cR * w ; aCG += cG * w ; aCB += cB * w ; aW += w ; 
} 

#line 481
void FsrEasuSetH ( 
inout min16float2 dirPX , inout min16float2 dirPY , 
inout min16float2 lenP , 
min16float2 pp , 
bool biST , bool biUV , 
min16float2 lA , min16float2 lB , min16float2 lC , min16float2 lD , min16float2 lE ) { 
    min16float2 w = AH2_x ( min16float ( 0.0 ) ) ; 
    if ( biST ) w = ( min16float2 ( 1.0 , 0.0 ) + min16float2 ( - pp . x , pp . x ) ) * AH2_x ( min16float ( AH1_x ( min16float ( 1.0 ) ) - pp . y ) ) ; 
    if ( biUV ) w = ( min16float2 ( 1.0 , 0.0 ) + min16float2 ( - pp . x , pp . x ) ) * AH2_x ( min16float ( pp . y ) ) ; 
    
    min16float2 dc = lD - lC ; 
    min16float2 cb = lC - lB ; 
    min16float2 lenX = max ( abs ( dc ) , abs ( cb ) ) ; 
    lenX = ARcpH2 ( lenX ) ; 
    min16float2 dirX = lD - lB ; 
    dirPX += dirX * w ; 
    lenX = ASatH2 ( abs ( dirX ) * lenX ) ; 
    lenX *= lenX ; 
    lenP += lenX * w ; 
    min16float2 ec = lE - lC ; 
    min16float2 ca = lC - lA ; 
    min16float2 lenY = max ( abs ( ec ) , abs ( ca ) ) ; 
    lenY = ARcpH2 ( lenY ) ; 
    min16float2 dirY = lE - lA ; 
    dirPY += dirY * w ; 
    lenY = ASatH2 ( abs ( dirY ) * lenY ) ; 
    lenY *= lenY ; 
    lenP += lenY * w ; 
} 

void FsrEasuH ( 
out min16float3 pix , 
uint2 ip , 
uint4 con0 , 
uint4 con1 , 
uint4 con2 , 
uint4 con3 ) { 
    
    float2 pp = float2 ( ip ) * asfloat ( uint2 ( con0 . xy ) ) + asfloat ( uint2 ( con0 . zw ) ) ; 
    float2 fp = floor ( pp ) ; 
    pp -= fp ; 
    min16float2 ppp = min16float2 ( pp ) ; 
    
    float2 p0 = fp * asfloat ( uint2 ( con1 . xy ) ) + asfloat ( uint2 ( con1 . zw ) ) ; 
    float2 p1 = p0 + asfloat ( uint2 ( con2 . xy ) ) ; 
    float2 p2 = p0 + asfloat ( uint2 ( con2 . zw ) ) ; 
    float2 p3 = p0 + asfloat ( uint2 ( con3 . xy ) ) ; 
    min16float4 bczzR = FsrEasuRH ( p0 ) ; 
    min16float4 bczzG = FsrEasuGH ( p0 ) ; 
    min16float4 bczzB = FsrEasuBH ( p0 ) ; 
    min16float4 ijfeR = FsrEasuRH ( p1 ) ; 
    min16float4 ijfeG = FsrEasuGH ( p1 ) ; 
    min16float4 ijfeB = FsrEasuBH ( p1 ) ; 
    min16float4 klhgR = FsrEasuRH ( p2 ) ; 
    min16float4 klhgG = FsrEasuGH ( p2 ) ; 
    min16float4 klhgB = FsrEasuBH ( p2 ) ; 
    min16float4 zzonR = FsrEasuRH ( p3 ) ; 
    min16float4 zzonG = FsrEasuGH ( p3 ) ; 
    min16float4 zzonB = FsrEasuBH ( p3 ) ; 
    
    min16float4 bczzL = bczzB * AH4_x ( min16float ( 0.5 ) ) + ( bczzR * AH4_x ( min16float ( 0.5 ) ) + bczzG ) ; 
    min16float4 ijfeL = ijfeB * AH4_x ( min16float ( 0.5 ) ) + ( ijfeR * AH4_x ( min16float ( 0.5 ) ) + ijfeG ) ; 
    min16float4 klhgL = klhgB * AH4_x ( min16float ( 0.5 ) ) + ( klhgR * AH4_x ( min16float ( 0.5 ) ) + klhgG ) ; 
    min16float4 zzonL = zzonB * AH4_x ( min16float ( 0.5 ) ) + ( zzonR * AH4_x ( min16float ( 0.5 ) ) + zzonG ) ; 
    min16float bL = bczzL . x ; 
    min16float cL = bczzL . y ; 
    min16float iL = ijfeL . x ; 
    min16float jL = ijfeL . y ; 
    min16float fL = ijfeL . z ; 
    min16float eL = ijfeL . w ; 
    min16float kL = klhgL . x ; 
    min16float lL = klhgL . y ; 
    min16float hL = klhgL . z ; 
    min16float gL = klhgL . w ; 
    min16float oL = zzonL . z ; 
    min16float nL = zzonL . w ; 
    
    min16float2 dirPX = AH2_x ( min16float ( 0.0 ) ) ; 
    min16float2 dirPY = AH2_x ( min16float ( 0.0 ) ) ; 
    min16float2 lenP = AH2_x ( min16float ( 0.0 ) ) ; 
    FsrEasuSetH ( dirPX , dirPY , lenP , ppp , true , false , min16float2 ( bL , cL ) , min16float2 ( eL , fL ) , min16float2 ( fL , gL ) , min16float2 ( gL , hL ) , min16float2 ( jL , kL ) ) ; 
    FsrEasuSetH ( dirPX , dirPY , lenP , ppp , false , true , min16float2 ( fL , gL ) , min16float2 ( iL , jL ) , min16float2 ( jL , kL ) , min16float2 ( kL , lL ) , min16float2 ( nL , oL ) ) ; 
    min16float2 dir = min16float2 ( dirPX . r + dirPX . g , dirPY . r + dirPY . g ) ; 
    min16float len = lenP . r + lenP . g ; 
    
    min16float2 dir2 = dir * dir ; 
    min16float dirR = dir2 . x + dir2 . y ; 
    bool zro = dirR < AH1_x ( min16float ( 1.0 / 32768.0 ) ) ; 
    dirR = APrxLoRsqH1 ( dirR ) ; 
    dirR = zro ? AH1_x ( min16float ( 1.0 ) ) : dirR ; 
    dir . x = zro ? AH1_x ( min16float ( 1.0 ) ) : dir . x ; 
    dir *= AH2_x ( min16float ( dirR ) ) ; 
    len = len * AH1_x ( min16float ( 0.5 ) ) ; 
    len *= len ; 
    min16float stretch = ( dir . x * dir . x + dir . y * dir . y ) * APrxLoRcpH1 ( max ( abs ( dir . x ) , abs ( dir . y ) ) ) ; 
    min16float2 len2 = min16float2 ( AH1_x ( min16float ( 1.0 ) ) + ( stretch - AH1_x ( min16float ( 1.0 ) ) ) * len , AH1_x ( min16float ( 1.0 ) ) + AH1_x ( min16float ( - 0.5 ) ) * len ) ; 
    min16float lob = AH1_x ( min16float ( 0.5 ) ) + AH1_x ( min16float ( ( 1.0 / 4.0 - 0.04 ) - 0.5 ) ) * len ; 
    min16float clp = APrxLoRcpH1 ( lob ) ; 
    
#line 581
    min16float2 bothR = max ( max ( min16float2 ( - ijfeR . z , ijfeR . z ) , min16float2 ( - klhgR . w , klhgR . w ) ) , max ( min16float2 ( - ijfeR . y , ijfeR . y ) , min16float2 ( - klhgR . x , klhgR . x ) ) ) ; 
    min16float2 bothG = max ( max ( min16float2 ( - ijfeG . z , ijfeG . z ) , min16float2 ( - klhgG . w , klhgG . w ) ) , max ( min16float2 ( - ijfeG . y , ijfeG . y ) , min16float2 ( - klhgG . x , klhgG . x ) ) ) ; 
    min16float2 bothB = max ( max ( min16float2 ( - ijfeB . z , ijfeB . z ) , min16float2 ( - klhgB . w , klhgB . w ) ) , max ( min16float2 ( - ijfeB . y , ijfeB . y ) , min16float2 ( - klhgB . x , klhgB . x ) ) ) ; 
    
    min16float2 pR = AH2_x ( min16float ( 0.0 ) ) ; 
    min16float2 pG = AH2_x ( min16float ( 0.0 ) ) ; 
    min16float2 pB = AH2_x ( min16float ( 0.0 ) ) ; 
    min16float2 pW = AH2_x ( min16float ( 0.0 ) ) ; 
    FsrEasuTapH ( pR , pG , pB , pW , min16float2 ( 0.0 , 1.0 ) - ppp . xx , min16float2 ( - 1.0 , - 1.0 ) - ppp . yy , dir , len2 , lob , clp , bczzR . xy , bczzG . xy , bczzB . xy ) ; 
    FsrEasuTapH ( pR , pG , pB , pW , min16float2 ( - 1.0 , 0.0 ) - ppp . xx , min16float2 ( 1.0 , 1.0 ) - ppp . yy , dir , len2 , lob , clp , ijfeR . xy , ijfeG . xy , ijfeB . xy ) ; 
    FsrEasuTapH ( pR , pG , pB , pW , min16float2 ( 0.0 , - 1.0 ) - ppp . xx , min16float2 ( 0.0 , 0.0 ) - ppp . yy , dir , len2 , lob , clp , ijfeR . zw , ijfeG . zw , ijfeB . zw ) ; 
    FsrEasuTapH ( pR , pG , pB , pW , min16float2 ( 1.0 , 2.0 ) - ppp . xx , min16float2 ( 1.0 , 1.0 ) - ppp . yy , dir , len2 , lob , clp , klhgR . xy , klhgG . xy , klhgB . xy ) ; 
    FsrEasuTapH ( pR , pG , pB , pW , min16float2 ( 2.0 , 1.0 ) - ppp . xx , min16float2 ( 0.0 , 0.0 ) - ppp . yy , dir , len2 , lob , clp , klhgR . zw , klhgG . zw , klhgB . zw ) ; 
    FsrEasuTapH ( pR , pG , pB , pW , min16float2 ( 1.0 , 0.0 ) - ppp . xx , min16float2 ( 2.0 , 2.0 ) - ppp . yy , dir , len2 , lob , clp , zzonR . zw , zzonG . zw , zzonB . zw ) ; 
    min16float3 aC = min16float3 ( pR . x + pR . y , pG . x + pG . y , pB . x + pB . y ) ; 
    min16float aW = pW . x + pW . y ; 
    
#line 599
    pix = min ( min16float3 ( bothR . y , bothG . y , bothB . y ) , max ( - min16float3 ( bothR . x , bothG . x , bothB . x ) , aC * AH3_x ( min16float ( ARcpH1 ( aW ) ) ) ) ) ; 
} 


#line 661


#line 669
void FsrRcasCon ( 
out uint4 con , 

float sharpness ) { 
    
    sharpness = exp2 ( float ( - sharpness ) ) ; 
    float2 hSharp = float2 ( sharpness , sharpness ) ; 
    con [ 0 ] = asuint ( float ( sharpness ) ) ; 
    con [ 1 ] = AU1_AH2_AF2_x ( float2 ( hSharp ) ) ; 
    con [ 2 ] = 0 ; 
    con [ 3 ] = 0 ; 
} 

#line 779


#line 786


min16float4 FsrRcasLoadH ( min16int2 p ) ; 
void FsrRcasInputH ( inout min16float r , inout min16float g , inout min16float b ) ; 

void FsrRcasH ( 
out min16float pixR , 
out min16float pixG , 
out min16float pixB , 

#line 797

uint2 ip , 
uint4 con ) { 
    
#line 804
    min16int2 sp = min16int2 ( ip ) ; 
    min16float3 b = FsrRcasLoadH ( sp + min16int2 ( 0 , - 1 ) ) . rgb ; 
    min16float3 d = FsrRcasLoadH ( sp + min16int2 ( - 1 , 0 ) ) . rgb ; 
    
#line 810
    
    min16float3 e = FsrRcasLoadH ( sp ) . rgb ; 
    
    min16float3 f = FsrRcasLoadH ( sp + min16int2 ( 1 , 0 ) ) . rgb ; 
    min16float3 h = FsrRcasLoadH ( sp + min16int2 ( 0 , 1 ) ) . rgb ; 
    
    min16float bR = b . r ; 
    min16float bG = b . g ; 
    min16float bB = b . b ; 
    min16float dR = d . r ; 
    min16float dG = d . g ; 
    min16float dB = d . b ; 
    min16float eR = e . r ; 
    min16float eG = e . g ; 
    min16float eB = e . b ; 
    min16float fR = f . r ; 
    min16float fG = f . g ; 
    min16float fB = f . b ; 
    min16float hR = h . r ; 
    min16float hG = h . g ; 
    min16float hB = h . b ; 
    
    FsrRcasInputH ( bR , bG , bB ) ; 
    FsrRcasInputH ( dR , dG , dB ) ; 
    FsrRcasInputH ( eR , eG , eB ) ; 
    FsrRcasInputH ( fR , fG , fB ) ; 
    FsrRcasInputH ( hR , hG , hB ) ; 
    
    min16float bL = bB * AH1_x ( min16float ( 0.5 ) ) + ( bR * AH1_x ( min16float ( 0.5 ) ) + bG ) ; 
    min16float dL = dB * AH1_x ( min16float ( 0.5 ) ) + ( dR * AH1_x ( min16float ( 0.5 ) ) + dG ) ; 
    min16float eL = eB * AH1_x ( min16float ( 0.5 ) ) + ( eR * AH1_x ( min16float ( 0.5 ) ) + eG ) ; 
    min16float fL = fB * AH1_x ( min16float ( 0.5 ) ) + ( fR * AH1_x ( min16float ( 0.5 ) ) + fG ) ; 
    min16float hL = hB * AH1_x ( min16float ( 0.5 ) ) + ( hR * AH1_x ( min16float ( 0.5 ) ) + hG ) ; 
    
    min16float nz = AH1_x ( min16float ( 0.25 ) ) * bL + AH1_x ( min16float ( 0.25 ) ) * dL + AH1_x ( min16float ( 0.25 ) ) * fL + AH1_x ( min16float ( 0.25 ) ) * hL - eL ; 
    nz = ASatH1 ( abs ( nz ) * APrxMedRcpH1 ( AMax3H1 ( AMax3H1 ( bL , dL , eL ) , fL , hL ) - AMin3H1 ( AMin3H1 ( bL , dL , eL ) , fL , hL ) ) ) ; 
    nz = AH1_x ( min16float ( - 0.5 ) ) * nz + AH1_x ( min16float ( 1.0 ) ) ; 
    
    min16float mn4R = min ( AMin3H1 ( bR , dR , fR ) , hR ) ; 
    min16float mn4G = min ( AMin3H1 ( bG , dG , fG ) , hG ) ; 
    min16float mn4B = min ( AMin3H1 ( bB , dB , fB ) , hB ) ; 
    min16float mx4R = max ( AMax3H1 ( bR , dR , fR ) , hR ) ; 
    min16float mx4G = max ( AMax3H1 ( bG , dG , fG ) , hG ) ; 
    min16float mx4B = max ( AMax3H1 ( bB , dB , fB ) , hB ) ; 
    
    min16float2 peakC = min16float2 ( 1.0 , - 1.0 * 4.0 ) ; 
    
    min16float hitMinR = mn4R * ARcpH1 ( AH1_x ( min16float ( 4.0 ) ) * mx4R ) ; 
    min16float hitMinG = mn4G * ARcpH1 ( AH1_x ( min16float ( 4.0 ) ) * mx4G ) ; 
    min16float hitMinB = mn4B * ARcpH1 ( AH1_x ( min16float ( 4.0 ) ) * mx4B ) ; 
    min16float hitMaxR = ( peakC . x - mx4R ) * ARcpH1 ( AH1_x ( min16float ( 4.0 ) ) * mn4R + peakC . y ) ; 
    min16float hitMaxG = ( peakC . x - mx4G ) * ARcpH1 ( AH1_x ( min16float ( 4.0 ) ) * mn4G + peakC . y ) ; 
    min16float hitMaxB = ( peakC . x - mx4B ) * ARcpH1 ( AH1_x ( min16float ( 4.0 ) ) * mn4B + peakC . y ) ; 
    min16float lobeR = max ( - hitMinR , hitMaxR ) ; 
    min16float lobeG = max ( - hitMinG , hitMaxG ) ; 
    min16float lobeB = max ( - hitMinB , hitMaxB ) ; 
    min16float lobe = max ( AH1_x ( min16float ( - ( 0.25 - ( 1.0 / 16.0 ) ) ) ) , min ( AMax3H1 ( lobeR , lobeG , lobeB ) , AH1_x ( min16float ( 0.0 ) ) ) ) * AH2_AU1_x ( uint ( con . y ) ) . x ; 
    
#line 870
    
    
    min16float rcpL = APrxMedRcpH1 ( AH1_x ( min16float ( 4.0 ) ) * lobe + AH1_x ( min16float ( 1.0 ) ) ) ; 
    pixR = ( lobe * bR + lobe * dR + lobe * hR + lobe * fR + eR ) * rcpL ; 
    pixG = ( lobe * bG + lobe * dG + lobe * hG + lobe * fG + eG ) * rcpL ; 
    pixB = ( lobe * bB + lobe * dB + lobe * hB + lobe * fB + eB ) * rcpL ; 
} 


#line 997


#line 1024


void FsrLfgaF ( inout float3 c , float3 t , float a ) { c += ( t * AF3_x ( float ( a ) ) ) * min ( AF3_x ( float ( 1.0 ) ) - c , c ) ; } 




void FsrLfgaH ( inout min16float3 c , min16float3 t , min16float a ) { c += ( t * AH3_x ( min16float ( a ) ) ) * min ( AH3_x ( min16float ( 1.0 ) ) - c , c ) ; } 

#line 1034
void FsrLfgaHx2 ( inout min16float2 cR , inout min16float2 cG , inout min16float2 cB , min16float2 tR , min16float2 tG , min16float2 tB , min16float a ) { 
    cR += ( tR * AH2_x ( min16float ( a ) ) ) * min ( AH2_x ( min16float ( 1.0 ) ) - cR , cR ) ; cG += ( tG * AH2_x ( min16float ( a ) ) ) * min ( AH2_x ( min16float ( 1.0 ) ) - cG , cG ) ; cB += ( tB * AH2_x ( min16float ( a ) ) ) * min ( AH2_x ( min16float ( 1.0 ) ) - cB , cB ) ; 
} 


#line 1055

void FsrSrtmF ( inout float3 c ) { c *= AF3_x ( float ( ARcpF1 ( AMax3F1 ( c . r , c . g , c . b ) + AF1_x ( float ( 1.0 ) ) ) ) ) ; } 

void FsrSrtmInvF ( inout float3 c ) { c *= AF3_x ( float ( ARcpF1 ( max ( AF1_x ( float ( 1.0 / 32768.0 ) ) , AF1_x ( float ( 1.0 ) ) - AMax3F1 ( c . r , c . g , c . b ) ) ) ) ) ; } 



void FsrSrtmH ( inout min16float3 c ) { c *= AH3_x ( min16float ( ARcpH1 ( AMax3H1 ( c . r , c . g , c . b ) + AH1_x ( min16float ( 1.0 ) ) ) ) ) ; } 
void FsrSrtmInvH ( inout min16float3 c ) { c *= AH3_x ( min16float ( ARcpH1 ( max ( AH1_x ( min16float ( 1.0 / 32768.0 ) ) , AH1_x ( min16float ( 1.0 ) ) - AMax3H1 ( c . r , c . g , c . b ) ) ) ) ) ; } 

void FsrSrtmHx2 ( inout min16float2 cR , inout min16float2 cG , inout min16float2 cB ) { 
    min16float2 rcp = ARcpH2 ( AMax3H2 ( cR , cG , cB ) + AH2_x ( min16float ( 1.0 ) ) ) ; cR *= rcp ; cG *= rcp ; cB *= rcp ; 
} 
void FsrSrtmInvHx2 ( inout min16float2 cR , inout min16float2 cG , inout min16float2 cB ) { 
    min16float2 rcp = ARcpH2 ( max ( AH2_x ( min16float ( 1.0 / 32768.0 ) ) , AH2_x ( min16float ( 1.0 ) ) - AMax3H2 ( cR , cG , cB ) ) ) ; cR *= rcp ; cG *= rcp ; cB *= rcp ; 
} 


#line 1097


#line 1101
float FsrTepdDitF ( uint2 p , uint f ) { 
    float x = AF1_x ( float ( p . x + f ) ) ; 
    float y = AF1_x ( float ( p . y ) ) ; 
    
    float a = AF1_x ( float ( ( 1.0 + sqrt ( 5.0 ) ) / 2.0 ) ) ; 
    
    float b = AF1_x ( float ( 1.0 / 3.69 ) ) ; 
    x = x * a + ( y * b ) ; 
    return AFractF1 ( x ) ; 
} 

#line 1115
void FsrTepdC8F ( inout float3 c , float dit ) { 
    float3 n = sqrt ( c ) ; 
    n = floor ( n * AF3_x ( float ( 255.0 ) ) ) * AF3_x ( float ( 1.0 / 255.0 ) ) ; 
    float3 a = n * n ; 
    float3 b = n + AF3_x ( float ( 1.0 / 255.0 ) ) ; b = b * b ; 
    
#line 1123
    float3 r = ( c - b ) * APrxMedRcpF3 ( a - b ) ; 
    
#line 1126
    c = ASatF3 ( n + AGtZeroF3 ( AF3_x ( float ( dit ) ) - r ) * AF3_x ( float ( 1.0 / 255.0 ) ) ) ; 
} 

#line 1132
void FsrTepdC10F ( inout float3 c , float dit ) { 
    float3 n = sqrt ( c ) ; 
    n = floor ( n * AF3_x ( float ( 1023.0 ) ) ) * AF3_x ( float ( 1.0 / 1023.0 ) ) ; 
    float3 a = n * n ; 
    float3 b = n + AF3_x ( float ( 1.0 / 1023.0 ) ) ; b = b * b ; 
    float3 r = ( c - b ) * APrxMedRcpF3 ( a - b ) ; 
    c = ASatF3 ( n + AGtZeroF3 ( AF3_x ( float ( dit ) ) - r ) * AF3_x ( float ( 1.0 / 1023.0 ) ) ) ; 
} 



min16float FsrTepdDitH ( uint2 p , uint f ) { 
    float x = AF1_x ( float ( p . x + f ) ) ; 
    float y = AF1_x ( float ( p . y ) ) ; 
    float a = AF1_x ( float ( ( 1.0 + sqrt ( 5.0 ) ) / 2.0 ) ) ; 
    float b = AF1_x ( float ( 1.0 / 3.69 ) ) ; 
    x = x * a + ( y * b ) ; 
    return min16float ( AFractF1 ( x ) ) ; 
} 

void FsrTepdC8H ( inout min16float3 c , min16float dit ) { 
    min16float3 n = sqrt ( c ) ; 
    n = floor ( n * AH3_x ( min16float ( 255.0 ) ) ) * AH3_x ( min16float ( 1.0 / 255.0 ) ) ; 
    min16float3 a = n * n ; 
    min16float3 b = n + AH3_x ( min16float ( 1.0 / 255.0 ) ) ; b = b * b ; 
    min16float3 r = ( c - b ) * APrxMedRcpH3 ( a - b ) ; 
    c = ASatH3 ( n + AGtZeroH3 ( AH3_x ( min16float ( dit ) ) - r ) * AH3_x ( min16float ( 1.0 / 255.0 ) ) ) ; 
} 

void FsrTepdC10H ( inout min16float3 c , min16float dit ) { 
    min16float3 n = sqrt ( c ) ; 
    n = floor ( n * AH3_x ( min16float ( 1023.0 ) ) ) * AH3_x ( min16float ( 1.0 / 1023.0 ) ) ; 
    min16float3 a = n * n ; 
    min16float3 b = n + AH3_x ( min16float ( 1.0 / 1023.0 ) ) ; b = b * b ; 
    min16float3 r = ( c - b ) * APrxMedRcpH3 ( a - b ) ; 
    c = ASatH3 ( n + AGtZeroH3 ( AH3_x ( min16float ( dit ) ) - r ) * AH3_x ( min16float ( 1.0 / 1023.0 ) ) ) ; 
} 

#line 1171
min16float2 FsrTepdDitHx2 ( uint2 p , uint f ) { 
    float2 x ; 
    x . x = AF1_x ( float ( p . x + f ) ) ; 
    x . y = x . x + AF1_x ( float ( 8.0 ) ) ; 
    float y = AF1_x ( float ( p . y ) ) ; 
    float a = AF1_x ( float ( ( 1.0 + sqrt ( 5.0 ) ) / 2.0 ) ) ; 
    float b = AF1_x ( float ( 1.0 / 3.69 ) ) ; 
    x = x * AF2_x ( float ( a ) ) + AF2_x ( float ( y * b ) ) ; 
    return min16float2 ( AFractF2 ( x ) ) ; 
} 

void FsrTepdC8Hx2 ( inout min16float2 cR , inout min16float2 cG , inout min16float2 cB , min16float2 dit ) { 
    min16float2 nR = sqrt ( cR ) ; 
    min16float2 nG = sqrt ( cG ) ; 
    min16float2 nB = sqrt ( cB ) ; 
    nR = floor ( nR * AH2_x ( min16float ( 255.0 ) ) ) * AH2_x ( min16float ( 1.0 / 255.0 ) ) ; 
    nG = floor ( nG * AH2_x ( min16float ( 255.0 ) ) ) * AH2_x ( min16float ( 1.0 / 255.0 ) ) ; 
    nB = floor ( nB * AH2_x ( min16float ( 255.0 ) ) ) * AH2_x ( min16float ( 1.0 / 255.0 ) ) ; 
    min16float2 aR = nR * nR ; 
    min16float2 aG = nG * nG ; 
    min16float2 aB = nB * nB ; 
    min16float2 bR = nR + AH2_x ( min16float ( 1.0 / 255.0 ) ) ; bR = bR * bR ; 
    min16float2 bG = nG + AH2_x ( min16float ( 1.0 / 255.0 ) ) ; bG = bG * bG ; 
    min16float2 bB = nB + AH2_x ( min16float ( 1.0 / 255.0 ) ) ; bB = bB * bB ; 
    min16float2 rR = ( cR - bR ) * APrxMedRcpH2 ( aR - bR ) ; 
    min16float2 rG = ( cG - bG ) * APrxMedRcpH2 ( aG - bG ) ; 
    min16float2 rB = ( cB - bB ) * APrxMedRcpH2 ( aB - bB ) ; 
    cR = ASatH2 ( nR + AGtZeroH2 ( dit - rR ) * AH2_x ( min16float ( 1.0 / 255.0 ) ) ) ; 
    cG = ASatH2 ( nG + AGtZeroH2 ( dit - rG ) * AH2_x ( min16float ( 1.0 / 255.0 ) ) ) ; 
    cB = ASatH2 ( nB + AGtZeroH2 ( dit - rB ) * AH2_x ( min16float ( 1.0 / 255.0 ) ) ) ; 
} 

void FsrTepdC10Hx2 ( inout min16float2 cR , inout min16float2 cG , inout min16float2 cB , min16float2 dit ) { 
    min16float2 nR = sqrt ( cR ) ; 
    min16float2 nG = sqrt ( cG ) ; 
    min16float2 nB = sqrt ( cB ) ; 
    nR = floor ( nR * AH2_x ( min16float ( 1023.0 ) ) ) * AH2_x ( min16float ( 1.0 / 1023.0 ) ) ; 
    nG = floor ( nG * AH2_x ( min16float ( 1023.0 ) ) ) * AH2_x ( min16float ( 1.0 / 1023.0 ) ) ; 
    nB = floor ( nB * AH2_x ( min16float ( 1023.0 ) ) ) * AH2_x ( min16float ( 1.0 / 1023.0 ) ) ; 
    min16float2 aR = nR * nR ; 
    min16float2 aG = nG * nG ; 
    min16float2 aB = nB * nB ; 
    min16float2 bR = nR + AH2_x ( min16float ( 1.0 / 1023.0 ) ) ; bR = bR * bR ; 
    min16float2 bG = nG + AH2_x ( min16float ( 1.0 / 1023.0 ) ) ; bG = bG * bG ; 
    min16float2 bB = nB + AH2_x ( min16float ( 1.0 / 1023.0 ) ) ; bB = bB * bB ; 
    min16float2 rR = ( cR - bR ) * APrxMedRcpH2 ( aR - bR ) ; 
    min16float2 rG = ( cG - bG ) * APrxMedRcpH2 ( aG - bG ) ; 
    min16float2 rB = ( cB - bB ) * APrxMedRcpH2 ( aB - bB ) ; 
    cR = ASatH2 ( nR + AGtZeroH2 ( dit - rR ) * AH2_x ( min16float ( 1.0 / 1023.0 ) ) ) ; 
    cG = ASatH2 ( nG + AGtZeroH2 ( dit - rG ) * AH2_x ( min16float ( 1.0 / 1023.0 ) ) ) ; 
    cB = ASatH2 ( nB + AGtZeroH2 ( dit - rB ) * AH2_x ( min16float ( 1.0 / 1023.0 ) ) ) ; 
} 


#line 1223


#line 31 "C:\\Users\\LiuXu\\source\\repos\\Magpie\\MODULE_Common\\FfxEasuShader.hlsl"
float4 main_Impl ( ) { 
    uint4 con0 , con1 , con2 , con3 ; 
    FsrEasuCon ( con0 , con1 , con2 , con3 , 1280 , 720 , 1280 , 720 , 1920 , 1080 ) ; 
    return float4 ( 1.0f , 1.0f , 1.0f , 1.0f ) ; 
}  