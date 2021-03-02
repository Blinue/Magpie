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

        [DllImport("Runtime.dll", CallingConvention = CallingConvention.StdCall)]
        [return: MarshalAs(UnmanagedType.LPWStr)]
        public static extern string GetLastErrorMsg();
    }
}
