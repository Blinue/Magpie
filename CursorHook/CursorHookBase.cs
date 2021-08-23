using NLog;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Runtime.InteropServices;


namespace Magpie.CursorHook {
	// 运行时钩子和启动时钩子通用的部分
	// 继承此类只需实现 Run
	internal abstract class CursorHookBase : IDisposable {
		private static Logger Logger { get; } = LogManager.GetCurrentClassLogger();

		protected IntPtr hwndHost = IntPtr.Zero;
		protected IntPtr hwndSrc = IntPtr.Zero;


		private readonly (int x, int y) cursorSize = NativeMethods.GetCursorSize();

		// 保存已替换 HCURSOR 的窗口，以在卸载钩子时还原
		private readonly HashSet<IntPtr> replacedHwnds = new HashSet<IntPtr>();

		private static readonly IntPtr arrowCursor =
			NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_ARROW);
		private static readonly IntPtr handCursor =
			NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_HAND);
		private static readonly IntPtr appStartingCursor =
			NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_APPSTARTING);
		private static readonly IntPtr iBeamCursor =
			NativeMethods.LoadCursor(IntPtr.Zero, NativeMethods.IDC_IBEAM);

		// 原光标到透明光标的映射
		// 不替换透明的系统光标
		private readonly Dictionary<IntPtr, SafeCursorHandle> hCursorToTptCursor =
			new Dictionary<IntPtr, SafeCursorHandle>() {
				{arrowCursor, new SafeCursorHandle(arrowCursor, false)},
				{handCursor, new SafeCursorHandle(handCursor, false)},
				{appStartingCursor, new SafeCursorHandle(appStartingCursor, false)},
				{iBeamCursor, new SafeCursorHandle(iBeamCursor, false)}
			};

		protected const string HOST_WINDOW_CLASS_NAME = "Window_Magpie_967EB565-6F73-4E94-AE53-00CC42592A22";

		private static readonly uint MAGPIE_WM_NEWCURSOR = NativeMethods.RegisterWindowMessage(
			EasyHook.NativeAPI.Is64Bit ? "MAGPIE_WM_NEWCURSOR64" : "MAGPIE_WM_NEWCURSOR32");

		public abstract void Run();

		// 用于创建 SetCursor 委托
		[UnmanagedFunctionPointer(CallingConvention.StdCall, SetLastError = true)]
		protected delegate IntPtr SetCursorDelegate(IntPtr hCursor);

		// 取代 SetCursor 的钩子
		protected IntPtr SetCursorHook(IntPtr hCursor) {
			Logger.Debug($"源窗口已调用 SetCursor\n\thCursor：{hCursor}");

			IntPtr hCursorTar = hCursor;

			if (hwndHost == IntPtr.Zero || hCursor == IntPtr.Zero || !NativeMethods.IsWindow(hwndHost)) {
				// 不存在全屏窗口时钩子不做任何操作
			} else if (hCursorToTptCursor.ContainsKey(hCursor)) {
				hCursorTar = hCursorToTptCursor[hCursor].DangerousGetHandle();
			} else {
				// 未出现过的 hCursor
				SafeCursorHandle hTptCursor = CreateTransparentCursor(hCursor);
				if (hTptCursor == SafeCursorHandle.Zero) {
					return NativeMethods.SetCursor(hCursor);
				}

				hCursorToTptCursor[hCursor] = hTptCursor;

				// 向全屏窗口发送光标句柄
				ReportCursorMap(hTptCursor, hCursor);

				hCursorTar = hTptCursor.DangerousGetHandle();
			}

			return NativeMethods.SetCursor(hCursorTar);
		}


		private SafeCursorHandle CreateTransparentCursor(IntPtr hotSpot) {
			int len = cursorSize.x * cursorSize.y;

			// 全 0xff
			byte[] andPlane = new byte[len];
			for (int i = 0; i < len; ++i) {
				andPlane[i] = 0xff;
			}

			// 全 0
			byte[] xorPlane = new byte[len];

			(int xHotSpot, int yHotSpot) = GetCursorHotSpot(hotSpot);

			SafeCursorHandle rt = NativeMethods.CreateCursor(
				NativeMethods.GetModule(),
				Math.Min(xHotSpot, cursorSize.x),   // 获取的hotspot可能超出范围，不进行限制CreateCursor会失败
				Math.Min(yHotSpot, cursorSize.y),
				cursorSize.x, cursorSize.y,
				andPlane, xorPlane
			);

			if (rt == SafeCursorHandle.Zero) {
				Logger.Error($"创建透明光标失败\n\tWin32 错误代码：{Marshal.GetLastWin32Error()}");
			}

			return rt;
		}

		private (int, int) GetCursorHotSpot(IntPtr hCursor) {
			if (hCursor == IntPtr.Zero) {
				return (0, 0);
			}

			if (!NativeMethods.GetIconInfo(hCursor, out NativeMethods.ICONINFO ii)) {
				return (0, 0);
			}

			_ = NativeMethods.DeleteObject(ii.hbmMask);
			_ = NativeMethods.DeleteObject(ii.hbmColor);

			return ((int)ii.xHotspot, (int)ii.yHotspot);
		}

		// 向全屏窗口发送光标句柄
		private void ReportCursorMap(SafeCursorHandle hTptCursor, IntPtr hCursor) {
			if (!NativeMethods.PostMessage(hwndHost, MAGPIE_WM_NEWCURSOR, hTptCursor.DangerousGetHandle(), hCursor)) {
				Logger.Error($"PostMessage 失败\n\tWin32 错误代码：{Marshal.GetLastWin32Error()}");
			} else {
				Logger.Info($"已向全屏窗口发送新的映射：\n\t源光标：{hCursor}\n\t透明光标：{hTptCursor.DangerousGetHandle()}");
			}
		}

		// 向全屏窗口汇报至今为止的映射
		protected void ReportCursorMap() {
			foreach (KeyValuePair<IntPtr, SafeCursorHandle> item in hCursorToTptCursor) {
				// 排除系统光标
				if (IsBuiltInCursor(item.Key)) {
					continue;
				}

				ReportCursorMap(item.Value, item.Key);
			}
		}

		protected void ReplaceHCursors() {
			// 将窗口类中的 HCURSOR 替换为透明光标
			void ReplaceHCursor(IntPtr hWnd) {
				if (replacedHwnds.Contains(hWnd)) {
					return;
				}

				IntPtr hCursor = new IntPtr(NativeMethods.GetClassAuto(hWnd, NativeMethods.GCLP_HCURSOR));
				if (hCursor == IntPtr.Zero || IsBuiltInCursor(hCursor)) {
					// 不替换透明的系统光标
					return;
				}

				if (hCursorToTptCursor.ContainsKey(hCursor)) {
					// 之前已替换过
					SafeCursorHandle hTptCursor = hCursorToTptCursor[hCursor];

					// 替换窗口类的 HCURSOR
					if (NativeMethods.SetClassAuto(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor.DangerousGetHandle().ToInt64()) == hCursor.ToInt64()) {
						_ = replacedHwnds.Add(hWnd);
					} else {
						Logger.Error($"SetClassLongAuto 失败\n\tWin32 错误代码：{Marshal.GetLastWin32Error()}");
					}
				} else {
					// 以下代码如果出错不会有任何更改

					SafeCursorHandle hTptCursor = CreateTransparentCursor(hCursor);
					if (hTptCursor == SafeCursorHandle.Zero) {
						return;
					}

					// 替换窗口类的 HCURSOR
					if (NativeMethods.SetClassAuto(hWnd, NativeMethods.GCLP_HCURSOR, hTptCursor.DangerousGetHandle().ToInt64()) == hCursor.ToInt64()) {
						// 向全屏窗口发送光标句柄
						if (NativeMethods.PostMessage(hwndHost, MAGPIE_WM_NEWCURSOR, hTptCursor.DangerousGetHandle(), hCursor)) {
							// 替换成功
							_ = replacedHwnds.Add(hWnd);
							hCursorToTptCursor[hCursor] = hTptCursor;
						} else {
							_ = NativeMethods.SetClassAuto(hWnd, NativeMethods.GCLP_HCURSOR, hCursor.ToInt64());
							Logger.Error($"PostMessage 失败\n\tWin32 错误代码：{Marshal.GetLastWin32Error()}");
						}
					} else {
						Logger.Error($"SetClassLongAuto 失败\n\tWin32 错误代码：{Marshal.GetLastWin32Error()}");
					}
				}
			}

			// 替换源窗口和它的所有子窗口的窗口类的 HCRUSOR
			ReplaceHCursor(hwndSrc);
			_ = NativeMethods.EnumChildWindows(hwndSrc, (IntPtr hWnd, int _) => {
				ReplaceHCursor(hWnd);
				return true;
			}, IntPtr.Zero);

			Logger.Info("已替换源窗口和其所有子窗口的窗口类光标");

			// 向源窗口发送 WM_SETCURSOR，一般可以使其调用 SetCursor
			if (!NativeMethods.PostMessage(
				hwndSrc,
				NativeMethods.WM_SETCURSOR,
				hwndSrc,
				(IntPtr)NativeMethods.HTCLIENT
			)) {
				Logger.Error($"PostMessage 失败\n\tWin32 错误代码：{Marshal.GetLastWin32Error()}");
			}
		}

		protected void ReplaceHCursorsBack() {
			foreach (IntPtr hwnd in replacedHwnds) {
				IntPtr hCursor = new IntPtr(NativeMethods.GetClassAuto(hwnd, NativeMethods.GCLP_HCURSOR));
				if (hCursor == IntPtr.Zero || IsBuiltInCursor(hCursor)) {
					// 不替换透明的系统光标
					return;
				}

				// 在 _hCursorToTptCursor 中反向查找
				KeyValuePair<IntPtr, SafeCursorHandle> item = hCursorToTptCursor
					.FirstOrDefault(pair => pair.Value.DangerousGetHandle() == hCursor);

				if (item.Key == IntPtr.Zero || item.Value == SafeCursorHandle.Zero) {
					// 找不到就不替换
					continue;
				}

				_ = NativeMethods.SetClassAuto(hwnd, NativeMethods.GCLP_HCURSOR, item.Key.ToInt64());
			}

			replacedHwnds.Clear();

			Logger.Info("已还原源窗口和其所有子窗口的窗口类光标");
		}

		private bool IsBuiltInCursor(IntPtr hCursor) {
			return hCursor == arrowCursor
				   || hCursor == handCursor
				   || hCursor == appStartingCursor
				   || hCursor == iBeamCursor;
		}

		public void Dispose() {
			Dispose(true);
			GC.SuppressFinalize(this);
		}

		protected virtual void Dispose(bool disposing) {
			// 清理 HCURSOR
			foreach (SafeCursorHandle cursorHandle in hCursorToTptCursor.Values) {
				if (cursorHandle != null && !cursorHandle.IsInvalid) {
					cursorHandle.Dispose();
				}
			}
			// SafeHandle records the fact that we've called Dispose.
		}
	}
}
