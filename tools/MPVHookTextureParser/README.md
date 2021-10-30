# MPVHookTextureParser

用于将 mpv hook 中的 TEXTURE 块转换为 DDS 格式。

### 使用说明

以下面的格式将 TEXTURE 块的文本复制到文件中：

```
{WIDTH} {HEIGHT}
{TEXTURE DATA}
```

如

```
45 2592
00000000000000000000000000000000a164deb9bc0290ba6a16...
```

假设该文件名为 TEXTURE.txt，要将结果输出到 weights.dds 中，执行以下命令

``` bash
> .\MPVHookTextureParser TEXTURE.txt weights.dds
```

