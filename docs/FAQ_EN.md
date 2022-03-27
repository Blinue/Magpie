## How to customize scaling effects (e.g. to change the sharpness of FSR)?

Please check the [Customize Scaling Configurations](https://github.com/Blinue/Magpie/wiki/Customizing_Scaling_Configurations) page.

## There are 2 cursors on the screen

This problem should no longer occur since v0.6.1. Please submit an issue if you triggered this bug on newer versions.

## "Initialization failed" when launching

Please first check the [System Requirements](https://github.com/Blinue/Magpie/blob/master/README_EN.md#System-Requirements), and then try fixing/updating the dotnet and MSVC runtime libraries. Please submit an issue if the procedures above didn't work.

## Duplicate-monitoring

When displaying performance monitor like RTSS (Rivatuner Statistics Server), there might be 2 OSD layers displayed with Magpie scaling. This is caused by d3d, the way to capture the screen since v0.7.0. It will be captured by RTSS as well. You can solve the problem by adding it to the blacklist.

## Error occurs when using Graphics Capture

It is a rare bug after v0.8, especially under Win10 v2004 or newer systems. Please notify the developers if you encounter this bug.

## The hot keys don't work, but `Scale after 5s` works.

1. Try changing the hot keys.
2. Try running Magpie as Administrator.

## Does Magpie support multiple monitors?

Support starts from v0.8.

## Lagging/latency

Please check the [Performance Improvements](https://github.com/Blinue/Magpie/wiki/Performance_Improvements) page.

## I'd like to do something with Magpie. Do I need to get approval?

There is no need for approval as long as you are follow its [License](https://github.com/Blinue/Magpie/blob/main/LICENSE) (GPLv3).

## Will using Magpie in multi-player games be detected as cheating?

Magpie is designed to be non-intrusive, so it will not likely to be detected as a cheating tool. Currently there is no banning report for using Magpie. Nevertheless, you need to take the risks of using this software on your own.

## What is "Disable DirectFlip?"

DirectFlip is a technology to reduce input lags, but it may cause troubles in some circumstances. Please turn on this option when you are in the following situationsL:

1. The game lags unexpectedly (game known with the issue: Days Gone).
2. Abnormal low frame rates.
3. The screen freezes when streaming.

## What is "Simulate Exclusive Fullscreen?"

A lot of software checks whether there are games running under dedicated fullscreen mode before popping up windows. Turn on this option if you'd like to play in fullscreen without being disturbed,

## What is the frame rate displayed in Magpie?

The frame rate displayed by Magpie is that of its own rather than that of the game. Due to the non-intrusive nature, Magpie has no way to detect the frame rates of the games themselves. We recommend you to use tools like RTSS to display the games' frame rates. There add-on layers can usually be captured by Magpie as well.
