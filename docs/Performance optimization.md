This article may be helpful if you are facing performance issues (e.g. lagging, latency, too much power consumption).

Below are some possible situations:

## My graphics card is not powerful enough

If you cannot run some effects with high computing power requirements (e.g. Anime4K, AdaptiveSharpen, etc.) smoothly, try the following:

1. Change to the variants with lower requirements. For example, Anime4K_Upscale_S is much faster than Anime4K_Upscale_L. CAS is much faster than AdaptiveSharpen. They can effectively improve the smoothness of the effects at the cost of some quality degradation.
2. Change the capture mode. We recommend you to try each of them.

## Intermittent lagging

If your graphics card is powerful enough, but you are still experiencing lagging issues, try the following:

### Assume you're on Windows 11 24H2 and Nvidia cards

#### Strange fps drop after upscaling enabled for a while
At the beginning, everything is fine (Game 60 FPS / Magpie 60 FPS). But after about 10~15 seconds, both game and magpie fps drop to about 37 FPS, at the same time GPU usage go up to 100%, GPU MHz goes down from like 1000 to 700.

First, if you're gaming with "Power management mode: Optimal power", then try to set magpie program to "Prefer maximum performance" or maybe "Adaptive" but I haven't try adaptive mode, yet there is no guarantee. Game remains "Optimal power" is fine. By use this setting, GPU MHz will no longer goes down abnormal.

#### Nvidia App and Nvidia control panel
However, one of my PC doesn't work with this settings while others can be fixed with this way. The investigation result is, there is compatibility issue between Nvidia App and Nvidia control panel. If you use Nvidia App changed your driver settings, open control panel, modify same settings cause the control panel crash or says "Access denied". And settings by Nvidia App are not apply to game or program at all.

Don't use Nvidia App (version 11.0.2.341) to adjust Nvidia driver settings of driver like 572.16. Close Nvidia App and Nvidia control panel, go to `C:\ProgramData\NVIDIA Corporation\Drs`, delete all `.bin` files like `nvdrsdb0.bin`, `nvdrsdb1.bin`, `nvdrssel.bin`, `update.bin`, open Nvidia control panel again and redo all of your global settings and program specified settings, and avoid use Nvidia App driver settings after that.

#### Screen looks not as smooth as native game window
Game 60 FPS / Magpie 60 FPS, appears in either `Graphics Capture` or `Desktop Duplication`.

The investigation result is vertical sync. Although it looks very strange. At least for Windows 11 24H2, you need to turn on vertical sync for a game, to make WGC and DXGI capture doesn't lose frame. If not, even WGC and DXGI capture at 60 FPS and game run at 60 FPS, many or some game frame will lose or called duplicated frame.

Required vertical sync mode for game is "Traditional / Classic / Vertical sync: Use the 3D application setting and enable in-game vsync / Vertical sync: On". Not turn off vertical sync, or use "Vertical sync: Fast". I'm not sure if "Vertical sync: Adaptive / Adaptive (half refresh rate)" will work or not but I have no time for detailed testing.

For magpie, go with "Use the 3D application setting".

#### Crazy frame lose when using Desktop Duplication
At the same time, Game 60 FPS / Magpie 60 FPS. I find that this situation appears when I am using Magpie v0.10.6. Enable vertical sync in Magpie settings and use "Vertical sync: Use the 3D application setting" for Magpie program settings solved this. Upgrade to Magpie v0.11.1 also solved this.

#### Low Lantancy Mode and Max Frame Rate
Check these options if you encountered some other strange behavior. I'm not sure if Magpie has these issue, but for Lossless Scaling, use "Low Lantancy Mode: Ultra" will cause WGC API drag and drop position step back after release mouse button.

Set "Low Lantancy Mode: Off" and "Max Frame Rate: Off" for Magpie.
For game, set "Max Frame Rate: Off", but you can leave "Low Lantancy Mode: Ultra" for games if needed.

#### Hyperthread and Intel Alder Lake big.little CPUs
I suggest if you don't want to turn off Hyperthread, use cmd.exe batch `start /affinity 0x55 "" "C:\path to\game.exe"` for game or it's launcher, game will inherit parent process affinity. And `start /affinity 0xaa` for Magpie. Also adjust game graphics options, try to limit game CPU usage below 50% (4 physical core of 4 core 8 thread as example).

The 0x55 specify game run on 0 2 4 6 logical core of 4 core CPU, and other stuff 0xaa run on 1 3 5 7.

For high priority, use `start /high` for both game and Magpie, but I find certain game are easy to crash with affinity and high for unknown reason.

0x55 and 0xaa are example value, I use https://bitsum.com/tools/cpu-affinity-calculator

You may also utilize this trick to force game run on big cores of Intel Alder Lake big.little CPUs.

Reference:  
https://superuser.com/questions/690509/does-windows-know-how-to-appropriately-assign-threads-to-a-quad-core-processor-t

### Still lagging?
1. Change the capture mode. We recommend you to try each of them.
2. Turn on "Disable DirectFlip." DirectFlip is a technology to reduce input lag, but may be incompatible with some certain games.
3. If you have multiple graphics card, try changing the graphics adaptor.
4. Increase the process priority of Magpie and the priority in the driver (if such options exist).
5. Please submit an [Issue (Performance)](https://github.com/Blinue/Magpie/issues/new?assignees=&labels=performance&template=02_performance.yaml) if none of the attempts above works.

## I'd like to reduce the power consumption of Magpie

When you need to save electricity or reduce the heat generated, try the following:

1. Limit the frame rate.
2. Opt for effects that require lower performance.
