#pragma once

/*
* 锐化相关
*/
// Adaptive sharpen
constexpr auto MAGPIE_ADAPTIVE_SHARPEN_PASS1_SHADER = L"shaders/AdaptiveSharpenPass1Shader.cso";
constexpr auto MAGPIE_ADAPTIVE_SHARPEN_PASS2_SHADER = L"shaders/AdaptiveSharpenPass2Shader.cso";


/*
* Anime4K 相关
*/
constexpr auto MAGPIE_ANIME4K_CONV_4x3x3x1_SHADER = L"shaders/Anime4KConv4x3x3x1Shader.cso";
constexpr auto MAGPIE_ANIME4K_CONV_4x3x3x8_PASS1_SHADER = L"shaders/Anime4KConv4x3x3x8Pass1Shader.cso";
constexpr auto MAGPIE_ANIME4K_CONV_4x3x3x8_PASS2_SHADER = L"shaders/Anime4KConv4x3x3x8Pass2Shader.cso";
constexpr auto MAGPIE_ANIME4K_CONV_4x3x3x8_PASS3_SHADER = L"shaders/Anime4KConv4x3x3x8Pass3Shader.cso";
constexpr auto MAGPIE_ANIME4K_CONV_4x3x3x8_PASS4_SHADER = L"shaders/Anime4KConv4x3x3x8Pass4Shader.cso";
constexpr auto MAGPIE_ANIME4K_CONV_4x3x3x8_PASS5_SHADER = L"shaders/Anime4KConv4x3x3x8Pass5Shader.cso";
constexpr auto MAGPIE_ANIME4K_CONV_REDUCE_SHADER = L"shaders/Anime4KConvReduceShader.cso";
constexpr auto MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x1_SHADER = L"shaders/Anime4KDenoiseConv4x3x3x1Shader.cso";
constexpr auto MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS1_SHADER = L"shaders/Anime4KDenoiseConv4x3x3x8Pass1Shader.cso";
constexpr auto MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS2_SHADER = L"shaders/Anime4KDenoiseConv4x3x3x8Pass2Shader.cso";
constexpr auto MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS3_SHADER = L"shaders/Anime4KDenoiseConv4x3x3x8Pass3Shader.cso";
constexpr auto MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS4_SHADER = L"shaders/Anime4KDenoiseConv4x3x3x8Pass4Shader.cso";
constexpr auto MAGPIE_ANIME4K_DENOISE_CONV_4x3x3x8_PASS5_SHADER = L"shaders/Anime4KDenoiseConv4x3x3x8Pass5Shader.cso";
constexpr auto MAGPIE_ANIME4K_DENOISE_CONV_REDUCE_SHADER = L"shaders/Anime4KDenoiseConvReduceShader.cso";
constexpr auto MAGPIE_ANIME4K_SHARPEN_COMBINE_SHADER = L"shaders/Anime4KSharpenCombineShader.cso";
constexpr auto MAGPIE_ANIME4K_DARKLINES_PASS1_SHADER = L"shaders/Anime4KDarkLinesPass1Shader.cso";
constexpr auto MAGPIE_ANIME4K_DARKLINES_PASS2_SHADER = L"shaders/Anime4KDarkLinesPass2Shader.cso";
constexpr auto MAGPIE_ANIME4K_DARKLINES_PASS3_SHADER = L"shaders/Anime4KDarkLinesPass3Shader.cso";
constexpr auto MAGPIE_ANIME4K_DARKLINES_PASS4_SHADER = L"shaders/Anime4KDarkLinesPass4Shader.cso";
constexpr auto MAGPIE_ANIME4K_DARKLINES_PASS5_SHADER = L"shaders/Anime4KDarkLinesPass5Shader.cso";
constexpr auto MAGPIE_ANIME4K_THINLINES_PASS1_SHADER = L"shaders/Anime4KThinLinesPass1Shader.cso";
constexpr auto MAGPIE_ANIME4K_THINLINES_PASS2_SHADER = L"shaders/Anime4KThinLinesPass2Shader.cso";
constexpr auto MAGPIE_ANIME4K_THINLINES_PASS3_SHADER = L"shaders/Anime4KThinLinesPass3Shader.cso";
constexpr auto MAGPIE_ANIME4K_THINLINES_PASS4_SHADER = L"shaders/Anime4KThinLinesPass4Shader.cso";
constexpr auto MAGPIE_ANIME4K_THINLINES_PASS5_SHADER = L"shaders/Anime4KThinLinesPass5Shader.cso";
constexpr auto MAGPIE_ANIME4K_THINLINES_PASS6_SHADER = L"shaders/Anime4KThinLinesPass6Shader.cso";
constexpr auto MAGPIE_ANIME4K_THINLINES_PASS7_SHADER = L"shaders/Anime4KThinLinesPass7Shader.cso";
constexpr auto MAGPIE_RGB2YUV_SHADER = L"shaders/RGB2YUVShader.cso";


/*
* 缩放相关
*/
// Lanczos3
constexpr auto MAGPIE_LANCZOS6_SCALE_SHADER = L"shaders/Lanczos6ScaleShader.cso";
// Jinc2
constexpr auto MAGPIE_JINC2_SCALE_SHADER = L"shaders/Jinc2ScaleShader.cso";
// Mitchell-Netravali
constexpr auto MAGPIE_MITCHELL_NETRAVALI_SCALE_SHADER = L"shaders/MitchellNetravaliScaleShader.cso";
// Pixel
constexpr auto MAGPIE_PIXEL_SCALE_SHADER = L"shaders/PixelScaleShader.cso";
