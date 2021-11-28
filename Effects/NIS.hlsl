//!MAGPIE EFFECT
//!VERSION 1


//!CONSTANT
//!VALUE INPUT_PT_X
float inputPtX;

//!CONSTANT
//!VALUE INPUT_PT_Y
float inputPtY;

//!CONSTANT
//!VALUE SCALE_X
float scaleX;

//!CONSTANT
//!VALUE SCALE_Y
float scaleY;

//!TEXTURE
Texture2D INPUT;

//!TEXTURE
//!WIDTH INPUT_WIDTH
//!HEIGHT INPUT_HEIGHT
//!FORMAT R16G16B16A16_FLOAT
Texture2D shEdgeMap;

//!SAMPLER
//!FILTER LINEAR
SamplerState sam;

//!SAMPLER
//!FILTER POINT
SamplerState sam1;

//!CONSTANT
//!DEFAULT 0.5
//!MIN 0
//!MAX 1
float sharpness;


//!COMMON

#define kDetectRatio (1127.f / 1024.f)
#define kDetectThres (64.0f / 1024.0f)
#define kPhaseCount 64
#define kFilterSize 8
#define kEps 1.0f
#define kMinContrastRatio 2.0f
#define kMaxContrastRatio 10.0f
#define kRatioNorm (1.0f / (kMaxContrastRatio - kMinContrastRatio))
#define kContrastBoost 1.0f
#define kSharpStartY 0.45f
#define kSharpEndY 0.9f
#define kSharpScaleY (1.0f / (kSharpEndY - kSharpStartY))
#define kSharpStrengthScale (kSharpStrengthMax - kSharpStrengthMin)
#define sharpen_slider (sharpness - 0.5f)
#define MinScale ((sharpen_slider >= 0.0f) ? 1.25f : 1.0f)
#define kSharpStrengthMin (max(0.0f, 0.4f + sharpen_slider * MinScale * 1.2f))
#define kSharpStrengthMax (1.6f + sharpen_slider * 1.8f)
#define LimitScale ((sharpen_slider >= 0.0f) ? 1.25f : 1.0f)
#define kSharpLimitMin (max(0.1f, 0.14f + sharpen_slider * LimitScale * 0.32f))
#define kSharpLimitMax (0.5f + sharpen_slider * LimitScale * 0.6f)
#define kSharpLimitScale (kSharpLimitMax - kSharpLimitMin)

#define NIS_SCALE_FLOAT 1.0f
#define NIS_SCALE_INT 1

const static float shCoefScaler[kPhaseCount][kFilterSize] = {
    {0.0,     0.0,    1.0000, 0.0,     0.0,    0.0, 0.0, 0.0},
    {0.0029, -0.0127, 1.0000, 0.0132, -0.0034, 0.0, 0.0, 0.0},
    {0.0063, -0.0249, 0.9985, 0.0269, -0.0068, 0.0, 0.0, 0.0},
    {0.0088, -0.0361, 0.9956, 0.0415, -0.0103, 0.0005, 0.0, 0.0},
    {0.0117, -0.0474, 0.9932, 0.0562, -0.0142, 0.0005, 0.0, 0.0},
    {0.0142, -0.0576, 0.9897, 0.0713, -0.0181, 0.0005, 0.0, 0.0},
    {0.0166, -0.0674, 0.9844, 0.0874, -0.0220, 0.0010, 0.0, 0.0},
    {0.0186, -0.0762, 0.9785, 0.1040, -0.0264, 0.0015, 0.0, 0.0},
    {0.0205, -0.0850, 0.9727, 0.1206, -0.0308, 0.0020, 0.0, 0.0},
    {0.0225, -0.0928, 0.9648, 0.1382, -0.0352, 0.0024, 0.0, 0.0},
    {0.0239, -0.1006, 0.9575, 0.1558, -0.0396, 0.0029, 0.0, 0.0},
    {0.0254, -0.1074, 0.9487, 0.1738, -0.0439, 0.0034, 0.0, 0.0},
    {0.0264, -0.1138, 0.9390, 0.1929, -0.0488, 0.0044, 0.0, 0.0},
    {0.0278, -0.1191, 0.9282, 0.2119, -0.0537, 0.0049, 0.0, 0.0},
    {0.0288, -0.1245, 0.9170, 0.2310, -0.0581, 0.0059, 0.0, 0.0},
    {0.0293, -0.1294, 0.9058, 0.2510, -0.0630, 0.0063, 0.0, 0.0},
    {0.0303, -0.1333, 0.8926, 0.2710, -0.0679, 0.0073, 0.0, 0.0},
    {0.0308, -0.1367, 0.8789, 0.2915, -0.0728, 0.0083, 0.0, 0.0},
    {0.0308, -0.1401, 0.8657, 0.3120, -0.0776, 0.0093, 0.0, 0.0},
    {0.0313, -0.1426, 0.8506, 0.3330, -0.0825, 0.0103, 0.0, 0.0},
    {0.0313, -0.1445, 0.8354, 0.3540, -0.0874, 0.0112, 0.0, 0.0},
    {0.0313, -0.1460, 0.8193, 0.3755, -0.0923, 0.0122, 0.0, 0.0},
    {0.0313, -0.1470, 0.8022, 0.3965, -0.0967, 0.0137, 0.0, 0.0},
    {0.0308, -0.1479, 0.7856, 0.4185, -0.1016, 0.0146, 0.0, 0.0},
    {0.0303, -0.1479, 0.7681, 0.4399, -0.1060, 0.0156, 0.0, 0.0},
    {0.0298, -0.1479, 0.7505, 0.4614, -0.1104, 0.0166, 0.0, 0.0},
    {0.0293, -0.1470, 0.7314, 0.4829, -0.1147, 0.0181, 0.0, 0.0},
    {0.0288, -0.1460, 0.7119, 0.5049, -0.1187, 0.0190, 0.0, 0.0},
    {0.0278, -0.1445, 0.6929, 0.5264, -0.1226, 0.0200, 0.0, 0.0},
    {0.0273, -0.1431, 0.6724, 0.5479, -0.1260, 0.0215, 0.0, 0.0},
    {0.0264, -0.1411, 0.6528, 0.5693, -0.1299, 0.0225, 0.0, 0.0},
    {0.0254, -0.1387, 0.6323, 0.5903, -0.1328, 0.0234, 0.0, 0.0},
    {0.0244, -0.1357, 0.6113, 0.6113, -0.1357, 0.0244, 0.0, 0.0},
    {0.0234, -0.1328, 0.5903, 0.6323, -0.1387, 0.0254, 0.0, 0.0},
    {0.0225, -0.1299, 0.5693, 0.6528, -0.1411, 0.0264, 0.0, 0.0},
    {0.0215, -0.1260, 0.5479, 0.6724, -0.1431, 0.0273, 0.0, 0.0},
    {0.0200, -0.1226, 0.5264, 0.6929, -0.1445, 0.0278, 0.0, 0.0},
    {0.0190, -0.1187, 0.5049, 0.7119, -0.1460, 0.0288, 0.0, 0.0},
    {0.0181, -0.1147, 0.4829, 0.7314, -0.1470, 0.0293, 0.0, 0.0},
    {0.0166, -0.1104, 0.4614, 0.7505, -0.1479, 0.0298, 0.0, 0.0},
    {0.0156, -0.1060, 0.4399, 0.7681, -0.1479, 0.0303, 0.0, 0.0},
    {0.0146, -0.1016, 0.4185, 0.7856, -0.1479, 0.0308, 0.0, 0.0},
    {0.0137, -0.0967, 0.3965, 0.8022, -0.1470, 0.0313, 0.0, 0.0},
    {0.0122, -0.0923, 0.3755, 0.8193, -0.1460, 0.0313, 0.0, 0.0},
    {0.0112, -0.0874, 0.3540, 0.8354, -0.1445, 0.0313, 0.0, 0.0},
    {0.0103, -0.0825, 0.3330, 0.8506, -0.1426, 0.0313, 0.0, 0.0},
    {0.0093, -0.0776, 0.3120, 0.8657, -0.1401, 0.0308, 0.0, 0.0},
    {0.0083, -0.0728, 0.2915, 0.8789, -0.1367, 0.0308, 0.0, 0.0},
    {0.0073, -0.0679, 0.2710, 0.8926, -0.1333, 0.0303, 0.0, 0.0},
    {0.0063, -0.0630, 0.2510, 0.9058, -0.1294, 0.0293, 0.0, 0.0},
    {0.0059, -0.0581, 0.2310, 0.9170, -0.1245, 0.0288, 0.0, 0.0},
    {0.0049, -0.0537, 0.2119, 0.9282, -0.1191, 0.0278, 0.0, 0.0},
    {0.0044, -0.0488, 0.1929, 0.9390, -0.1138, 0.0264, 0.0, 0.0},
    {0.0034, -0.0439, 0.1738, 0.9487, -0.1074, 0.0254, 0.0, 0.0},
    {0.0029, -0.0396, 0.1558, 0.9575, -0.1006, 0.0239, 0.0, 0.0},
    {0.0024, -0.0352, 0.1382, 0.9648, -0.0928, 0.0225, 0.0, 0.0},
    {0.0020, -0.0308, 0.1206, 0.9727, -0.0850, 0.0205, 0.0, 0.0},
    {0.0015, -0.0264, 0.1040, 0.9785, -0.0762, 0.0186, 0.0, 0.0},
    {0.0010, -0.0220, 0.0874, 0.9844, -0.0674, 0.0166, 0.0, 0.0},
    {0.0005, -0.0181, 0.0713, 0.9897, -0.0576, 0.0142, 0.0, 0.0},
    {0.0005, -0.0142, 0.0562, 0.9932, -0.0474, 0.0117, 0.0, 0.0},
    {0.0005, -0.0103, 0.0415, 0.9956, -0.0361, 0.0088, 0.0, 0.0},
    {0.0, -0.0068, 0.0269, 0.9985, -0.0249, 0.0063, 0.0, 0.0},
    {0.0, -0.0034, 0.0132, 1.0000, -0.0127, 0.0029, 0.0, 0.0}
};

const static float shCoefUSM[kPhaseCount][kFilterSize] = {
    {0,      -0.6001, 1.2002, -0.6001,  0,      0, 0, 0},
    {0.0029, -0.6084, 1.1987, -0.5903, -0.0029, 0, 0, 0},
    {0.0049, -0.6147, 1.1958, -0.5791, -0.0068, 0.0005, 0, 0},
    {0.0073, -0.6196, 1.1890, -0.5659, -0.0103, 0, 0, 0},
    {0.0093, -0.6235, 1.1802, -0.5513, -0.0151, 0, 0, 0},
    {0.0112, -0.6265, 1.1699, -0.5352, -0.0195, 0.0005, 0, 0},
    {0.0122, -0.6270, 1.1582, -0.5181, -0.0259, 0.0005, 0, 0},
    {0.0142, -0.6284, 1.1455, -0.5005, -0.0317, 0.0005, 0, 0},
    {0.0156, -0.6265, 1.1274, -0.4790, -0.0386, 0.0005, 0, 0},
    {0.0166, -0.6235, 1.1089, -0.4570, -0.0454, 0.0010, 0, 0},
    {0.0176, -0.6187, 1.0879, -0.4346, -0.0532, 0.0010, 0, 0},
    {0.0181, -0.6138, 1.0659, -0.4102, -0.0615, 0.0015, 0, 0},
    {0.0190, -0.6069, 1.0405, -0.3843, -0.0698, 0.0015, 0, 0},
    {0.0195, -0.6006, 1.0161, -0.3574, -0.0796, 0.0020, 0, 0},
    {0.0200, -0.5928, 0.9893, -0.3286, -0.0898, 0.0024, 0, 0},
    {0.0200, -0.5820, 0.9580, -0.2988, -0.1001, 0.0029, 0, 0},
    {0.0200, -0.5728, 0.9292, -0.2690, -0.1104, 0.0034, 0, 0},
    {0.0200, -0.5620, 0.8975, -0.2368, -0.1226, 0.0039, 0, 0},
    {0.0205, -0.5498, 0.8643, -0.2046, -0.1343, 0.0044, 0, 0},
    {0.0200, -0.5371, 0.8301, -0.1709, -0.1465, 0.0049, 0, 0},
    {0.0195, -0.5239, 0.7944, -0.1367, -0.1587, 0.0054, 0, 0},
    {0.0195, -0.5107, 0.7598, -0.1021, -0.1724, 0.0059, 0, 0},
    {0.0190, -0.4966, 0.7231, -0.0649, -0.1865, 0.0063, 0, 0},
    {0.0186, -0.4819, 0.6846, -0.0288, -0.1997, 0.0068, 0, 0},
    {0.0186, -0.4668, 0.6460, 0.0093, -0.2144, 0.0073, 0, 0},
    {0.0176, -0.4507, 0.6055, 0.0479, -0.2290, 0.0083, 0, 0},
    {0.0171, -0.4370, 0.5693, 0.0859, -0.2446, 0.0088, 0, 0},
    {0.0161, -0.4199, 0.5283, 0.1255, -0.2598, 0.0098, 0, 0},
    {0.0161, -0.4048, 0.4883, 0.1655, -0.2754, 0.0103, 0, 0},
    {0.0151, -0.3887, 0.4497, 0.2041, -0.2910, 0.0107, 0, 0},
    {0.0142, -0.3711, 0.4072, 0.2446, -0.3066, 0.0117, 0, 0},
    {0.0137, -0.3555, 0.3672, 0.2852, -0.3228, 0.0122, 0, 0},
    {0.0132, -0.3394, 0.3262, 0.3262, -0.3394, 0.0132, 0, 0},
    {0.0122, -0.3228, 0.2852, 0.3672, -0.3555, 0.0137, 0, 0},
    {0.0117, -0.3066, 0.2446, 0.4072, -0.3711, 0.0142, 0, 0},
    {0.0107, -0.2910, 0.2041, 0.4497, -0.3887, 0.0151, 0, 0},
    {0.0103, -0.2754, 0.1655, 0.4883, -0.4048, 0.0161, 0, 0},
    {0.0098, -0.2598, 0.1255, 0.5283, -0.4199, 0.0161, 0, 0},
    {0.0088, -0.2446, 0.0859, 0.5693, -0.4370, 0.0171, 0, 0},
    {0.0083, -0.2290, 0.0479, 0.6055, -0.4507, 0.0176, 0, 0},
    {0.0073, -0.2144, 0.0093, 0.6460, -0.4668, 0.0186, 0, 0},
    {0.0068, -0.1997, -0.0288, 0.6846, -0.4819, 0.0186, 0, 0},
    {0.0063, -0.1865, -0.0649, 0.7231, -0.4966, 0.0190, 0, 0},
    {0.0059, -0.1724, -0.1021, 0.7598, -0.5107, 0.0195, 0, 0},
    {0.0054, -0.1587, -0.1367, 0.7944, -0.5239, 0.0195, 0, 0},
    {0.0049, -0.1465, -0.1709, 0.8301, -0.5371, 0.0200, 0, 0},
    {0.0044, -0.1343, -0.2046, 0.8643, -0.5498, 0.0205, 0, 0},
    {0.0039, -0.1226, -0.2368, 0.8975, -0.5620, 0.0200, 0, 0},
    {0.0034, -0.1104, -0.2690, 0.9292, -0.5728, 0.0200, 0, 0},
    {0.0029, -0.1001, -0.2988, 0.9580, -0.5820, 0.0200, 0, 0},
    {0.0024, -0.0898, -0.3286, 0.9893, -0.5928, 0.0200, 0, 0},
    {0.0020, -0.0796, -0.3574, 1.0161, -0.6006, 0.0195, 0, 0},
    {0.0015, -0.0698, -0.3843, 1.0405, -0.6069, 0.0190, 0, 0},
    {0.0015, -0.0615, -0.4102, 1.0659, -0.6138, 0.0181, 0, 0},
    {0.0010, -0.0532, -0.4346, 1.0879, -0.6187, 0.0176, 0, 0},
    {0.0010, -0.0454, -0.4570, 1.1089, -0.6235, 0.0166, 0, 0},
    {0.0005, -0.0386, -0.4790, 1.1274, -0.6265, 0.0156, 0, 0},
    {0.0005, -0.0317, -0.5005, 1.1455, -0.6284, 0.0142, 0, 0},
    {0.0005, -0.0259, -0.5181, 1.1582, -0.6270, 0.0122, 0, 0},
    {0.0005, -0.0195, -0.5352, 1.1699, -0.6265, 0.0112, 0, 0},
    {0, -0.0151, -0.5513, 1.1802, -0.6235, 0.0093, 0, 0},
    {0, -0.0103, -0.5659, 1.1890, -0.6196, 0.0073, 0, 0},
    {0.0005, -0.0068, -0.5791, 1.1958, -0.6147, 0.0049, 0, 0},
    {0, -0.0029, -0.5903, 1.1987, -0.6084, 0.0029, 0, 0}
};



typedef float4 NVF4;
typedef float NVF;


float getY(float3 rgba) {
    return 0.2126f * rgba.x + 0.7152f * rgba.y + 0.0722f * rgba.z;
}


float4 GetEdgeMap(float p[3][3]) {
    const float g_0 = abs(p[0][0] + p[0][1] + p[0][2] - p[2][0] - p[2][1] - p[2][2]);
    const float g_45 = abs(p[1][0] + p[0][0] + p[0][1] - p[2][1] - p[2][2] - p[1][2]);
    const float g_90 = abs(p[0][0] + p[1][0] + p[2][0] - p[0][2] - p[1][2] - p[2][2]);
    const float g_135 = abs(p[1][0] + p[2][0] + p[2][1] - p[0][1] - p[0][2] - p[1][2]);

    const float g_0_90_max = max(g_0, g_90);
    const float g_0_90_min = min(g_0, g_90);
    const float g_45_135_max = max(g_45, g_135);
    const float g_45_135_min = min(g_45, g_135);

    float e_0_90 = 0;
    float e_45_135 = 0;

    float edge_0 = 0;
    float edge_45 = 0;
    float edge_90 = 0;
    float edge_135 = 0;

    if ((g_0_90_max + g_45_135_max) == 0) {
        e_0_90 = 0;
        e_45_135 = 0;
    } else {
        e_0_90 = g_0_90_max / (g_0_90_max + g_45_135_max);
        e_0_90 = min(e_0_90, 1.0f);
        e_45_135 = 1.0f - e_0_90;
    }

    if ((g_0_90_max > (g_0_90_min * kDetectRatio)) && (g_0_90_max > kDetectThres) && (g_0_90_max > g_45_135_min)) {
        if (g_0_90_max == g_0) {
            edge_0 = 1.0f;
            edge_90 = 0;
        } else {
            edge_0 = 0;
            edge_90 = 1.0f;
        }
    } else {
        edge_0 = 0;
        edge_90 = 0;
    }

    if ((g_45_135_max > (g_45_135_min * kDetectRatio)) && (g_45_135_max > kDetectThres) &&
        (g_45_135_max > g_0_90_min)) {

        if (g_45_135_max == g_45) {
            edge_45 = 1.0f;
            edge_135 = 0;
        } else {
            edge_45 = 0;
            edge_135 = 1.0f;
        }
    } else {
        edge_45 = 0;
        edge_135 = 0;
    }

    float weight_0, weight_90, weight_45, weight_135;
    if ((edge_0 + edge_90 + edge_45 + edge_135) >= 2.0f) {
        if (edge_0 == 1.0f) {
            weight_0 = e_0_90;
            weight_90 = 0;
        } else {
            weight_0 = 0;
            weight_90 = e_0_90;
        }

        if (edge_45 == 1.0f) {
            weight_45 = e_45_135;
            weight_135 = 0;
        } else {
            weight_45 = 0;
            weight_135 = e_45_135;
        }
    } else if ((edge_0 + edge_90 + edge_45 + edge_135) >= 1.0f) {
        weight_0 = edge_0;
        weight_90 = edge_90;
        weight_45 = edge_45;
        weight_135 = edge_135;
    } else {
        weight_0 = 0;
        weight_90 = 0;
        weight_45 = 0;
        weight_135 = 0;
    }

    return float4(weight_0, weight_90, weight_45, weight_135);
}

float CalcLTI(float p0, float p1, float p2, float p3, float p4, float p5, int phase_index) {
    float y0, y1, y2, y3, y4;

    if (phase_index <= kPhaseCount / 2) {
        y0 = p0;
        y1 = p1;
        y2 = p2;
        y3 = p3;
        y4 = p4;
    } else {
        y0 = p1;
        y1 = p2;
        y2 = p3;
        y3 = p4;
        y4 = p5;
    }

    const float a_min = min(min(y0, y1), y2);
    const float a_max = max(max(y0, y1), y2);

    const float b_min = min(min(y2, y3), y4);
    const float b_max = max(max(y2, y3), y4);

    const float a_cont = a_max - a_min;
    const float b_cont = b_max - b_min;

    const float cont_ratio = max(a_cont, b_cont) / (min(a_cont, b_cont) + kEps);
    return (1.0f - saturate((cont_ratio - kMinContrastRatio) * kRatioNorm)) * kContrastBoost;
}


float4 GetInterpEdgeMap(const float4 edge[2][2], float phase_frac_x, float phase_frac_y) {
    float4 h0, h1, f;

    h0.x = lerp(edge[0][0].x, edge[0][1].x, phase_frac_x);
    h0.y = lerp(edge[0][0].y, edge[0][1].y, phase_frac_x);
    h0.z = lerp(edge[0][0].z, edge[0][1].z, phase_frac_x);
    h0.w = lerp(edge[0][0].w, edge[0][1].w, phase_frac_x);

    h1.x = lerp(edge[1][0].x, edge[1][1].x, phase_frac_x);
    h1.y = lerp(edge[1][0].y, edge[1][1].y, phase_frac_x);
    h1.z = lerp(edge[1][0].z, edge[1][1].z, phase_frac_x);
    h1.w = lerp(edge[1][0].w, edge[1][1].w, phase_frac_x);

    f.x = lerp(h0.x, h1.x, phase_frac_y);
    f.y = lerp(h0.y, h1.y, phase_frac_y);
    f.z = lerp(h0.z, h1.z, phase_frac_y);
    f.w = lerp(h0.w, h1.w, phase_frac_y);

    return f;
}

float EvalPoly6(const float pxl[6], int phase_int) {
    float y = 0.f;
    {
        [unroll]
        for (int i = 0; i < 6; ++i) {
            y += shCoefScaler[phase_int][i] * pxl[i];
        }
    }
    float y_usm = 0.f;
    {
        [unroll]
        for (int i = 0; i < 6; ++i) {
            y_usm += shCoefUSM[phase_int][i] * pxl[i];
        }
    }

    // let's compute a piece-wise ramp based on luma
    const float y_scale = 1.0f - saturate((y * (1.0f / 255) - kSharpStartY) * kSharpScaleY);

    // scale the ramp to sharpen as a function of luma
    const float y_sharpness = y_scale * kSharpStrengthScale + kSharpStrengthMin;

    y_usm *= y_sharpness;

    // scale the ramp to limit USM as a function of luma
    const float y_sharpness_limit = (y_scale * kSharpLimitScale + kSharpLimitMin) * y;

    y_usm = min(y_sharpness_limit, max(-y_sharpness_limit, y_usm));
    // reduce ringing
    y_usm *= CalcLTI(pxl[0], pxl[1], pxl[2], pxl[3], pxl[4], pxl[5], phase_int);

    return y + y_usm;
}

float FilterNormal(const float p[6][6], int phase_x_frac_int, int phase_y_frac_int) {
    float h_acc = 0.0f;
    [unroll]
    for (int j = 0; j < 6; ++j) {
        float v_acc = 0.0f;
        [unroll]
        for (int i = 0; i < 6; ++i) {
            v_acc += p[i][j] * shCoefScaler[phase_y_frac_int][i];
        }
        h_acc += v_acc * shCoefScaler[phase_x_frac_int][j];
    }

    // let's return the sum unpacked -> we can accumulate it later
    return h_acc;
}

float4 GetDirFilters(float p[6][6], float phase_x_frac, float phase_y_frac, int phase_x_frac_int, int phase_y_frac_int) {
    float4 f;
    // 0 deg filter
    float interp0Deg[6];
    {
        [unroll]
        for (int i = 0; i < 6; ++i) {
            interp0Deg[i] = lerp(p[i][2], p[i][3], phase_x_frac);
        }
    }

    f.x = EvalPoly6(interp0Deg, phase_y_frac_int);

    // 90 deg filter
    float interp90Deg[6];
    {
        [unroll]
        for (int i = 0; i < 6; ++i) {
            interp90Deg[i] = lerp(p[2][i], p[3][i], phase_y_frac);
        }
    }

    f.y = EvalPoly6(interp90Deg, phase_x_frac_int);

    //45 deg filter
    float pphase_b45;
    pphase_b45 = 0.5f + 0.5f * (phase_x_frac - phase_y_frac);

    float temp_interp45Deg[7];
    temp_interp45Deg[1] = lerp(p[2][1], p[1][2], pphase_b45);
    temp_interp45Deg[3] = lerp(p[3][2], p[2][3], pphase_b45);
    temp_interp45Deg[5] = lerp(p[4][3], p[3][4], pphase_b45);

    if (pphase_b45 >= 0.5f) {
        pphase_b45 = pphase_b45 - 0.5f;

        temp_interp45Deg[0] = lerp(p[1][1], p[0][2], pphase_b45);
        temp_interp45Deg[2] = lerp(p[2][2], p[1][3], pphase_b45);
        temp_interp45Deg[4] = lerp(p[3][3], p[2][4], pphase_b45);
        temp_interp45Deg[6] = lerp(p[4][4], p[3][5], pphase_b45);
    } else {
        pphase_b45 = 0.5f - pphase_b45;

        temp_interp45Deg[0] = lerp(p[1][1], p[2][0], pphase_b45);
        temp_interp45Deg[2] = lerp(p[2][2], p[3][1], pphase_b45);
        temp_interp45Deg[4] = lerp(p[3][3], p[4][2], pphase_b45);
        temp_interp45Deg[6] = lerp(p[4][4], p[5][3], pphase_b45);
    }

    float interp45Deg[6];
    float pphase_p45 = phase_x_frac + phase_y_frac;
    if (pphase_p45 >= 1) {
        [unroll]
        for (int i = 0; i < 6; i++) {
            interp45Deg[i] = temp_interp45Deg[i + 1];
        }
        pphase_p45 = pphase_p45 - 1;
    } else {
        [unroll]
        for (int i = 0; i < 6; i++) {
            interp45Deg[i] = temp_interp45Deg[i];
        }
    }

    f.z = EvalPoly6(interp45Deg, (int)(pphase_p45 * 64));

    //135 deg filter
    float pphase_b135;
    pphase_b135 = 0.5f * (phase_x_frac + phase_y_frac);

    float temp_interp135Deg[7];

    temp_interp135Deg[1] = lerp(p[3][1], p[4][2], pphase_b135);
    temp_interp135Deg[3] = lerp(p[2][2], p[3][3], pphase_b135);
    temp_interp135Deg[5] = lerp(p[1][3], p[2][4], pphase_b135);

    if (pphase_b135 >= 0.5f) {
        pphase_b135 = pphase_b135 - 0.5f;

        temp_interp135Deg[0] = lerp(p[4][1], p[5][2], pphase_b135);
        temp_interp135Deg[2] = lerp(p[3][2], p[4][3], pphase_b135);
        temp_interp135Deg[4] = lerp(p[2][3], p[3][4], pphase_b135);
        temp_interp135Deg[6] = lerp(p[1][4], p[2][5], pphase_b135);
    } else {
        pphase_b135 = 0.5f - pphase_b135;

        temp_interp135Deg[0] = lerp(p[4][1], p[3][0], pphase_b135);
        temp_interp135Deg[2] = lerp(p[3][2], p[2][1], pphase_b135);
        temp_interp135Deg[4] = lerp(p[2][3], p[1][2], pphase_b135);
        temp_interp135Deg[6] = lerp(p[1][4], p[0][3], pphase_b135);
    }

    float interp135Deg[6];
    float pphase_p135 = 1 + (phase_x_frac - phase_y_frac);
    if (pphase_p135 >= 1) {
        [unroll]
        for (int i = 0; i < 6; ++i) {
            interp135Deg[i] = temp_interp135Deg[i + 1];
        }
        pphase_p135 = pphase_p135 - 1;
    } else {
        [unroll]
        for (int i = 0; i < 6; ++i) {
            interp135Deg[i] = temp_interp135Deg[i];
        }
    }

    f.w = EvalPoly6(interp135Deg, (int)(pphase_p135 * 64));
    return f;
}


//!PASS 1
//!BIND INPUT
//!SAVE shEdgeMap

float4 Pass1(float2 pos) {
    float p[3][3];

    [unroll]
    for (int j = 0; j < 3; j++) {
        [unroll]
        for (int k = 0; k < 3; k++) {
            const float3 px = INPUT.Sample(sam1, pos + float2(k - 1, j - 1) * float2(inputPtX, inputPtY)).xyz;
            p[j][k] = getY(px);
        }
    }

    return GetEdgeMap(p);
}


//!PASS 2
//!BIND INPUT, shEdgeMap

float4 Pass2(float2 pos) {
    float2 srcPos = pos / float2(inputPtX, inputPtY) - 0.5f;
    float2 srcPosB = floor(srcPos);

    // load 6x6 support to regs
    float p[6][6];
    {
        [unroll]
        for (int i = 0; i < 6; ++i) {
            [unroll]
            for (int j = 0; j < 6; ++j) {
                p[i][j] = getY(INPUT.Sample(sam, (srcPosB + float2(j - 2, i - 2)) * float2(inputPtX, inputPtY)).rgb);
            }
        }
    }

    // compute discretized filter phase
    const float2 f = srcPos - srcPosB;
    const int fx_int = (int)(f.x * kPhaseCount);
    const int fy_int = (int)(f.y * kPhaseCount);
    
    // get traditional scaler filter output
    const float pixel_n = FilterNormal(p, fx_int, fy_int);
    
    // get directional filter bank output
    float4 opDirYU = GetDirFilters(p, f.x, f.y, fx_int, fy_int);

    // final luma is a weighted product of directional & normal filters

    // generate weights for directional filters
    float4 edge[2][2];
    [unroll]
    for (int i = 0; i < 2; i++) {
        [unroll]
        for (int j = 0; j < 2; j++) {
            // need to shift edge map sampling since it's a 2x2 centered inside 6x6 grid                
            edge[i][j] = shEdgeMap.Sample(sam1, (srcPosB + float2(j, i)) * float2(inputPtX, inputPtY));
        }
    }
    const float4 w = GetInterpEdgeMap(edge, f.x, f.y) * NIS_SCALE_INT;
    
    // final pixel is a weighted sum filter outputs
    const float opY = (opDirYU.x * w.x + opDirYU.y * w.y + opDirYU.z * w.z + opDirYU.w * w.w +
        pixel_n * (NIS_SCALE_FLOAT - w.x - w.y - w.z - w.w)) * (1.0f / NIS_SCALE_FLOAT);
    // do bilinear tap for chroma upscaling
    
    float4 op = INPUT.Sample(sam, pos);
    
    const float corr = opY * (1.0f / NIS_SCALE_FLOAT) - getY(float3(op.x, op.y, op.z));
    op.x += corr;
    op.y += corr;
    op.z += corr;

    return op;
}
