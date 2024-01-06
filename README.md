This repo contains scripts that generate user shader for prescaling in
[mpv](https://mpv.io/).

For the generated user shaders, check the [master branch](https://github.com/bjin/mpv-prescalers/tree/master).

# Usage

Python 3 is required. `./gen.sh` will generate all user shaders in
current work directory.

Alternatively, you could generate shader with customized options:
```
./nnedi3.py --nns 32 --win 8x4 --max-downscaling-ratio 1.8 > ~/.config/mpv/shaders/nnedi3.hook
```

Or play video directly with scripts
```
mpv --glsl-shaders=<(path/to/ravu.py --weights-file path/to/ravu_weights-r3.py --use-gather) video.mkv
```

# License

Shaders in this repo are licensed under terms of LGPLv3. Check the header of
each file for details.
