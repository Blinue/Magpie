Magpie 提供了和其他程序交互的机制。通过它们，你的应用可以和 Magpie 配合使用。

[MagpieWatcher](https://github.com/Blinue/MagpieWatcher) 演示了如何使用这些机制。

## 如何在缩放状态改变时得到通知

你应该监听 MagpieScalingChanged 消息。

```c++
UINT WM_MAGPIE_SCALINGCHANGED = RegisterWindowMessage(L"MagpieScalingChanged");
```

### 参数

`wParam` 为事件 ID，对于不同的事件 `lParam` 有不同的含义。目前支持两个事件：

* 0: 缩放已结束。不使用 `lParam`。
* 1: 缩放已开始。`lParam` 为缩放窗口句柄。

### 注意事项

如果你的进程完整性级别 (Integration level) 比 Magpie 更高，由于用户界面特权隔离 (UIPI)，你将无法收到 Magpie 广播的消息。这种情况下请调用 [ChangeWindowMessageFilterEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-changewindowmessagefilterex) 以允许接收 MagpieScalingChanged 消息。

```c++
ChangeWindowMessageFilterEx(hYourWindow, WM_MAGPIE_SCALINGCHANGED, MSGFLT_ADD, nullptr);
```

## 如何获取缩放窗口句柄

你可以监听 MagpieScalingChanged 消息来获取缩放窗口句柄，也可以查找类名为`Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22`的窗口以在缩放中途获取该句柄。Magpie 将确保此类名不会改变，且不会同时存在多个缩放窗口。

```c++
HWND hwndScaling = FindWindow(L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22", nullptr);
```

## 如何将你的窗口置于缩放窗口上方

你的窗口必须是置顶的。你还应该监听 MagpieScalingChanged 消息，当收到该消息时缩放窗口已经显示，然后你可以使用 `BringWindowToTop` 函数将自己的窗口置于缩放窗口上方。缩放窗口在存在期间不会尝试调整自己在 Z 轴的位置。

```c++
HWND hWnd = CreateWindowEx(WS_EX_TOPMOST, ...);
...
if (message == WM_MAGPIE_SCALINGCHANGED) {
    switch (wParam) {
        case 0:
            // 缩放已结束
            break;
        case 1:
            // 缩放已开始
            // 将本窗口置于缩放窗口上面
            BringWindowToTop(hWnd);
            break;
        default:
            break;
    }
}
```

## 如何获取缩放信息

缩放窗口的[窗口属性](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-window-properties)中存储着缩放信息。目前支持以下属性：

* `Magpie.SrcHWND`: 源窗口句柄
* `Magpie.SrcLeft`、`Magpie.SrcTop`、`Magpie.SrcRight`、`Magpie.SrcBottom`: 被缩放区域的边界
* `Magpie.DestLeft`、`Magpie.DestTop`、`Magpie.DestRight`、`Magpie.DestBottom`: 缩放后区域矩形边界

```c++
HWND hwndSrc = (HWND)GetProp(hwndScaling, L"Magpie.SrcHWND");

RECT srcRect;
srcRect.left = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.SrcLeft");
srcRect.top = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.SrcTop");
srcRect.right = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.SrcRight");
srcRect.bottom = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.SrcBottom");

RECT destRect;
destRect.left = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.DestLeft");
destRect.top = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.DestTop");
destRect.right = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.DestRight");
destRect.bottom = (LONG)(INT_PTR)GetProp(hwndScaling, L"Magpie.DestBottom");
```

### 注意事项

1. 这些属性只在缩放窗口初始化完成后才保证存在，因此建议检索属性前检查缩放窗口是否可见，尤其是当窗口句柄是使用类名获取到的。
2. 这些属性中存储的坐标不受 DPI 虚拟化影响，你需要将程序的 DPI 感知级别设置为 Per-Monitor V2 才能正确使用它们。有关详细信息，请参阅 [High DPI Desktop Application Development on Windows](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows)。

## 如何使 Magpie 在你的窗口位于前台时保持缩放

前台窗口改变时 Magpie 会停止缩放，只对某些系统窗口例外。你可以通过设置属性 `Magpie.ToolWindow` 将自己的窗口添加入例外，这对由该窗口拥有 (owned) 的窗口也有效。

```c++
SetProp(hYourWindow, L"Magpie.ToolWindow", (HANDLE)TRUE);
```

### 注意事项

根据[文档](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setpropw)的要求，你应该在你的窗口被销毁前使用 RemoveProp 清理这个属性。但如果你忘了也不会有问题，[系统会自动清理它](https://devblogs.microsoft.com/oldnewthing/20231030-00/?p=108939)。
