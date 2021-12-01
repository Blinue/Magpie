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

此捕获模式只支持 Win10 v1909 及更新版本，此外也有部分游戏不支持此模式。

## 快捷键不起作用，但可以使用 `5秒后缩放`

你需要让 Magpie 拥有与源窗口相同或更高的权限，尝试以管理员身份运行 Magpie。

## 是否支持多屏？

提供有限的支持，目前无法在 DPI 缩放不同的多屏上工作。

## 卡顿/延迟

请查看[性能优化建议](https://github.com/Blinue/Magpie/wiki/%E6%80%A7%E8%83%BD%E4%BC%98%E5%8C%96%E5%BB%BA%E8%AE%AE)。

## 我想对 Magpie 做一些喜闻乐见的事情，需要获得许可吗？

不需要，但你要遵守它的[许可证](https://github.com/Blinue/Magpie/blob/master/LICENSE)（GPLv3）。

## 在多人游戏中使用 Magpie 会被认定为作弊吗？