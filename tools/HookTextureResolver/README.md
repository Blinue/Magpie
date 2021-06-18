# HookTextureResolver

用于读取mpv hook中的TEXTURE块。[RAVU](https://github.com/bjin/mpv-prescalers)使用了TEXTURE块，移植到hlsl时必须将其解码。

较小的TEXTURE块可以转换为hlsl数组，较大的TEXTURE块则必须导出为纹理图片，将其作为effect的输入。

导出为图片时的一个问题是Direct2D不支持读取alpha通道，因此我将导出图片的宽度增加一倍以容纳更多信息。

png格式的每个颜色通道只有8位，因此导出时会有严重的精度损失。为了缓解这个问题，我进行了一些简单的压缩。实践证明，一定程度的精度损失对RAVU的效果没有肉眼可见的影响。

