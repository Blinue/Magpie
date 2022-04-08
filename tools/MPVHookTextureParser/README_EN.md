# MPVHookTextureParser

Translates the `TEXTURE` in mpv hood to `DDS` format.

### Usage Guides

Copy the texts in `TEXTURE` in to the files in the following format:

```
{WIDTH} {HEIGHT}
{TEXTURE DATA}
```

e.g.

```
45 2592
00000000000000000000000000000000a164deb9bc0290ba6a16...
```

Assuming the filename is `TEXTURE.txt`, execute the following command to output the results to `weights.dds`:

``` bash
> .\MPVHookTextureParser TEXTURE.txt weights.dds
```
