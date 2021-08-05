# MAGPIE Translated by Prefix

Magpie can enlarge any window to full screen and supports a variety of advanced zoom algorithms, including Lanczos, [Anime4K](https://github.com/bloc97/Anime4K), [FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR), [FSRCNNX](https://github.com/igv/FSRCNN-TensorFlow), etc.

It is mainly used for the enlarged display of the game window. It is suitable for situations where the full-screen mode is not supported, or the built-in full-screen mode will blur the picture.

Stars are welcome, and contributions of any kind are welcome.

☛ [Compilation Guide](https://github.com/Blinue/Magpie/wiki/%E7%BC%96%E8%AF%91%E6%8C%87%E5%8D%97)

## Instructions

![Window Screenshot](img/Window Screenshot.png)

When the window to be enlarged is in the foreground, press the hot key to display the window in full screen, and press the hot key again or switch the foreground window to exit the full screen.

The following is the configuration description:

#### Zoom mode

The program presets a variety of zoom modes, if they do not meet your needs, please [custom zoom](https://github.com/Blinue/Magpie/wiki/%E8%87%AA%E5%AE%9A%E4%B9%89%E7%BC%A9%E6%94%BE).

1. Lanczos: Common traditional interpolation algorithm, good at preserving sharp edges.
2. RAVU: See [About RAVU](https://github.com/bjin/mpv-prescalers#about-ravu). This preset uses a zoom variant.
3. FSRCNNX: A variant of FSRCNN. Excellent performance in various occasions.
4. ACNet: Porting of [ACNetGLSL](https://github.com/TianZerL/ACNetGLSL). Suitable for animation-style image and video enlargement.
5. Anime4K: an open source high-quality real-time animation scaling/noise reduction algorithm.
6. FSR: Suitable for 3D games.
7. Pixels: Enlarge each pixel by an integer multiple to preserve the visual effect of the original image. Two magnifications of 2x and 3x are preset.

#### Crawl mode

Instruct the program how to grab the source window image

1. WinRT Capture: Use [Screen Capture API](https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture) to capture windows, the most recommended method. This API has been provided since Windows 10, v1803.
2. GDI: Use GDI to grab the source window, the speed is slightly slower.

#### Injection mode

If a custom cursor is used in the source window, two cursors may appear on the screen. In order to solve this problem, Magpie provides the function of process injection:

1. No injection: suitable for situations where the source window does not have a custom cursor
2. Runtime injection: Inject the source window thread while performing zooming, and cancel the injection after exiting the full screen
3. Injection at startup: It is suitable for occasions where the runtime injection does not work. The running process cannot be injected. You need to manually select the program to be started and injected.

#### advanced options

* Display frame rate: display the current frame rate in the upper left corner of the screen

## Implementation principle

Because of the different implementation principles, Magpie is better than [Lossless Scaling](https://store.steampowered.com/app/993090/Lossless_Scaling/) and [IntegerScaler](https://tanalin.com/en/projects/integer-scaler/) Much more powerful. The principle of Magpie is very simple: use a full-screen window to cover the screen, and the content of the captured original window will be enlarged and displayed in the full-screen window. In this way, the scaling algorithm is not subject to any restrictions, allowing us to freely use existing excellent scaling algorithms.

## Use suggestions

1. If you have set DPI scaling, and the window to be enlarged does not support it (shown as a blurry picture), please enter the compatibility settings of the program first, and set "High DPI Scaling Alternative" to "Application".

   ![High DPI Setting](img/High DPI Setting.png)

2. Some games support adjusting the window size, but simply use linear scaling. In this case, please set it to the original resolution first.

## Disclaimer

Because of the use of process injection technology, this program is very likely to be reported as a virus. For security reasons, you should check the source code and compile it yourself.

The original intention of developing this program does not contain any maliciousness, but the consequences of using it should be borne by you.

See [LICENSE](./LICENSE).
