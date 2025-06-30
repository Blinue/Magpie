Magpie provides mechanisms for interaction with other programs. Through these mechanisms, your application can cooperate with Magpie.

[MagpieWatcher](https://github.com/Blinue/MagpieWatcher) demonstrates how to use these mechanisms.

## How to Receive Notifications When Scaling State Changes

You should listen for the `MagpieScalingChanged` message.

```c++
UINT WM_MAGPIE_SCALINGCHANGED = RegisterWindowMessage(L"MagpieScalingChanged");
```

### Parameters

The `wParam` parameter indicates the event ID, and the meaning of `lParam` varies depending on the event. The following events are supported:

* 0: Scaling has ended or the source window has lost focus. `lParam` is 0 if scaling has ended, or 1 if the source window has lost focus.
* 1: Scaling has started or the source window has returned to the foreground. `lParam` contains the handle of the scaled window.
* 2: The scaled window’s position or size has changed. This event is exclusive to windowed scaling.
* 3: User has started resizing or moving the scaled window. This event is exclusive to windowed scaling.

### Notes

If your process has a higher integrity level than Magpie, you won't receive messages broadcasted by Magpie due to User Interface Privilege Isolation (UIPI). In such cases, call [ChangeWindowMessageFilterEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-changewindowmessagefilterex) to allow receiving the `MagpieScalingChanged` message.

```c++
ChangeWindowMessageFilterEx(hYourWindow, WM_MAGPIE_SCALINGCHANGED, MSGFLT_ADD, nullptr);
```

## How to Get the Handle of the Scaled Window

You can listen for the `MagpieScalingChanged` message to obtain the handle of the scaled window. Additionally, while Magpie is scaling, you can also search for the window with the class name `Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22`. Magpie ensures that this class name remains unchanged and that only one scaled window exists at a time.

```c++
HWND hwndScaling = FindWindow(L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22", nullptr);
```

## How to Place Your Window Above the Scaled Window

You should listen for the `MagpieScalingChanged` message and adjust your window's Z-order accordingly. Below is a simple example. For more advanced use cases, refer to [MagpieWatcher](https://github.com/Blinue/MagpieWatcher).

```c++
if (message == WM_MAGPIE_SCALINGCHANGED) {
    if (wParam == 0) {
        // Remove topmost status
        SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    } else if (wParam == 1) {
        // Ensure your window stays above the scaled window
        SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
    }
}
```

## How to Obtain Scaling Information

Scaling information is stored in the [window properties](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-window-properties) of the scaled window. Currently available properties include:

* `Magpie.Windowed`：Indicates whether Magpie is performing windowed scaling
* `Magpie.SrcHWND`: Handle of the source window
* `Magpie.SrcLeft`、`Magpie.SrcTop`、`Magpie.SrcRight`、`Magpie.SrcBottom`: Source region of scaling
* `Magpie.DestLeft`、`Magpie.DestTop`、`Magpie.DestRight`、`Magpie.DestBottom`: Destination region of scaling

```c++
HWND hwndSrc = (HWND)GetProp(hwndScaling, L"Magpie.SrcHWND");
bool isWindowed = (bool)GetProp(hwndScaling, L"Magpie.Windowed");

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

### Notes

1. These properties are only guaranteed to exist after the scaled window has completed its initialization. Therefore, it is advisable to check whether the scaled window is visible before retrieving these properties, especially when the window handle is obtained using the class name.
2. The coordinates stored in these properties are not DPI-virtualized. To use them correctly, you need to set your application's DPI awareness level to Per-Monitor V2. For more details, please refer to [High DPI Desktop Application Development on Windows](https://learn.microsoft.com/en-us/windows/win32/hidpi/high-dpi-desktop-application-development-on-windows).
