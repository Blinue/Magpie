## 如何自定义缩放（比如改变 FSR 的锐度）？

请查阅[自定义缩放配置](https://github.com/Blinue/Magpie/wiki/%E8%87%AA%E5%AE%9A%E4%B9%89%E7%BC%A9%E6%94%BE%E9%85%8D%E7%BD%AE)。

## 屏幕上有两个光标

v0.6.1 之后不应遇到此问题。如果你在最新版本上遇到了这个错误，请提交 issue。

## 启动时提示“初始化失败”

首先请检查[系统需求](https://github.com/Blinue/Magpie/blob/master/README.md#%E7%B3%BB%E7%BB%9F%E9%9C%80%E6%B1%82)，尝试修复/更新 dotnet 和 MSVC 运行库。如果这些尝试不起作用，请提交一个 issue。

## 双重监控

使用性能计数器屏显时，例如RTSS(Rivatuner Statistics Server)，magpie缩放后可能产生两个OSD计数显示层，
这是由于 v0.7.0 使用d3d的捕获方式呈现画面，它也会被RTSS捕捉，添加到黑名单解决。

## 使用 Graphics Capture 出错

v0.8 之后这种情况相当罕见，尤其是在 Win10 v2004 或更新的系统中。如果你遇到了，请告知开发者。

## 快捷键不起作用，但可以使用 `5秒后缩放`

1. 尝试更换快捷键
2. 尝试以管理员身份运行 Magpie

## 是否支持多屏？

从 v0.8 开始支持。

## 卡顿/延迟

请查看[性能优化建议](https://github.com/Blinue/Magpie/wiki/%E6%80%A7%E8%83%BD%E4%BC%98%E5%8C%96%E5%BB%BA%E8%AE%AE)。

## 我想对 Magpie 做一些喜闻乐见的事情，需要获得许可吗？

不需要，但你要遵守它的[许可证](https://github.com/Blinue/Magpie/blob/master/LICENSE)（GPLv3）。

## 在多人游戏中使用 Magpie 会被认定为作弊吗？

Magpie 被设计为非侵入式，因此一般不会被识别为作弊工具。目前没有因使用 Magpie 被封禁的报告。尽管如此，你应该了解到使用本软件的风险。

## 什么是“禁用 DirectFlip”？

DirectFlip 是一种用于降低输入延迟的技术，但可能会在一些情况下造成问题。如果你遇到了下面的情形，可以尝试打开这个选项：

1. 游戏异常卡顿（已知游戏：Days Gone）
2. 不正常的低帧率
3. 串流时画面冻结

## 什么是“模拟独占全屏”

很多软件弹窗或推送前会先检查是否有独占全屏的游戏。如果你希望在全屏状态下不受打扰，可以打开此选项。

## Magpie 中显示的帧率是什么？

Magpie 显示的是自己的帧率而不是游戏的，基于非侵入性原则，Magpie 无法获知游戏的帧率。建议使用 RTSS 等工具显示游戏帧率，它们的叠加层一般也可以被 Magpie 捕获到。
