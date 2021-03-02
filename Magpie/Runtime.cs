using System;
using System.Runtime.InteropServices;


namespace Magpie {
    class Runtime {
        [DllImport("Runtime.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern bool CreateMagWindow(
            uint frameRate,
            [MarshalAs(UnmanagedType.LPWStr)] string effectsJson,
            bool noDisturb = false
        );

        [DllImport("Runtime.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern void DestroyMagWindow();

        [DllImport("Runtime.dll", CallingConvention = CallingConvention.StdCall)]
        public static extern bool HasMagWindow();

        // 由于无法理解的原因，这里不能直接封送为 string
        // 见 https://stackoverflow.com/questions/15793736/difference-between-marshalasunmanagedtype-lpwstr-and-marshal-ptrtostringuni
        [DllImport("Runtime.dll", CallingConvention = CallingConvention.StdCall, EntryPoint = "GetLastErrorMsg")]
        private static extern IntPtr GetLastErrorMsgNative();

        public static string GetLastErrorMsg() {
            return Marshal.PtrToStringUni(GetLastErrorMsgNative());
        }
    }
}
