<br>
<p align="center">
  <img src="./src/Magpie/Icons/SVG/Magpie Icon Full Disabled.svg" width="150px" height="150px" alt="Logo">
</p>
<h1 align="center">Magpie</h1>

<div align="center">

[![License](https://img.shields.io/github/license/Blinue/Magpie)](./LICENSE)
[![build](https://github.com/Blinue/Magpie/actions/workflows/build.yml/badge.svg)](https://github.com/Blinue/Magpie/actions/workflows/build.yml)
[![All Contributors](https://img.shields.io/github/all-contributors/Blinue/Magpie)](#acknowledgement-)
[![GitHub all releases](https://img.shields.io/github/downloads/Blinue/Magpie/total)](https://github.com/Blinue/Magpie/releases)

</div>

:earth_africa: **English** | [简体中文](./README_ZH.md)

Magpie is a lightweight window scaling tool that comes equipped with various efficient scaling algorithms and filters. Its primary purpose is to enhance game graphics and enable non-fullscreen games to display in fullscreen mode.

We are using [Weblate](https://weblate.org/) for localization work and would appreciate your help in translating Magpie into more languages.

[![Translation status](https://hosted.weblate.org/widgets/magpie/-/287x66-white.png)](https://hosted.weblate.org/engage/magpie/)

:point_right: [Download](https://github.com/Blinue/Magpie/releases)

:point_right: [FAQ](https://github.com/Blinue/Magpie/wiki/FAQ%20(EN))

:point_right: [Built-in effects](https://github.com/Blinue/Magpie/wiki/Built-in%20effects)

:point_right: [Compilation guide](https://github.com/Blinue/Magpie/wiki/Compilation%20guide)

:point_right: [Contributing](./CONTRIBUTING.md)

## Features

* Scale any window to fullscreen
* Numerous built-in algorithms, including Lanczos, [Anime4K](https://github.com/bloc97/Anime4K), [FSR](https://github.com/GPUOpen-Effects/FidelityFX-FSR), Adaptive Sharpen, various CRT shaders, and more
* WinUI-based user interface with support for light and dark themes
* Create configuration profiles for specific windows
* Multi-monitor support

## How to use

1. Configuring scaling modes

    Magpie provides some simple scaling modes by default, but it is recommended to configure them according to your specific use case. Then, change the global scaling mode on the "Profiles"-"Defaults" page.

2. Scaling a window

    To scale a window, bring the desired window to the foreground and press the shortcut key (default is Win+Shift+A) to display it in fullscreen mode. Note that the window to be scaled must be in windowed mode, not maximized or fullscreen mode. You can also use the "Scale after xs" button on the "Home" page, and Magpie will automatically scale the foreground window after a few seconds.

3. Creating profiles for windows
    
    This allows you to save configurations specific to a particular window. Magpie also supports automatically activate scaling when that window is brought to the foreground.

4. Customizing effects

    Magpie uses Direct3D compute shader to implement effects, but the syntax has been extended to define resources and organize multiple passes. For more information, please refer to [MagpieFX](https://github.com/Blinue/Magpie/wiki/MagpieFX%20(EN)). Those with experience in shader writing can easily create custom effects.

## Screenshots

<img src="img/Main window.png" alt= "Main window" height="300">

## System requirements

1. Windows 10 v1903+ or Windows 11
2. DirectX feature level 11

## Hints

1. If you have set DPI scaling and the window you want to scale does not support high DPI (which is common in older games), it is recommended to first enter the program's compatibility settings and set "High DPI scaling override" to "Application".

2. Some games support zooming the window, but with extremely naive algorithms. Please set the resolution to the built-in (best) option.

## Acknowledgement ✨

Thanks go to these wonderful people:

<!-- ALL-CONTRIBUTORS-LIST:START - Do not remove or modify this section -->
<!-- prettier-ignore-start -->
<!-- markdownlint-disable -->
<table>
  <tbody>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Blinue"><img src="https://avatars.githubusercontent.com/u/34770031?v=4?s=100" width="100px;" alt="Xu"/><br /><sub><b>Xu</b></sub></a><br /><a href="#maintenance-Blinue" title="Maintenance">🚧</a> <a href="https://github.com/Blinue/Magpie/commits?author=Blinue" title="Code">💻</a> <a href="https://github.com/Blinue/Magpie/pulls?q=is%3Apr+reviewed-by%3ABlinue" title="Reviewed Pull Requests">👀</a> <a href="https://github.com/Blinue/Magpie/commits?author=Blinue" title="Documentation">📖</a> <a href="#question-Blinue" title="Answering Questions">💬</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/hooke007"><img src="https://avatars.githubusercontent.com/u/41094733?v=4?s=100" width="100px;" alt="hooke007"/><br /><sub><b>hooke007</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=hooke007" title="Documentation">📖</a> <a href="#question-hooke007" title="Answering Questions">💬</a> <a href="#userTesting-hooke007" title="User Testing">📓</a> <a href="https://github.com/Blinue/Magpie/commits?author=hooke007" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://palxex.ys168.com"><img src="https://avatars.githubusercontent.com/u/58222?v=4?s=100" width="100px;" alt="Pal Lockheart"/><br /><sub><b>Pal Lockheart</b></sub></a><br /><a href="#userTesting-palxex" title="User Testing">📓</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://www.stevedonaghy.com/"><img src="https://avatars.githubusercontent.com/u/1029699?v=4?s=100" width="100px;" alt="Steve Donaghy"/><br /><sub><b>Steve Donaghy</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=neoKushan" title="Code">💻</a> <a href="#translation-neoKushan" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://gyrojeff.top"><img src="https://avatars.githubusercontent.com/u/30655701?v=4?s=100" width="100px;" alt="gyro永不抽风"/><br /><sub><b>gyro永不抽风</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=JeffersonQin" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/ButtERRbrod"><img src="https://avatars.githubusercontent.com/u/89013889?v=4?s=100" width="100px;" alt="ButtERRbrod"/><br /><sub><b>ButtERRbrod</b></sub></a><br /><a href="#translation-ButtERRbrod" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/0x4E69676874466F78"><img src="https://avatars.githubusercontent.com/u/4449851?v=4?s=100" width="100px;" alt="NightFox"/><br /><sub><b>NightFox</b></sub></a><br /><a href="#translation-0x4E69676874466F78" title="Translation">🌍</a></td>
    </tr>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Tzugimaa"><img src="https://avatars.githubusercontent.com/u/4981077?v=4?s=100" width="100px;" alt="Tzugimaa"/><br /><sub><b>Tzugimaa</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=Tzugimaa" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/WHMHammer"><img src="https://avatars.githubusercontent.com/u/35433952?v=4?s=100" width="100px;" alt="WHMHammer"/><br /><sub><b>WHMHammer</b></sub></a><br /><a href="#translation-WHMHammer" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/kato-megumi"><img src="https://avatars.githubusercontent.com/u/29451351?v=4?s=100" width="100px;" alt="kato-megumi"/><br /><sub><b>kato-megumi</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=kato-megumi" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/MikeWang000000"><img src="https://avatars.githubusercontent.com/u/11748152?v=4?s=100" width="100px;" alt="Mike Wang"/><br /><sub><b>Mike Wang</b></sub></a><br /><a href="#userTesting-MikeWang000000" title="User Testing">📓</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://sammyhori.com"><img src="https://avatars.githubusercontent.com/u/116026761?v=4?s=100" width="100px;" alt="Sammy Hori"/><br /><sub><b>Sammy Hori</b></sub></a><br /><a href="#translation-sammyhori" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/NeilTohno"><img src="https://avatars.githubusercontent.com/u/28284594?v=4?s=100" width="100px;" alt="NeilTohno"/><br /><sub><b>NeilTohno</b></sub></a><br /><a href="#translation-NeilTohno" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/a0193143"><img src="https://avatars.githubusercontent.com/u/32773311?v=4?s=100" width="100px;" alt="a0193143"/><br /><sub><b>a0193143</b></sub></a><br /><a href="#translation-a0193143" title="Translation">🌍</a></td>
    </tr>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/soulset001"><img src="https://avatars.githubusercontent.com/u/121711747?v=4?s=100" width="100px;" alt="soulset001"/><br /><sub><b>soulset001</b></sub></a><br /><a href="#translation-soulset001" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/WluhWluh"><img src="https://avatars.githubusercontent.com/u/52004526?v=4?s=100" width="100px;" alt="WluhWluh"/><br /><sub><b>WluhWluh</b></sub></a><br /><a href="#design-WluhWluh" title="Design">🎨</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/SerdarSaglam"><img src="https://avatars.githubusercontent.com/u/42881121?v=4?s=100" width="100px;" alt="Serdar Sağlam"/><br /><sub><b>Serdar Sağlam</b></sub></a><br /><a href="#translation-SerdarSaglam" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/AndrusGerman"><img src="https://avatars.githubusercontent.com/u/30560543?v=4?s=100" width="100px;" alt="Andrus Diaz German"/><br /><sub><b>Andrus Diaz German</b></sub></a><br /><a href="#translation-AndrusGerman" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Kefir2105"><img src="https://avatars.githubusercontent.com/u/103105829?v=4?s=100" width="100px;" alt="Kefir2105"/><br /><sub><b>Kefir2105</b></sub></a><br /><a href="#translation-Kefir2105" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/animeojisan"><img src="https://avatars.githubusercontent.com/u/132756551?v=4?s=100" width="100px;" alt="animeojisan"/><br /><sub><b>animeojisan</b></sub></a><br /><a href="#translation-animeojisan" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/MuscularPuky"><img src="https://avatars.githubusercontent.com/u/93962018?v=4?s=100" width="100px;" alt="MuscularPuky"/><br /><sub><b>MuscularPuky</b></sub></a><br /><a href="#translation-MuscularPuky" title="Translation">🌍</a></td>
    </tr>
    <tr>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Zoommod"><img src="https://avatars.githubusercontent.com/u/71239440?v=4?s=100" width="100px;" alt="Zoommod"/><br /><sub><b>Zoommod</b></sub></a><br /><a href="#translation-Zoommod" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/fil08"><img src="https://avatars.githubusercontent.com/u/125665523?v=4?s=100" width="100px;" alt="fil08"/><br /><sub><b>fil08</b></sub></a><br /><a href="#translation-fil08" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/IsaiasYang"><img src="https://avatars.githubusercontent.com/u/20205571?v=4?s=100" width="100px;" alt="攸羚"/><br /><sub><b>攸羚</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=IsaiasYang" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="http://ohaiibuzzle.dev"><img src="https://avatars.githubusercontent.com/u/23693150?v=4?s=100" width="100px;" alt="OHaiiBuzzle"/><br /><sub><b>OHaiiBuzzle</b></sub></a><br /><a href="#translation-ohaiibuzzle" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/Rastadu23"><img src="https://avatars.githubusercontent.com/u/52637051?v=4?s=100" width="100px;" alt="Rastadu23"/><br /><sub><b>Rastadu23</b></sub></a><br /><a href="#translation-Rastadu23" title="Translation">🌍</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/hauuau"><img src="https://avatars.githubusercontent.com/u/52239673?v=4?s=100" width="100px;" alt="hauuau"/><br /><sub><b>hauuau</b></sub></a><br /><a href="https://github.com/Blinue/Magpie/commits?author=hauuau" title="Code">💻</a></td>
      <td align="center" valign="top" width="14.28%"><a href="https://github.com/nellydocs"><img src="https://avatars.githubusercontent.com/u/71311423?v=4?s=100" width="100px;" alt="nellydocs"/><br /><sub><b>nellydocs</b></sub></a><br /><a href="#translation-nellydocs" title="Translation">🌍</a></td>
    </tr>
  </tbody>
</table>

<!-- markdownlint-restore -->
<!-- prettier-ignore-end -->

<!-- ALL-CONTRIBUTORS-LIST:END -->

This project follows the [all-contributors](https://allcontributors.org/) specification. Contributions of any kind are welcome!
