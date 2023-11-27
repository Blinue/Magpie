Magpie ships with a handful of effects that can be used in combinations. Most of the effects have parameters that can be customized. All effects are stored in the `effects` folder. You can easily add effects if you are familiar with HLSL. Check the [MagpieFX](https://github.com/Blinue/Magpie/wiki/MagpieFX%20(EN)) page.

## Introduction to shipped effects

* ACNet: Port of [ACNetGLSL](https://github.com/TianZerL/ACNetGLSL). Suitable for anime-style images. Strong denoise effects.
  * Output size: twice that of the input

* AdaptiveSharpen: Self-adaptive sharpening algorithm. This algorithm focuses on sharpening the blurry edges in the images. Hence it has less image noise, ringing artifacts, and stripes compared to the common sharpening algorithms.
  * Output size: the same as the input
  * Parameter
    * Sharpness

* Anime4K_3D_AA_Upscale_US and Anime4K_3D_Upscale_US: 3D game scaling algorithms provided by Anime4K. The AA variant supports anti-aliasing.
  * Output size: twice that of the input

* Anime4K_Denoise_Bilateral_Mean, Anime4K_Denoise_Bilateral_Median, and Anime4K_Denoise_Bilateral_Mode: Denoise algorithms provided by Anime4K. Uses mean, median, and mode, respectively.
  * Output size: the same as the input
  * Parameter:
    * Strength: Denoise magnitude

* Anime4K_Restore_S, Anime4K_Restore_M, Anime4K_Restore_L, Anime4K_Restore_VL, Anime4K_Restore_UL, Anime4K_Restore_Soft_S, Anime4K_Restore_Soft_M, Anime4K_Restore_Soft_L, Anime4K_Restore_Soft_VL, Anime4K_Restore_Soft_UL: Algorithms to restore the lines in animations. In increasing order of demand for computing power. The Soft variants are more conservative in sharpening.
  * Output size: the same as the input

* Anime4K_Thin_HQ: Algorithm to clarify lines in animations provided by Anime4K.
  * Output size: the same as the input
  * Parameters
    * Strength: The strength in each iteration.
    * Iterations: The number of iterations. Decreasing strength and increasing iterations improves the quality of the images, but will lower the processing speed.

* Anime4K_Upscale_S, Anime4K_Upscale_L, Anime4K_Upscale_Denoise_S, Anime4K_Upscale_Denoise_L, and Anime4K_Upscale_GAN_x2_S: Anime-style scaling algorithms provided by Anime4K. The denoise variant includes denoise functionality. The GAN variant, which keeps more details, is still under experiment.
  * Output size: twice that of the input

* Bicubic: Interpolation algorithms. The lite variant is fast, but at the cost of quality degradation, Suitable for users will weak graphics cards.
  * Output size: determined by scale configuration
  * Parameters
    * B: Too large values will result in blurry images.
    * C: Too large values will result in ring artifacts.
  * Note: Different combinations of parameters will lead to different variants of the algorithm. For example: Mitchell(B=C≈0.333333), Catmull-Rom(B=0 C=0.5), bicubic Photoshop(B=0 C=0.75), Spline(B=1 C=0)

* Bilinear：Bilinear interpolation
  * Output size: determined by scale configuration

* CAS: Port of [FidelityFX-CAS](https://github.com/GPUOpen-Effects/FidelityFX-CAS). Lightweight sharpening effects.
  * Output size: the same as the input
  * Parameter
    * Sharpness

* CAS_Scaling：Port [FidelityFX-CAS](https://github.com/GPUOpen-Effects/FidelityFX-CAS). Supports scaling
  * Output size: the same as the input
  * Parameter
    * Sharpness

* CRT_Easymode: Easy-to-configure lightweight CRT shader.
  * Output size: determined by scale configuration
  * Parameters
    * Sharpness Horizontal
    * Sharpness Vertical
    * Mask Strength
    * Mask Dot Width
    * Mask Dot Height
    * Mask Stagger
    * Mask Size
    * Scanline Strength
    * Scanline Beam Width Min
    * Scanline Beam Width Max
    * Scanline Brightness Min
    * Scanline Brightness Max
    * Scanline Cutoff
    * Gamma Input
    * Gamma Output
    * Brightness Boost
    * Dilation

* CRT_Geom: One of the most popular CRT shaders. Aims to emulate arcade machines. Check [Emulation General Wiki](https://emulation.gametechwiki.com/index.php/CRT_Geom).
  * Output size: determined by scale configuration
  * Parameters
    * Target Gamma
    * Monitor Gamma
    * Distance
    * Curvature: Whether to emulate the screen curvature
    * Curvature Radius
    * Corner Size
    * Corner Smoothness
    * Horizontal Tilt
    * Vertical Tilt
    * Horizontal Overscan
    * Vertical Overscan
    * Dot Mask
    * Sharpness: The larger the value is, the clear the image becomes
    * Scanline Weight
    * Luminance Boost
    * Interlacing: Whether to emulate interlacing

* CRT_Hyllian: Provides sharp and clear outputs with slight rims. Similar to Sony BVM displays.
  * Output size: determined by scale configuration
  * Parameters
    * Phosphor
    * Vertical Scanlines
    * Input Gamma
    * Output Gamma
    * Sharpness
    * Color Boost
    * Red Boost
    * Green Boost
    * Blue Boost
    * Scanline Strength
    * Min Beam Width
    * Max Beam Width
    * Anti-Ringing

* CRT_Lottes: Provides multiple masks emulating Bloom and Halation effects. Similar to CGA arcade displays.
  * Output size: determined by scale configuration
  * Parameters
    * Scanline Hardness
    * Pixel Hardness
    * Horizontal Display Warp
    * Vertical Display Warp
    * Mask Dark
    * Mask Light
    * Shadow Mask
    * Brightness Boost
    * Bloom-X Soft
    * Bloom-Y Soft
    * Bloom Amount
    * Filter Kernel Shape

* Deband
  * Output size: the same as the input
  * Parameters
    * Threshold：The threshold of difference below which a pixel is considered to be part of a gradient.
    * Range：The range (in source pixels) at which to sample for neighbours.
    * Iterations：The number of debanding iterations to perform. Each iteration samples from random positions, so increasing the number of iterations is likely to increase the debanding quality. Conversely, it slows the shader down.
    * Grain：Add some extra noise to the image. This significantly helps cover up remaining banding and blocking artifacts, at comparatively little visual quality.

* FineSharp: High-quality sharpening. The earliest version was the AviSynth script.
  * Output size: the same as the input
  * Parameters
    * sstr: Sharpening strength. If you change this parameter, you will have to also change cstr. Check the note below.
    * cstr: Balancing strength.
    * xstr: The sharpening strength in the final step of the XSharpen style.
    * xrep: To fix the artifacts in the last step of sharpening.
  * Note: mapping between sstr and cstr (sstr->cstr): 0->0, 0.5->0.1, 1.0->0.6, 2.0->0.9, 2.5->1.00, 3.0->1.09, 3.5->1.15, 4.0->1.19, 8.0->1.249, 255.0->1.5

* FSR_EASU: Port of the scaling step in [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR). The DX10 variant works on graphics cards supporting DirectX feature level, but will be slower.
  * Output size: determined by scale configuration

* FSR_RCAS: Port of the sharpening step in [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR).
  * Output size: the same as the input
  * Parameter
    * sharpness

* FSRCNNX: Port of FSRCNNX_x2_8-0-4-1
  * Output size: twice that of the input

* FSRCNNX_LineArt: Port of FSRCNNX_x2_8-0-4-1_LineArt
  * Output size: twice that of the input

* FXAA_Medium, FXAA_High, FXAA_Ultra: Fast anti-aliasing by approximation. In increasing order of demand to computing power.
  * Output size: the same as the input

* GTU_v050: Aims to emulate the blurry and mixed effects of CRT displays rather than masks or curvature. Supports scanning lines.
  * Output size: determined by scale configuration
  * Parameters
    * Composite Connection
    * No Scanlines
    * Signal Resolution Y
    * Signal Resolution I
    * Signal Resolution Q
    * TV Vertical Resolution
    * Black Level
    * Contrast

* ImageAdjustment：Image parameters adjustment
  * Output size: the same as the input
  * Parameters
    * Target Gamma
    * Monitor Gamma
    * Saturation
    * Luminance
    * Contrast
    * Brightness Boost
    * Black Level
    * Red Channel
    * Green Channel
    * Blue Channel

* Jinc: Scaling with the Jinc algorithm
  * Output size: determined by scale configuration
  * Parameters
    * Window Sinc Param: The smaller the value is the sharper the images become. Must be greater than 0. Default value: 0.5
    * Sinc Param: The larger the value is the sharper the images become. Must be greater than 0. Default value: 0.825
    * Anti-ringing Strength: The greater the value is the better the effect becomes, but the images will be more blurry.

* Lanczos: Scaling with the Lanczos algorithm.
  * Output size: determined by scale configuration
  * Parameters
    * Anti-ringing Strength: The greater the value is the better the effect becomes, but the images will be more blurry.

* LCAS：Lightweight 3D image scaling algorithm
  * Output size: determined by scale configuration
  * Parameter
    * Sharpness

* LumaSharpen: A popular sharpening effect in reshade.
  * Output size: the same as the input
  * Parameters
    * Sharpening Strength
    * Sharpening Limit: Anti-ringing strength.
    * Sample Pattern: Filter type.
    * Offset Bias: Filter bias.

* MMPX：Pixel art scaling algorithm. It can add details while retaining the artistic style.
  * Output size: twice that of the input

* Nearest: Nearest neighbor interpolation
  * Output size: determined by scale configuration

* NIS: Port of [NVIDIA Image Scaling](https://github.com/NVIDIAGameWorks/NVIDIAImageScaling).
  * Output size: determined by scale configuration
  * Parameters
    * Sharpness
  * Note: Only supports upscaling.

* NNEDI3_nns16_win8x4 and NNEDI3_nns64_win8x6：These shaders originally designed for deinterlacing and are also high-quality interpolation algorithms. NNEDI3_nns64_win8x6 produces higher quality results, but slower.
  * Output size: twice that of the input

* NVSharpen: Port of NVSharpen that was published along with NIS.
  * Output size: the same as the input
  * Parameter
    * Sharpness

* Pixellate: Scale with the Pixellate algorithm. Suitable for upscaling pixel arts.
  * Output size: determined by scale configuration

* RAVU_Lite_R3: Port of ravu-lite-r3
  * Output size: twice that of the input

* RAVU_Zoom_R3: Port of ravu-zoom-r3
  * Output size: determined by scale configuration
  * Note: Only supports upscaling.

* SharpBilinear: Scale with the Sharp-Bilinear algorithm. Suitable for upscaling pixel arts.
  * Output size: determined by scale configuration

* SMAA_Low, SMAA_Medium, SMAA_High, and SMAA_Ultra: SMAA anti-aliasing. In increasing order of demand for computing power.
  * Output size: the same as the input

* SSimDownscaler: Downscaling algorithm based on perceptron. Sharper than Catrom.
  * Output size: determined by scale configuration
  * Parameter
    * Oversharp: The larger the value, the sharper the image.

* xBRZ_2x, xBRZ_3x, xBRZ_4x, xBRZ_5x, and xBRZ_6x: Scale with the xBRZ algorithm. Suitable for upscaling pixel arts.
  * Output size: determined by the variant.

* xBRZ_Freescale and xBRZ_Freescale_Multipass: xBRZ algorithm supporting any scaling factor.
  * Output size: determined by scale configuration
