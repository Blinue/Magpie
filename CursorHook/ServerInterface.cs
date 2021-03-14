using System;


namespace Magpie.CursorHook {
    /// <summary>
    /// Provides an interface for communicating from the client (target) to the server (injector)
    /// </summary>
    public class ServerInterface : MarshalByRefObject {
        public void ReportMessages(string[] messages) {
            foreach (var msg in messages) {
                ReportMessage(msg);
            }
        }

        public void ReportMessage(string message) {
            Console.WriteLine("##CursorHook##: " + message);
        }

        /// <summary>
        /// Report exception
        /// </summary>
        /// <param name="e"></param>
        public void ReportException(Exception e) {
            Console.WriteLine("##CursorHook##: " + "IPC出错：" + e.ToString());
        }

        /// <summary>
        /// Called to confirm that the IPC channel is still open / host application has not closed
        /// </summary>
        public void Ping() {
        }
    }
}
