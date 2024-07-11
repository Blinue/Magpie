### 双重监控

使用性能计数器屏显时，例如 RTSS (Rivatuner Statistics Server)，你可能会在缩放时看到两个叠加层。这是由于 Magpie 使用 Direct3D 呈现画面，它也会被 RTSS 捕捉。如果你不关心 Magpie 的性能，请将 Magpie 添加到黑名单。

### 快捷键不起作用，但可以使用 "x秒后缩放"

1. 尝试更换快捷键
2. 尝试以管理员身份运行 Magpie

### 是否支持多屏？

从 v0.8 开始支持。

### 卡顿/延迟

请查看[性能优化建议](https://github.com/Blinue/Magpie/wiki/性能优化建议)。

### 在多人游戏中使用 Magpie 会被认定为作弊吗？

Magpie 被设计为非侵入式，因此一般不会被识别为作弊工具。目前没有因使用 Magpie 被封禁的报告。尽管如此，你应该了解到使用本软件的风险。

### 什么是“禁用 DirectFlip”？

DirectFlip 是一种用于降低输入延迟的技术，但可能会在一些情况下造成问题。如果你遇到了下面的情形，可以尝试打开这个选项：

1. 游戏异常卡顿（已知游戏：Days Gone）
2. 不正常的低帧率
3. 串流时画面冻结

### 什么是“3D 游戏模式”？

此选项可以更改光标和游戏内叠加层的行为以适配 3D 游戏，建议在缩放 3D 游戏时开启此选项。

### 什么是“模拟独占全屏”？

很多软件弹窗或推送前会先检查是否有独占全屏的游戏，开启此选项可以让你在全屏状态下不受打扰。一些用户报告开启此选项后游戏可能无法正常启动 [#495](https://github.com/Blinue/Magpie/issues/495)，如果你遇到了类似的问题，请关闭它。

### Magpie 中显示的帧率是什么？

Magpie 显示的是自己的帧率而不是游戏的，基于非侵入性原则，Magpie 无法获知游戏的帧率。建议使用 RTSS 等工具显示游戏帧率，它们的叠加层一般也可以被 Magpie 捕获到。

### 是否支持触控？

从 v0.11 开始支持。支持触控要求 Magpie 拥有相当高的权限，更改此选项需要管理员权限，详情见[关于触控支持](https://github.com/Blinue/Magpie/wiki/关于触控支持)。

### 是否支持帧生成？

目前没有实现帧生成的计划。Magpie 的目的是提高游戏画质而不是性能，后处理补帧很难达到足够好的观感，而且还会增加延迟。游戏引擎可以使用运动矢量和深度缓冲以进行改善，但 Magpie 不具备这种条件。与之同理，FSR 2/3 也不在开发计划内，因其必须采用内部集成，后处理几乎无法实现。

如果你有好的解决方案，欢迎提交 PR。

### Magpie 和 Lossless Scaling 有何关系？

[Lossless Scaling](https://store.steampowered.com/app/993090/Lossless_Scaling/) 是 Steam 上的一款和 Magpie 功能类似的收费软件。Magpie 于 2021 年 2 月问世，当时市面上的窗口缩放软件（如 [IntegerScaler](https://tanalin.com/en/projects/integer-scaler/) 和 Lossless Scaling）仅支持简单的缩放算法，如最近邻缩放和整数倍缩放。这些软件的核心功能使用 [Magnification API](https://learn.microsoft.com/en-us/windows/win32/api/_magapi/) 实现，因此存在很大的局限性。

为了支持更高级的缩放算法，我开发并开源了 Magpie，它采用了完全不同的方式来实现缩放。最初，Magpie 仅支持 Anime4K、Lanczos、Adaptive Sharpen 等算法。在用户的反馈下，Magpie 在 [2021 年 7 月](https://github.com/Blinue/Magpie/commit/7f6c66f3b47ccd64da41d298faa7a8e185bd5299)加入了对 FSR 的支持。不久之后，Lossless Scaling 推出了 1.4.0 版本，采用与 Magpie 类似的方式支持 FSR。

现在，Magpie 和 Lossless Scaling 有很大的不同。Magpie 的重点是可用性和支持更多的缩放算法，而 Lossless Scaling 则主要聚焦于提高 3D 游戏的性能。
