Magpie provides several capture methods. They have their pros and cons in different scenarios.

| | Graphics Capture | Desktop Duplication | GDI | DwmSharedSurface |
| :---: | :---: | :---: | :---: |:---: |
| Supports DirectComposition (e.g. UWP) | Yes | Yes | No | No |
| Supports recording/streaming | No under extreme conditions<sup>[1]</sup> | No | Yes | Yes |
| Support the source window to span multiple screens | No under extreme conditions<sup>[1]</sup> | No | Yes | Yes |
| Ignores DPI virtualization<sup>[2]</sup> | No | No | Yes| Yes |
| Notes | The most recommended capture method | Requires Win10 v2004, suitable for games with more static frames<sup>[3]</sup>, could capture pop-ups | | Low VRAM usage |


[1]: (1) The source window does not support regular window capture. (2) The operating system is Windows 11.

[2]: The system will perform bicubic interpolation upscaling to windows that do not support DPI scaling. The capture methods supporting this options captures the images before such scaling.

[3]: The Desktop Duplication mode effectively reduces the power consumption if there are many static frames.
