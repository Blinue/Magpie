This article may be helpful if you are facing performance issues (e.g. lagging, latency, too much power consumption).

Below are some possible situations:

## My graphics card is not powerful enough

If you cannot run some effects with high computing power requirements (e.g. Anime4K, AdaptiveSharpen, etc.) smoothly, try the following:

1. Change to the variants with lower requirements. For example, Anime4K_Upscale_S is much faster than Anime4K_Upscale_L. CAS is much faster than AdaptiveSharpen. They can effectively improve the smoothness of the effects at the cost of some quality degradation.
2. Change the capture mode. We recommend you to try each of them.
3. Set the frame rate to "unlimited." This will turn off Vsync. It usually increases the frame rate substantially, but may causes the screen to tear.
4. Turn on "allow additional latency to improve performance" when Vsync is on. This will not lead to screen tearing and it also raises the frame rate. However, it will cause an extra 1-frame latency.

## Intermittent lagging

If your graphics card is powerful enough, but you are still experiencing lagging issues, try the following:

1. Change the capture mode. We recommend you to try each of them.
2. Turn on "Disable DirectFlip." DirectFlip is a technology to reduce input lag, but may be incompatible with some certain games.
3. If you have multiple graphics card, try changing the graphics adaptor.
4. Increase the process priority of Magpie and the priority in the driver (if such options exist).
5. Please submit an [Issue (Performance)](https://github.com/Blinue/Magpie/issues/new?assignees=&labels=performance&template=02_performance.yaml) if none of the attempts above works.

## I'd like to reduce the power consumption of Magpie

When you need to save electricity or reduce the heat generated, try the following:

1. Change the capture more. The Desktop Duplication capture mode effectively reduces the power consumption if there are a lot of static frames in the game.
2. Change the effects to their variants with lower requirements.
3. Limit the frame rate, which may cause screen tearing.
