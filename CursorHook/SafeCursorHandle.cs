using System;
using Microsoft.Win32.SafeHandles;


namespace Magpie.CursorHook {
	// HCURSOR 的安全包装
	internal class SafeCursorHandle : SafeHandleZeroOrMinusOneIsInvalid {
		public SafeCursorHandle() : base(true) {
		}

		public SafeCursorHandle(IntPtr handle, bool ownsHandle = true) : base(ownsHandle) {
			SetHandle(handle);
		}

		protected override bool ReleaseHandle() {
			return NativeMethods.DestroyCursor(handle);
		}

		public override bool Equals(object obj) {
			if (obj == null || GetType() != obj.GetType()) {
				return false;
			}

			SafeCursorHandle other = (SafeCursorHandle)obj;
			return handle == other.handle;
		}

		public override int GetHashCode() {
			return handle.ToInt32();
		}

		public static readonly SafeCursorHandle Zero = new SafeCursorHandle(IntPtr.Zero, false);
	}
}
