# VMware compatibility build

此分支基于 Magpie dev（上游提交 9e8e9027）。这是实验性兼容版本。

## 失败原因

VMware SVGA 驱动可能支持 DirectX feature level 11，并且暴露
ID3D11Device5 / ID3D11DeviceContext4，但 CreateFence 返回 E_INVALIDARG
(0x80070057)。原版在初始化呈现器和超分辨率后端时因此退出；截图也依赖
同一个接口。上游报告：

- https://github.com/Blinue/Magpie/issues/921
- https://github.com/Blinue/Magpie/issues/979

这些报告确认的是已知兼容问题，不等于已经确认每一台 VMware 虚拟机的失败原因。

## 修改

GpuCompletion 优先使用原有 Fence；CreateFence 失败时自动使用
D3D11_QUERY_EVENT。通过 End + Flush 提交 GPU 完成标记，GetData 查询结果。
该路径仍使用当前选中的 GPU，不强制 WARP 软件渲染。

呈现器、超分辨率后端和截图都使用此封装。查询操作始终在 immediate context
所属线程执行，避免违反 D3D11_CREATE_DEVICE_SINGLETHREADED 的约束。
等待有超时，并在设备出错或超时时记录日志，不把未完成的帧当作完成帧共享。
普通渲染等待上限为 5 秒，呈现器保持原有的 1 秒等待上限。

查询路径每次未完成时休眠 1 ms，可能比 Fence 增加一些延迟；截图在该路径下
会在后端线程等待完成，截图期间可能短暂影响捕获流畅度。

## 使用

1. Apple Silicon 的 VMware Fusion / Windows 11 ARM 请优先使用 ARM64 包；
   Intel/AMD Windows 虚拟机使用 x64 包。
2. 将整个目录解压到 Windows 本地磁盘，再运行 Magpie.exe。保留所有 DLL、
   Assets 和 effects 等配套文件。
3. 首次使用选择简单的缩放效果（例如 Lanczos），捕获方式先试 Graphics Capture；
   如果黑屏或捕获失败，再试 GDI。GDI 能否捕获取决于目标程序。
4. 呈现异常时可以尝试“禁用 DirectFlip”。确保 VMware 的 3D 加速已启用且
   VMware Tools 已安装。
5. 请关闭自动更新，以免兼容版本被上游原版覆盖。

当日志中出现 `CreateFence 不可用，回退到 D3D11_QUERY_EVENT`，说明已进入兼容路径。
如果仍失败，需要检查该条日志之后的错误；共享纹理、捕获和交换链方面的驱动
限制不属于本次 Fence 回退所解决的问题。

此版本未使用上游私钥签名 TouchHelper，因此需要签名的触控辅助功能不可用。

## 构建与验证

在满足上游编译依赖的 Windows 环境：

```powershell
python scripts/publish.py --platform=ARM64
# 或
python scripts/publish.py --platform=x64
```

输出目录为 publish/ARM64 或 publish/x64。仓库的 build workflow 构建这两个架构。
Windows 编译通过只证明源代码可以构建；VMware 上实际缩放、重绘和截图仍需实机验证。

本次操作时，VMware 的 vmrun 接口要求加密虚拟机密码，未能通过该接口进入来宾系统，
因此没有声称已经完成这台虚拟机上的运行验证。
