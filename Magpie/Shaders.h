#pragma once

/*
* 锐化相关
*/
// Adaptive sharpen
constexpr auto ADAPTIVE_SHARPEN_PASS1_SHADER = L"shaders/AdaptiveSharpenPass1Shader.cso";
constexpr auto ADAPTIVE_SHARPEN_PASS2_SHADER = L"shaders/AdaptiveSharpenPass2Shader.cso";


/*
* Anime4K 相关
*/
constexpr auto ANIME4K_UPSCALE_CONV_4x3x3x1_SHADER = L"shaders/Anime4KUpscaleConv4x3x3x1Shader.cso";
constexpr auto ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER1 = L"shaders/Anime4KUpscaleConv4x3x3x8Shader1.cso";
constexpr auto ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER2 = L"shaders/Anime4KUpscaleConv4x3x3x8Shader2.cso";
constexpr auto ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER3 = L"shaders/Anime4KUpscaleConv4x3x3x8Shader3.cso";
constexpr auto ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER4 = L"shaders/Anime4KUpscaleConv4x3x3x8Shader4.cso";
constexpr auto ANIME4K_UPSCALE_CONV_4x3x3x8_SHADER5 = L"shaders/Anime4KUpscaleConv4x3x3x8Shader5.cso";
constexpr auto ANIME4K_UPSCALE_CONV_REDUCE_SHADER = L"shaders/Anime4KUpscaleConvReduceShader.cso";
constexpr auto ANIME4K_UPSCALE_OUTPUT_SHADER = L"shaders/Anime4KUpscaleOutputShader.cso";
constexpr auto ANIME4K_DEBLUR_KERNEL_X_SHADER = L"shaders/Anime4KDeblurKernelXShader.cso";
constexpr auto ANIME4K_DEBLUR_KERNEL_Y_SHADER = L"shaders/Anime4KDeblurKernelYShader.cso";
constexpr auto ANIME4K_UPSCALE_DEBLUR_OUTPUT_SHADER = L"shaders/Anime4KUpscaleDeblurOutputShader.cso";

/*
* 缩放相关
*/
// Lanczos6
constexpr auto LANCZOS6_SCALE_SHADER = L"shaders/Lanczos6ScaleShader.cso";
// Jinc2
constexpr auto JINC2_SCALE_SHADER = L"shaders/Jinc2ScaleShader.cso";
// Mitchell-Netravali
constexpr auto MITCHELL_NETRAVALI_SCALE_SHADER = L"shaders/MitchellNetravaliScaleShader.cso";
