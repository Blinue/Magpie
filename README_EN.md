# MAGPIE

[![build](https://github.com/Blinue/Magpie/actions/workflows/build.yml/badge.svg)](https://github.com/Blinue/Magpie/actions/workflows/build.yml)
[![All Contributors](https://img.shields.io/github/all-contributors/Blinue/Magpie)](#%E8%B4%A1%E7%8C%AE%E8%80%85-)
[![许可](https://img.shields.io/github/license/Blinue/Magpie)](./LICENSE)

:earth_africa: [简体中文](./README.md) | **English**

Magpie magnifies any window to fullscreen with a handful of algorithms/filter. The main goal is to tackle the situations where the games either do not natively support fullscreen or display blurry images under fullscreen modes.

Please raise an issue if you are having trouble running the app.

👉 [Compilation Guidance (in Chinese)](https://github.com/Blinue/Magpie/wiki/编译指南)

👉 [FAQ (in Chinese)](https://github.com/Blinue/Magpie/wiki/FAQ)

👉 [Customized Scaling Configurations (in Chinese)](https://github.com/Blinue/Magpie/wiki/自定义缩放配置)（[Samples](https://gist.github.com/hooke007/818ecc88f18e229bca743b7ae48947ad)）

👉 [Contribution Guides (in Chinese)](./CONTRIBUTING.md)

## How to use the app

![Screenshot](img/窗口截图.png)

The the window to be magnified is at foreground, you can press the hot keys to make it fullscreen. Pressing the hot keys again or shift another window to foreground will exit the fullscreen mode.

Below are config guides:

#### Scaling Modes

The application ships with multiple scaling modes. If they don't fit your needs, please goto the [Customized Scaling Configurations](https://github.com/Blinue/Magpie/wiki/自定义缩放配置) page.

1. Lanczos：A common, traditional interpolation algorithm. It does well in keeping sharp edges.
2. FSR：A transplantation of [FidelityFX-FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR), good for 3D games.
3. FSRCNNX：Transplantation of [FSRCNNX](https://github.com/igv/FSRCNN-TensorFlow).
4. ACNet：Transplantation of [ACNetGLSL](https://github.com/TianZerL/ACNetGLSL). Good for Anime-style images or videos.
5. Anime4K：Transplantation of [Anime4K](https://github.com/bloc97/Anime4K). We use the Anime4K_Upscale_Denoise_L variant.
6. RAVU：See [About RAVU](https://github.com/bjin/mpv-prescalers#about-ravu).
7. CRT-Geom：One of the most popular CRT filters. We recommend you to customize your configurations. See [Emulation General Wiki](https://emulation.gametechwiki.com/index.php/CRT_Geom).
8. Integer Scale：Magnifies each pixel with integer factors. It completely keeps the visual effects of the original views. 2x and 3x are shipped with the Magpie.

#### Capture Modes

This section introduces how Magpie captures the source window. See [Capture Modes Comparison](https://github.com/Blinue/Magpie/wiki/%E6%8D%95%E8%8E%B7%E6%A8%A1%E5%BC%8F%E5%AF%B9%E6%AF%94) for their use cases.

1. Graphics Capture：Captures with [Screen Capture API](https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture). The recommended way. Available since Win10 v1903.
2. Desktop Duplication：Captures with [Desktop Duplication API](https://docs.microsoft.com/en-us/windows/win32/direct3ddxgi/desktop-dup-api). It is able to capture more types of windows compared with Graphics Capture. Available since Win10 v2004.
3. GDI：Captures with GDI. Has a lower CPU occupancy compared with Graphics Capture.
4. DwmSharedSurface：Capture with the private DwmSharedSurface API.

#### Change the Hot Keys

Magpie detects hot keys with [globalmousekeyhook](https://github.com/gmamaladze/globalmousekeyhook).

You need to manually type in your preferred hot keys (concatenate multiple keys with `+`).

1. Use upper cases for letter keys (e.g. `A`); Use special formats for digits (e.g. `D1`, or `NumPad1` if using the NumPad).
2. Capitalize the first letter for functional keys, like `Capital` (`Caps` is not recognized), `F1`, or `PageUp`.
3. Some keys can't be assigned individually. You have to combine them with other keys (e.g. `Control`, `Shift`, `Alt`).
4. Combinations of keys has to be triggered in the correct sequence (e.g. for `B+C`, you have to press and hold `B` and then press `C`).

## System Requirements

1. Windows 10/11
2. DirectX feature level 10
3. [.NET 6 Desktop Runtime](https://dotnet.microsoft.com/en-us/download/dotnet/6.0)
4. [MSVC 2015-2022 Runtime](https://docs.microsoft.com/cpp/windows/latest-supported-vc-redist)

## Hints

1. If you have set DPI scaling, but the window to be magnifies doesn't support high DPI: Go to the compatibility settings of the target application, and set the "Alternative High DPI Scaling" option to "Application".

   ![High DPI Setting](img/高DPI设置.png)

2. Some games support zooming the window, but with extremely naive algorithms. Please set the resolution to the built-in (best) option.

## Acknowledgement ✨

Thanks go to these wonderful people ([emoji key](https://allcontributors.org/docs/en/emoji-key)):

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tr>
    <td align="center"><a href="https://github.com/Blinue"><img src="https://avatars.githubusercontent.com/u/34770031?v=4?s=100" width="100px;" alt=""/><br /><sub><b>刘旭</b></sub></a><br /><a href="#maintenance-Blinue" title="Maintenance">🚧</a> <a href="https://github.com/Blinue/Magpie/commits?author=Blinue" title="Code">💻</a> <a href="https://github.com/Blinue/Magpie/pulls?q=is%3Apr+reviewed-by%3ABlinue" title="Reviewed Pull Requests">👀</a> <a href="https://github.com/Blinue/Magpie/commits?author=Blinue" title="Documentation">📖</a> <a href="#question-Blinue" title="Answering Questions">💬</a></td>
    <td align="center"><a href="https://github.com/hooke007"><img src="https://avatars.githubusercontent.com/u/41094733?v=4?s=100" width="100px;" alt=""/><br /><sub><b>hooke007</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=hooke007" title="Documentation">📖</a> <a href="#question-hooke007" title="Answering Questions">💬</a> <a href="#userTesting-hooke007" title="User Testing">📓</a> <a href="https://github.com/Blinue/Magpie/commits?author=hooke007" title="Code">💻</a></td>
    <td align="center"><a href="http://palxex.ys168.com"><img src="https://avatars.githubusercontent.com/u/58222?v=4?s=100" width="100px;" alt=""/><br /><sub><b>Pal Lockheart</b></sub></a><br /><a href="#userTesting-palxex" title="User Testing">📓</a></td>
    <td align="center"><a href="https://www.stevedonaghy.com/"><img src="https://avatars.githubusercontent.com/u/1029699?v=4?s=100" width="100px;" alt=""/><br /><sub><b>Steve Donaghy</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=neoKushan" title="Code">💻</a> <a href="#translation-neoKushan" title="Translation">🌍</a></td>
    <td align="center"><a href="http://gyrojeff.top"><img src="https://avatars.githubusercontent.com/u/30655701?v=4?s=100" width="100px;" alt=""/><br /><sub><b>gyro永不抽风</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=JeffersonQin" title="Code">💻</a></td>
    <td align="center"><a href="https://github.com/ButtERRbrod"><img src="https://avatars.githubusercontent.com/u/89013889?v=4?s=100" width="100px;" alt=""/><br /><sub><b>ButtERRbrod</b></sub></a><br /><a href="#translation-ButtERRbrod" title="Translation">🌍</a></td>
    <td align="center"><a href="https://github.com/0x4E69676874466F78"><img src="https://avatars.githubusercontent.com/u/4449851?v=4?s=100" width="100px;" alt=""/><br /><sub><b>NightFox</b></sub></a><br /><a href="#translation-0x4E69676874466F78" title="Translation">🌍</a></td>
  </tr>
  <tr>
    <td align="center"><a href="https://github.com/Tzugimaa"><img src="https://avatars.githubusercontent.com/u/4981077?v=4?s=100" width="100px;" alt=""/><br /><sub><b>Tzugimaa</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=Tzugimaa" title="Code">💻</a></td>
    <td align="center"><a href="https://github.com/WHMHammer"><img src="https://avatars.githubusercontent.com/u/35433952?v=4?s=100" width="100px;" alt=""/><br /><sub><b>WHMHammer</b></sub></a><br /><a href="#translation-WHMHammer" title="Translation">🌍</a></td>
  </tr>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the [all-contributors](https://github.com/all-contributors/all-contributors) format. We welcome any form of contribution!
