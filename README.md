# MAGPIE

窗口放大镜！

可以将任意窗口全屏显示，支持高级缩放算法，包括 Jinc、[Anime4K](https://github.com/bloc97/Anime4K)（本项目包含一个hlsl移植）、Lanczos等。

主要用于游戏窗口的放大显示，适用于那些不支持全屏模式，或者游戏自带的全屏模式会使画面模糊的情况。

本项目还处于早期阶段，欢迎fork和star，欢迎任何形式的贡献。

## 使用方法

![窗口截图](img/窗口截图.png)

程序启动后，激活要放大的窗口，按下热键即可全屏显示该窗口，再次按下热键将退出全屏。

以下为配置说明：

#### 缩放模式

目前缩放模式仅支持通用（Lanczos+锐化）以及动漫（Anime4K+mitchell+锐化）。内部使用json，如果你想，可以轻松地组合出自己的缩放模式。

#### 抓取模式

程序如何抓取源窗口图像，有三种选择：

1. WinRT Capture：使用[Screen Capture API](https://docs.microsoft.com/en-us/windows/uwp/audio-video-camera/screen-capture)抓取窗口，几乎完美的抓取方案，唯一的缺点是只支持Windows 10, version 1803及以后的版本
2. GDI：使用GDI抓取源窗口，速度较快，无法抓取到一些DirectX窗口
3. MagCallback：使用[Magnification API](https://docs.microsoft.com/en-us/previous-versions/windows/desktop/magapi/entry-magapi-sdk)抓取源窗口，相比GDI可以抓取到更多类型的窗口，但速度较慢

#### 注入模式

如果源程序使用了自定义鼠标，屏幕上可能出现两个鼠标，使用进程注入可解决这个问题。有三种选择：

1. 不使用注入：适用于源窗口没有自定义鼠标的场合
2. 运行时注入：在窗口运行时按下热键可进入全屏并注入窗口，退出全屏后取消注入
3. 启动时注入：适用于运行时注入不起作用的场合，不能注入正在运行的进程，需要手动选择要启动并注入的程序

#### 高级选项

* 显示帧率：在屏幕左上角显示帧率
* 低延迟模式：使用“可等待对象”降低帧延迟。开启后可有效降低输入延迟，在帧率不足时可稍微提高帧率
* 关闭垂直同步：解除锁帧。不推荐使用

## 效果截图

*以下图像均只用于演示目的*

#### 通用模式

源窗口

![通用_源](img/通用_源.png)

放大后

![通用_放大后](img/通用_放大后.png)

#### Anime4K模式

源窗口

![Anime4K_源](img/Anime4K_源.png)

放大后

![Anime4K_放大后](img/Anime4K_放大后.png)

## 实现原理

尽管功能与[Lossless Scaling](https://store.steampowered.com/app/993090/Lossless_Scaling/)和[IntegerScaler](https://tanalin.com/en/projects/integer-scaler/)类似，但本程序的实现原理与它们完全不同。Lossless Scaling和IntegerScaler使用Magnification API实现对窗口的放大，但此API无法实现高级缩放算法，其核心函数[MagSetImageScalingCallback](https://docs.microsoft.com/en-us/windows/win32/api/magnification/nf-magnification-magsetimagescalingcallback)已被废弃，因此它们必须与显卡驱动打交道，而你的显卡很可能不被支持。此外，它们只支持整数倍的放大，这极大限制了它们的使用场景。举例来说，它们无法把一个1024x768大小的窗口放大到1920x1080。

本程序使用了一个十分符合直觉的方式放大窗口：使用一个全屏窗口覆盖屏幕，捕获原窗口的内容放大后在该全屏窗口显示出来。这种方式使得缩放算法不受任何限制，让我们可以自由使用现存的优秀缩放算法。为了使用GPU加速，全屏窗口使用了Direct2D技术，将缩放算法实现为[Direct2D Effect](https://docs.microsoft.com/en-us/windows/win32/direct2d/effects-overview)，通过Effect的堆叠，我们可以用任何方式缩放窗口，以取得完美的效果。

这种方案唯一的限制便是系统光标，因此这里使用了一点hack：将系统的光标替换为透明，然后在全屏窗口上绘制它，因此虽然光标始终处于源窗口内，但其不可见。大多数情况下，这些更改不会被用户感知到，尽管如此，如果源窗口使用了自定义光标，用户会在屏幕上看到两个光标。为了解决这个问题，我们提供了更深入的hack选项，即注入源窗口的进程，将其自定义光标也替换为透明，然后在全屏窗口上将其绘制，更深入的解释见[光标映射](./光标映射.md)。经过测试，该方法表现良好，如果遇到失效的情况请提交issue。

## 使用提示

1. 如果你设置了DPI缩放，而要放大的窗口不支持（表现为画面模糊），请首先进入该程序的兼容性设置，将“高DPI缩放替代”设置为“应用程序”。

   ![高DPI设置](img/高DPI设置.png)

2. 一些游戏支持调整窗口的大小，但只是简单的使用线性缩放，这时请先将其设为原始分辨率。

## 免责声明

因为使用了进程注入技术，本程序极有可能被报毒。出于安全考虑，您应该检查源代码并自行编译。

开发本程序的初衷不含有任何恶意，但使用它所造成的后果应由您自己承担。

## 开发计划

见 [Milestones](https://github.com/Blinue/Magpie/milestones)

