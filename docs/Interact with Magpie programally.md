Magpie provides mechanisms for interaction with other programs. Through these mechanisms, your application can cooperate with Magpie.

[MagpieWatcher](https://github.com/Blinue/MagpieWatcher) demonstrates how to use these mechanisms.

## How to Receive Notifications When Scaling State Changes

You should listen for the MagpieScalingChanged message.

```c++
UINT WM_MAGPIE_SCALINGCHANGED = RegisterWindowMessage(L"MagpieScalingChanged");
```

### Parameters

`wParam` is the event ID. For different events, `lParam` has different meanings. Currently, two events are supported:

* 0: Scaling has ended. `lParam` is not used.
* 1: Scaling has started. `lParam` is the handle of the scaling window.

### Notes

If your process has a higher integrity level than Magpie, you won't receive messages broadcasted by Magpie due to User Interface Privilege Isolation (UIPI). In such cases, call [ChangeWindowMessageFilterEx](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-changewindowmessagefilterex) to allow receiving the MagpieScalingChanged message.

```c++
ChangeWindowMessageFilterEx(hYourWindow, WM_MAGPIE_SCALINGCHANGED, MSGFLT_ADD, nullptr);
```

## How to Get the Handle of the Scaling Window

You can listen for the MagpieScalingChanged message to obtain the handle of the scaling window. Additionally, while Magpie is scaling, you can also search for the window with the class name `Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22`. Magpie ensures that this class name remains unchanged and that only one scaling window exists at a time.

```c++
HWND hwndScaling = FindWindow(L"Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22", nullptr);
```

## How to Place Your Window Above the Scaling Window

Your window must be topmost. You should also listen for the MagpieScalingChanged message; you will receive one after the scaling window is shown, and then you can use `BringWindowToTop` to place your window above it. The scaling window does not attempt to adjust its position on the Z-axis while it exists.

```c++
HWND hWnd = CreateWindowEx(WS_EX_TOPMOST, ...);
...
if (message == WM_MAGPIE_SCALINGCHANGED) {
    switch (wParam) {
        case 0:
            // Scaling has ended
            break;
        case 1:
            // Scaling has started
            // Place this window above the scaling window
            BringWindowToTop(hWnd);
            break;
        default:
            break;
    }
}
```

## How to Obtain Scaling Information

Scaling information is stored in the [window properties](https://learn.microsoft.com/en-us/windows/win32/winmsg/about-window-properties) of the scaling window. Currently available properties include:

* `Magpie.SrcHWND`: Handle of the source window
* `Magpie.SrcLeft`、`Magpie.SrcTop`、`Magpie.SrcRight`、`Magpie.SrcBottom`: Source region of scaling
* `Magpie.DestLeft`、`Magpie.DestTop`、`Magpie.DestRight`、`Magpie.DestBottom`: Destination region of scaling

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

### Notes

These properties are only guaranteed to exist after the scaling window has completed its initialization. Therefore, it is advisable to check whether the scaling window is visible before retrieving these properties, especially when the window handle is obtained using the class name.

## How to Keep Magpie Scaling When Your Window Is in the Foreground

Magpie stops scaling when the foreground window changes, with some system windows being exceptions. By setting the `Magpie.ToolWindow` property, you can include your window and all its owned windows in the exceptions list.

```c++
SetProp(hYourWindow, L"Magpie.ToolWindow", (HANDLE)TRUE);
```

### Notes

According to the [documentation](https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-setpropw), you should use RemoveProp to clear this property before your window is destroyed. However, if you forget to do so, there's no need to worry: [the system will automatically clean it up](https://devblogs.microsoft.com/oldnewthing/20231030-00/?p=108939).
