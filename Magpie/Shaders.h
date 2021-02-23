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
constexpr auto MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x1_SHADER = L"shaders/Anime4KUpscaleConv4x3x3x1Shader.cso";
constexpr auto MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER1 = L"shaders/Anime4KUpscaleConv4x3x3x8Shader1.cso";
constexpr auto MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER2 = L"shaders/Anime4KUpscaleConv4x3x3x8Shader2.cso";
constexpr auto MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER3 = L"shaders/Anime4KUpscaleConv4x3x3x8Shader3.cso";
constexpr auto MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER4 = L"shaders/Anime4KUpscaleConv4x3x3x8Shader4.cso";
constexpr auto MAGPIE_ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER5 = L"shaders/Anime4KUpscaleConv4x3x3x8Shader5.cso";
constexpr auto MAGPIE_ANIME4K_UPSCALE_CONV_REDUCE_SHADER = L"shaders/Anime4KUpscaleConvReduceShader.cso";
constexpr auto MAGPIE_ANIME4K_UPSCALE_OUTPUT_SHADER = L"shaders/Anime4KUpscaleOutputShader.cso";
constexpr auto MAGPIE_ANIME4K_DEBLUR_KERNEL_SHADER = L"shaders/Anime4KDeblurKernelShader.cso";
constexpr auto MAGPIE_ANIME4K_UPSCALE_DEBLUR_OUTPUT_SHADER = L"shaders/Anime4KUpscaleDeblurOutputShader.cso";

/*
* 缩放相关
*/
// Lanczos6
constexpr auto MAGPIE_LANCZOS6_SCALE_SHADER = L"shaders/Lanczos6ScaleShader.cso";
// Jinc2
constexpr auto MAGPIE_JINC2_SCALE_SHADER = L"shaders/Jinc2ScaleShader.cso";
// Mitchell-Netravali
constexpr auto MAGPIE_MITCHELL_NETRAVALI_SCALE_SHADER = L"shaders/MitchellNetravaliScaleShader.cso";
