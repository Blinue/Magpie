# Magpie

[![许可](https://img.shields.io/github/license/Blinue/Magpie)](./LICENSE)
[![build](https://github.com/Blinue/Magpie/actions/workflows/build.yml/badge.svg)](https://github.com/Blinue/Magpie/actions/workflows/build.yml)
[![All Contributors](https://img.shields.io/github/all-contributors/Blinue/Magpie)](#%E8%B4%A1%E7%8C%AE%E8%80%85-)
[![GitHub all releases](https://img.shields.io/github/downloads/Blinue/Magpie/total)](https://github.com/Blinue/Magpie/releases)

🌍 **简体中文** | [English](./README_EN.md)

Magpie 是一个轻量级的缩放工具，能用多种高效算法和滤镜将任何窗口放大至全屏。它主要用于提升游戏画质和让不支持全屏的游戏也能全屏显示等。

:point_right: [下载](https://github.com/Blinue/Magpie/releases)

:point_right: [FAQ](https://github.com/Blinue/Magpie/wiki/FAQ)

:point_right: [内置效果介绍](https://github.com/Blinue/Magpie/wiki/内置效果介绍)

:point_right: [编译指南](https://github.com/Blinue/Magpie/wiki/编译指南)

:point_right: [贡献指南](./CONTRIBUTING.md)

## 功能

* 将任何窗口放大至全屏
* 众多内置算法，包括 Lanczos、[Anime4K](https://github.com/bloc97/Anime4K)、[FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR)、Adaptive Sharpen、多种 CRT 着色器等
* 基于 WinUI 的用户界面，支持浅色和深色主题
* 为特定窗口创建配置文件
* 多屏幕支持

## 如何使用

1. 配置缩放模式：首先在“缩放配置”页面配置所需的缩放模式。Magpie 预设了一些简单的缩放模式，但建议根据使用场景自行配置。然后在“配置文件”-“默认”页面更改全局缩放模式。
2. 缩放窗口：将窗口置于前台，按下快捷键（默认为 Win+Shift+A）即可缩放。注意，要缩放的窗口不能处于最大化或全屏状态，必须将其窗口化。也可以使用“主页”上的“x 秒后缩放”按钮，Magpie 将在数秒后自动缩放前台窗口。
3. 为窗口创建配置文件：这使你可以保存针对某个窗口的配置。创建配置文件后，可以在配置页面直接启动该程序/游戏。如果开启了“位于前台时自动缩放”，切换到该窗口时，Magpie 会自动执行缩放。
4. 自定义效果：Magpie 使用 Direct3D 计算着色器实现效果，但扩展了语法来定义资源、组织多个通道等，详见 [MagpieFX](https://github.com/Blinue/Magpie/wiki/MagpieFX) 。有着色器编写经验者可以轻松创建自定义效果。

## 截图

## 系统需求

1. Windows 10 v1903+ 或 Windows 11
2. DirectX 功能级别 11

## 使用提示

1. 如果你设置了 DPI 缩放，而要放大的窗口没有高 DPI 支持（这在老游戏中很常见），推荐首先进入该程序的兼容性设置，将“高 DPI 缩放替代”设置为“应用程序”。

   ![高DPI设置](img/高DPI设置.png)

2. 一些游戏支持调整窗口的大小，但只使用简单的缩放算法，这时请先将其设为原始（最佳）分辨率。

## 贡献者 ✨

感谢每一位参与贡献的人（[emoji key](https://allcontributors.org/docs/en/emoji-key)）：

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tbody>
    <tr>
      <td align="center"><a href="https://github.com/Blinue"><img src="https://avatars.githubusercontent.com/u/34770031?v=4?s=100" width="100px;" alt="刘旭"/><br /><sub><b>刘旭</b></sub></a><br /><a href="#maintenance-Blinue" title="Maintenance">🚧</a> <a href="https://github.com/Blinue/Magpie/commits?author=Blinue" title="Code">💻</a> <a href="https://github.com/Blinue/Magpie/pulls?q=is%3Apr+reviewed-by%3ABlinue" title="Reviewed Pull Requests">👀</a> <a href="https://github.com/Blinue/Magpie/commits?author=Blinue" title="Documentation">📖</a> <a href="#question-Blinue" title="Answering Questions">💬</a></td>
      <td align="center"><a href="https://github.com/hooke007"><img src="https://avatars.githubusercontent.com/u/41094733?v=4?s=100" width="100px;" alt="hooke007"/><br /><sub><b>hooke007</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=hooke007" title="Documentation">📖</a> <a href="#question-hooke007" title="Answering Questions">💬</a> <a href="#userTesting-hooke007" title="User Testing">📓</a> <a href="https://github.com/Blinue/Magpie/commits?author=hooke007" title="Code">💻</a></td>
      <td align="center"><a href="http://palxex.ys168.com"><img src="https://avatars.githubusercontent.com/u/58222?v=4?s=100" width="100px;" alt="Pal Lockheart"/><br /><sub><b>Pal Lockheart</b></sub></a><br /><a href="#userTesting-palxex" title="User Testing">📓</a></td>
      <td align="center"><a href="https://www.stevedonaghy.com/"><img src="https://avatars.githubusercontent.com/u/1029699?v=4?s=100" width="100px;" alt="Steve Donaghy"/><br /><sub><b>Steve Donaghy</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=neoKushan" title="Code">💻</a> <a href="#translation-neoKushan" title="Translation">🌍</a></td>
      <td align="center"><a href="http://gyrojeff.top"><img src="https://avatars.githubusercontent.com/u/30655701?v=4?s=100" width="100px;" alt="gyro永不抽风"/><br /><sub><b>gyro永不抽风</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=JeffersonQin" title="Code">💻</a></td>
      <td align="center"><a href="https://github.com/ButtERRbrod"><img src="https://avatars.githubusercontent.com/u/89013889?v=4?s=100" width="100px;" alt="ButtERRbrod"/><br /><sub><b>ButtERRbrod</b></sub></a><br /><a href="#translation-ButtERRbrod" title="Translation">🌍</a></td>
      <td align="center"><a href="https://github.com/0x4E69676874466F78"><img src="https://avatars.githubusercontent.com/u/4449851?v=4?s=100" width="100px;" alt="NightFox"/><br /><sub><b>NightFox</b></sub></a><br /><a href="#translation-0x4E69676874466F78" title="Translation">🌍</a></td>
    </tr>
    <tr>
      <td align="center"><a href="https://github.com/Tzugimaa"><img src="https://avatars.githubusercontent.com/u/4981077?v=4?s=100" width="100px;" alt="Tzugimaa"/><br /><sub><b>Tzugimaa</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=Tzugimaa" title="Code">💻</a></td>
      <td align="center"><a href="https://github.com/WHMHammer"><img src="https://avatars.githubusercontent.com/u/35433952?v=4?s=100" width="100px;" alt="WHMHammer"/><br /><sub><b>WHMHammer</b></sub></a><br /><a href="#translation-WHMHammer" title="Translation">🌍</a></td>
      <td align="center"><a href="https://github.com/kato-megumi"><img src="https://avatars.githubusercontent.com/u/29451351?v=4?s=100" width="100px;" alt="kato-megumi"/><br /><sub><b>kato-megumi</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=kato-megumi" title="Code">💻</a></td>
      <td align="center"><a href="https://github.com/MikeWang000000"><img src="https://avatars.githubusercontent.com/u/11748152?v=4?s=100" width="100px;" alt="Mike Wang"/><br /><sub><b>Mike Wang</b></sub></a><br /><a href="#userTesting-MikeWang000000" title="User Testing">📓</a></td>
      <td align="center"><a href="http://sammyhori.com"><img src="https://avatars.githubusercontent.com/u/116026761?v=4?s=100" width="100px;" alt="Sammy Hori"/><br /><sub><b>Sammy Hori</b></sub></a><br /><a href="#translation-sammyhori" title="Translation">🌍</a></td>
    </tr>
  </tbody>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

本项目遵循 [all-contributors](https://allcontributors.org/) 规范。欢迎任何形式的贡献！
