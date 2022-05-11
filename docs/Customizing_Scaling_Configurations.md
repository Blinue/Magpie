This article guides you how to define your own scaling modes.

Magpie with search for the `ScaleModels.json` configuration file in its folder when launching. It will generate the file with the default configurations if it is not found.

Click [here](https://github.com/Blinue/Magpie/blob/main/Magpie/Resources/BuiltInScaleModels.json) for the complete content of the original configuration file. The following example shows the syntax:

```json
[
  {
    "name": "Lanczos",
    "effects": [
      {
        "effect": "Lanczos",
        "scale": [ -1, -1 ]
      },
      {
        "effect": "AdaptiveSharpen",
        "curveHeight": 0.3
      }
    ]
  },
  {
    "name": "FSR",
    "effects": [
      {
        "effect": "FSR_EASU",
        "scale": [ -1, -1 ]
      },
      {
        "effect": "FSR_RCAS",
        "sharpness": 0.87
      }
    ]
  },
  {
    "...": "..."
  }
]
```

The root element of the configuration file is an array. Each element in the array represents one "scaling mode." The scaling modes are collections of "effects." Magpie will apply the effects in sequence when scaling. *The configuration file supports json comments, including inline comment `//` and block comment `/**/`.*

Magpie ships with a handful of effect that an be used in combinations. Most of the effects have parameters that can be customized. All effects are stored in the `effects` folder. You can easily add effects if you are familiar with HLSL. Check the [Customizing Effects](https://github.com/Blinue/Magpie/wiki/Customizing_Effects) page.

Many effects supports the `scale` parameter, which has to be an array with 2 elements. When they are positive, they mean the scaling factors of the width and the height. Negative numbers indicate the maximum ratio that fits in the screen. 0 mean to stretch and fit the screen. The default value of all `scale` parameters is `[1, 1]`, meaning exactly the same as the input. Check [Examples](#Examples) for their applications.

## Introduction to shipped effects

* ACNet: Transplantation of [ACNetGLSL](https://github.com/TianZerL/ACNetGLSL). Suitable for anime-style images. Strong denoise effects.
  * Output size: twice that of the input

* AdaptiveSharpen: Self-adaptive sharpening algorithm. This algorithm focuses on sharpening the blurry edges in the images. Hence it has less image noise, ringing artifacts, and stripes compared to the common sharpening algorithms.
  * Output size: the same as the input
  * Parameter
    * curveHeight: Sharpening magnitude. Must be greater than 0. Usually between 0.3~2.0. Default value: 1.0.

* Anime4K_3D_AA_Upscale_US and Anime4K_3D_Upscale_US: 3D game scaling algorithms provided by Anime4K. The AA variant supports anti-aliasing.
  * Output size: twice that of the input

* Anime4K_Denoise_Bilateral_Mean, Anime4K_Denoise_Bilateral_Median, and Anime4K_Denoise_Bilateral_Mode: Denoise algorithms provided by Anime4K. Uses mean, median, and mode, respectively.
  * Output size: the same as the input
  * Parameter:
    * intensitySigma: Denoise magnitude. Must be greater than 0. Default value: 1.0.

* Anime4K_Restore_M, Anime4K_Restore_L, and Anime4K_Restore_VL: Algorithms to restore the lines in animations. In increasing order of demand for computing power.
  * Output size: the same as the input

* Anime4K_Thin_HQ: Algorithm to clarify lines in animations provided by Anime4K.
  * Output size: the same as the input
  * Parameters
    * strength: The strength in each iteration. Must be greater than 0. Default value: 0.6.
    * iterations: The number of iterations. Must be an integer greater than 0. Default value: 1.
    Decreasing strength and increasing iterations improves the quality of the images, but will lower the processing speed.

* Anime4K_Upscale_S, Anime4K_Upscale_L, Anime4K_Upscale_Denoise_S, Anime4K_Upscale_Denoise_L, and Anime4K_Upscale_GAN_x2_S: Anime-style scaling algorithms provided by Anime4K. The denoise variant includes denoise functionality. The GAN variant, which keeps more details, is still under experiment.
  * Output size: twice that of the input

* Anime4K_Upscale_S_Lite: The light weight version of Anime4K_Upscale_S. It is faster, but at the cost of quality degradation, suitable for users will weak graphics cards.
  * Output size: twice that of the input

* Bicubic and Bicubic_Lite: Interpolation algorithms. The lite variant is fast, but at the cost of quality degradation, Suitable for users will weak graphics cards.
  * Output size: determined by the scale parameter. Best when used to downscale.
  * Parameters
    * scale: Scaling factor.
    * paramB: Must be in range 0~1. Default value: 0.333333. Too large values will result in blurry images.
    * paramC: Must be in range 0~1. Default value: 0.333333. Too large values will result in ring artifacts.
      Different combinations of parameters will lead to different variants of the algorithm. For example:
      Mitchell(B=C≈0.333333), Catmull-Rom(B=0 C=0.5), bicubic Photoshop(B=0 C=0.75), Spline(B=1 C=0)

* CAS: Transplantation of [FidelityFX-CAS](https://github.com/GPUOpen-Effects/FidelityFX-CAS). Lightweight sharpening effects.
  * Output size: the same as the input
  * Parameter
    * sharpness: Must be in range 0~1. Default value: 0.4.

* CRT_Easymode: Easy-to-configure lightweight CRT shader.
  * Output size: determined by the scale parameter.
  * Parameters
    * scale: Scaling factor.
    * sharpnessH: Horizontal sharpness. Range: 0~1. Default value: 0.5.
    * sharpnessV: Vertical sharpness. Range: 0~1. Default value: 1.
    * maskStrength: Mask strength. Range: 0~1. Default value: 0.3.
    * maskDotWidth: Range: 1~100. Default value: 1.
    * maskDotHeight: Range: 1~100. Default value: 1.
    * maskStagger: Range: 0~100. Default value: 0.
    * maskSize: Range: 1~100. Default value: 1.
    * scanlineStrength: Range: 0~1. Default value: 1.
    * scanlineBeamWidthMin: Range: 0.5~5. Default value: 1.5.
    * scanlineBeamWidthMax: Range: 0.5~5. Default value: 1.5.
    * scanlineBrightMin: Range: 0~1. Default value: 0.35.
    * scanlineBrightMax: Range: 0~1. Default value: 0.65.
    * scanlineCutoff: Range: 1~1000 (integer). Default value: 400.
    * gammaInput: Range: 0.1~5. Default value: 2.
    * gammaOutput: Range: 0.1~5. Default value: 1.8.
    * brightBoost: To increase brightness. Range: 1~2. Default value: 1.2.
    * dilation: Boolean. Default value: true.

* CRT_Geom: One of the most popular CRT shaders. Aims to emulate arcade machines. Check [Emulation General Wiki](https://emulation.gametechwiki.com/index.php/CRT_Geom).
  * Output size: determined by the scale parameter.
  * Parameters
    * scale: Scaling factor.
    * CRTGamma: Range: 0.1~5. Default value: 2.4.
    * monitorGamma: Range: 0.1~5. Default value: 2.2.
    * distance: Range: 0.1~3. Default value: 1.5.
    * curvature: Whether to emulate the screen curvature or not. Boolean. Default value: true.
    * radius: Range: 0.1~10. Default value: 2.
    * cornerSize: Range: 0.001~1. Default value: 0.03.
    * cornerSmooth: Range: 80~2000 (integer). Default value: 1000.
    * xTilt: Range: -0.5~0.5. Default value: 0.
    * yTilt: Range: -0.5~0.5. Default value: 0.
    * overScanX: Range: -125~125 (integer). Default value: 100.
    * overScanY: Range: -125~125 (integer). Default value: 100.
    * dotMask: Range: 0~0.3. Default value: 0.3.
    * sharper: The larger the value is, the clear the image becomes. Range: 1~3 (integer). Default value: 1.
    * scanlineWeight: Range: 0.1~0.5. Default value: 0.3.
    * lum: Increases illumination. Range: 0~1. Default value: 0.
    * interlace: Whether to emulate interlace. Boolean. Default value: true.

* CRT_Hyllian: Provides sharp and clear outputs with slight rims. Similar to Sony BVM displays.
  * Output size: determined by the scale parameter.
  * Parameters
    * scale: Scaling factor.
    * phosphor: Boolean. Default value: true.
    * vScanlines: Boolean. Default value: false.
    * inputGamma: Range: 0~5. Default value: 2.5.
    * outputGamma: Range: 0~5. Default value: 2.2.
    * sharpness: Range: 1~5 (integer). Default value: 1.
    * colorBoost: Range: 1~2. Default value: 1.5.
    * redBoost: Range: 1~2. Default value: 1.
    * greenBoost: Range: 1~2. Default value: 1.
    * blueBoost: Range: 1~2. Default value: 1.
    * scanlinesStrength: Range: 0~1. Default value: 0.5.
    * beamMinWidth: Range: 0~1. Default value: 0.86.
    * beamMaxWidth: Range: 0~1. Default value: 1.
    * crtAntiRinging: Range: 0~1. Default value: 0.8.

* CRT_Lottes: Provides multiple masks emulating Bloom and Halation effects. Similar to CGA arcade displays.
  * Output size: determined by the scale parameter.
  * Parameters
    * scale: Scaling factor.
    * hardScan: Range: -20~0 (integer). Default value: -8.
    * hardPix: Range: -20~0 (integer). Default value: -3.
    * warpX: Range: 0~0.125. Default value: 0.031.
    * warpY: Range: 0~0.125. Default value: 0.041.
    * maskDark: Range: 0~0.2. Default value: 0.5.
    * maskLight: Range: 0~0.2. Default value: 1.5.
    * scaleInLinearGamma: Boolean. Default value: true.
    * shadowMask: Masking style. Range: 0~4 (integer). Default value: 3.
    * brightBoost: Range: 0~2. Default value: 1.
    * hardBloomPix: Range: -2~-0.5. Default value: -1.5.
    * hardBloomScan: Range: -4~-1. Default value: -2.
    * bloomAmount: Range: 0~1. Default value: 0.15.
    * shape: Range: 0~10. Default value: 2.

* FineSharp: High-quality sharpening. The earliest version was the AviSynth script.
  * Output size: the same as the input
   * Parameters
     * sstr: Sharpening strength. Must be greater than or equal to 0. Default value: 2. If you change this parameter, you will have to also change cstr. Check the note below.
     * cstr: Balancing strength. Must be greater than or equal to 0. Default value: 0.9.
     * xstr: The sharpening strength in the final step of the XSharpen style. Range: 0~1 之间, better not to exceed 0.249. Default value: 0.19.
     * xrep: To fix the artifacts in the last step of sharpening. Must be greater than or equal to 0 0, Default value: 0.25.
   * Note: mapping between sstr and cstr (sstr->cstr): 0->0, 0.5->0.1, 1.0->0.6, 2.0->0.9, 2.5->1.00, 3.0->1.09, 3.5->1.15, 4.0->1.19, 8.0->1.249, 255.0->1.5

* FSR_EASU and FSR_EASU_DX10: Transplantation of the scaling step in [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR). The DX10 variant works on graphics cards supporting DirectX feature level, but will be slower.
  * Output size: determined by the scale parameter.
  * Parameter
    * scale: Scaling factor.

* FSR_RCAS: Transplantation of the sharpening step in [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR).
  * Output size: the same as the input
  * Parameter
    * sharpness: sharpening strength. Must be greater than 0. The greater the value is the sharper the images become. Default value: 0.87.

* FSRCNNX: Transplantation of FSRCNNX_x2_8-0-4-1
  * Output size: twice that of the input

* FSRCNNX_LineArt: Transplantation of FSRCNNX_x2_8-0-4-1_LineArt
  * Output size: twice that of the input

* FXAA_Medium, FXAA_High, FXAA_Ultra: Fast anti-aliasing by approximation. In increasing order of demand to computing power.
  * Output size: the same as the input

* GTU_v050: Aims to emulate the blurry and mixed effects of CRT displays rather than masks or curvature. Supports scanning lines.
  * Output size: determined by the scale parameter.
  * Parameters
    * scale: Scaling factor.
    * compositeConnection: Boolean. Default value: false.
    * noScanlines: Boolean. Default value: false.
    * signalResolution: Integer no less than 16. Default value: 256.
    * signalResolutionI: Positive integer. Default value: 83.
    * signalResolutionQ: Positive integer. Default value: 25.
    * tvVerticalResolution: Integer no less than 20. Default value: 250.
    * blackLevel: Range: -0.3~0.3. Default value: 0.07.
    * contrast: Range: 0~2. Default value: 1.

* Jinc: Scaling with the Jinc algorithm
  * Output size: determined by the scale parameter.
  * Parameters
    * scale: Scaling factor.
    * windowSinc: The smaller the value is the sharper the images become. Must be greater than 0. Default value: 0.5
    * sinc: The larger the value is the sharper the images become. Must be greater than 0. Default value: 0.825

* Lanczos: Scaling with the Lanczos algorithm.
  * Output size: determined by the scale parameter.
  * Parameters
    * scale: Scaling factor.
    * ARStrength: Anti-ringing strength. The greater the value is the better the effect becomes, but the images will be more blurry. Range: 0~1. Default value: 0.5.

* Linear: Bilinear interpolation.
  * Output size: determined by the scale parameter.
  * Parameter
    * scale: Scaling factor.

* LumaSharpen: A popular sharpening effect in reshade.
  * Output size: the same as the input
  * Parameters
    * sharpStrength: Sharpening strength. Must be greater than 0. Default value: 0.65.
    * sharpClamp: Anti-ringing strength. Range: 0~1. Default value: 0.035.
    * pattern: Filter type. Range: 0~3 (integer) (4 types of filters). Default value: 1.
    * offsetBias: Filter bias. Must be greater than or equal to 0. Default value: 1.

* Nearest: Nearest neighbor interpolation
  * Output size: determined by the scale parameter.
  * Parameter
    * scale: Scaling factor.

* NIS: Transplantation of [NVIDIA Image Scaling](https://github.com/NVIDIAGameWorks/NVIDIAImageScaling).
  * Output size: determined by the scale parameter.
  * Parameters
    * scale: Scaling factor.
    * sharpness: Sharpening strength. Range: 0~1. Default value: 0.5.

* NVSharpen: Transplantation of NVSharpen that was published along with NIS.
  * Output size: the same as the input
  * Parameter
    * sharpness: Sharpening strength. Range: 0~1. Default value: 0.5.

* Pixellate: Scale with the Pixellate algorithm. Suitable for upscaling pixel arts.
  * Output size: determined by the scale parameter.
  * Parameter
    * scale: Scaling factor.

* RAVU_Lite_R3: Transplantation of ravu-lite-r3
  * Output size: twice that of the input

* RAVU_Zoom_R3: Transplantation of ravu-zoom-r3
  * Output size: determined by the scale parameter.
  * Parameter
    * scale: Scaling factor.

* SharpBilinear: Scale with the Sharp-Bilinear algorithm. Suitable for upscaling pixel arts.
  * Output size: determined by the scale parameter.
  * Parameter
    * scale: Scaling factor.

* SMAA_Low, SMAA_Medium, SMAA_High, and SMAA_Ultra: SMAA anti-aliasing. In increasing order of demand for computing power.
  * Output size: the same as the input

* SSimDownscaler: Downscaling algorithm based on perceptron. Sharper than Catrom.
  * Output size: determined by the scale parameter.
  * Parameter
    * scale: Scaling factor.

* xBRZ_2x, xBRZ_3x, xBRZ_4x, xBRZ_5x, and xBRZ_6x: Scale with the xBRZ algorithm. Suitable for upscaling pixel arts.
  * Output size: determined by the variant.

* xBRZ_Freescale and xBRZ_Freescale_Multipass: xBRZ algorithm supporting any scaling factor.
  * Output size: determined by the scale parameter.
  * Parameter
    * scale: Scaling factor.

## Examples

1. If the display size is twice or 4 time that of the original window, you can apply Anime4K twice. The code snippet below shows how to use it:

    ```json
    {
        "name": "Animation 4x",
        "effects": [
          {
            "effect": "Anime4K_Upscale_Denoise_L"
          },
          {
            "effect": "Bicubic",
            "scale": [ -0.5, -0.5 ]
          },
          {
            "effect": "Anime4K_Upscale_L"
          }
        ]
    }
    ```

    To improve performance, you can scale the output of the first Anime4K scaling to half of the screen so the second Anime4K scaling scales it to exactly the size of the display. To clear the noises in the image, the first Anime4K used is the denoise variant.

2. Center the captured window (with dark edges).

    ```json
    {
        "name": "Source",
        "effects": [
          {
            "effect": "Nearest"
          }
        ]
    }
    ```

👉 [More examples](https://gist.github.com/hooke007/818ecc88f18e229bca743b7ae48947ad)
