![Lines of code](https://img.shields.io/tokei/lines/github/Blinue/Magpie)

# MAGPIE

Magpie可以将任意窗口放大至全屏，支持多种高级缩放算法，包括Lanczos、[Anime4K](https://github.com/bloc97/Anime4K)、[RAVU](https://github.com/bjin/mpv-prescalers)、[FSRCNNX](https://github.com/igv/FSRCNN-TensorFlow)等。

主要用于游戏窗口的放大显示，适用于不支持全屏模式，或者内置的全屏模式会使画面模糊的情况。

欢迎标星，欢迎任何形式的共享。

## 使用方法

![窗口截图](img/窗口截图.png)

要放大的窗口位于前台时，按下热键即可全屏显示该窗口，再次按下热键或者切换前台窗口将退出全屏。

以下为配置说明：

#### 缩放模式

程序预置了多种缩放模式，如果它们不符合你的需求，请[自定义缩放](docs/自定义缩放.md)。

1. Lanczos：常见的传统插值算法，善于保留锐利的边缘。
2. RAVU：见[About RAVU](https://github.com/bjin/mpv-prescalers#about-ravu)。此预置使用了zoom变体。
3. ACNet：[ACNetGLSL](https://github.com/TianZerL/ACNetGLSL)的移植。适合动画风格的图像和视频放大。
4. Anime4K：开源的高质量的实时动漫缩放/降噪算法。
   * Anime4K：对输入应用一次Anime4K。默认使用降噪版本。
   * Anime4K+ThinLines：执行Anime4K后细化线条。
5. 像素：将每个像素放大整数倍，可以完整保留原图像的视觉效果。预置了2x和3x两种放大倍率。

#### 抓取模式

指示程序如何抓取源窗口图像

1. WinRT Capture：使用[Screen Capture API](https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture)抓取窗口，最推荐的方法。此API从Windows 10, v1803开始提供。
2. GDI：使用GDI抓取源窗口，速度稍慢。

#### 注入模式

如果源程序使用了自定义光标，屏幕上可能出现两个光标，Magpie提供了进程注入的功能解决这个问题：

1. 不使用注入：适用于源窗口没有自定义光标的场合
2. 运行时注入：在执行缩放的同时注入源窗口线程，退出全屏后取消注入
3. 启动时注入：适用于运行时注入不起作用的场合，不能注入正在运行的进程，需要手动选择要启动并注入的程序。

#### 高级选项

* 显示帧率：在屏幕左上角显示当前帧率

## 实现原理

因为实现原理的差别，Magpie比[Lossless Scaling](https://store.steampowered.com/app/993090/Lossless_Scaling/)和[IntegerScaler](https://tanalin.com/en/projects/integer-scaler/)强大的多。Magpie的原理非常简单：使用一个全屏窗口覆盖屏幕，捕获原窗口的内容放大后在该全屏窗口显示出来。这种方式使得缩放算法不受任何限制，让我们可以自由使用现存的优秀缩放算法。

## 使用提示

1. 如果你设置了DPI缩放，而要放大的窗口不支持（表现为画面模糊），请首先进入该程序的兼容性设置，将“高DPI缩放替代”设置为“应用程序”。

   ![高DPI设置](img/高DPI设置.png)

2. 一些游戏支持调整窗口的大小，但只是简单的使用线性缩放，这时请先将其设为原始分辨率。

## 免责声明

因为使用了进程注入技术，本程序极有可能被报毒。出于安全考虑，您应该检查源代码并自行编译。

开发本程序的初衷不含有任何恶意，但使用它所造成的后果应由您自己承担。

参见[LICENSE](./LICENSE)。

## 鸣谢

感谢以下优秀项目，你们使Magpie的存在成为可能

* [bloc97/Anime4K](https://github.com/bloc97/Anime4K)
* [nlohmann/json](https://github.com/nlohmann/json)
* [Boost C++ Libraries](https://www.boost.org/)
* [EasyHook/EasyHook](https://github.com/EasyHook/EasyHook)
* [microsoft/cppwinrt](https://github.com/microsoft/cppwinrt)
* [libretro/common-shaders](https://github.com/libretro/common-shaders)
* [TianZerL/Anime4KCPP](https://github.com/TianZerL/Anime4KCPP)

