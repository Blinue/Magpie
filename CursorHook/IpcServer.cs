using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Magpie.CursorHook {
    // 无锁，需使用方同步
    class IpcServer {
        private ServerInterface serverInterface = null;
        private Queue<string> messageQueue = null;

        public bool IsConnected { get; private set; }

        // channelName 可以为 null，此时本类的方法不起作用
        public IpcServer(string channelName) {
            if(string.IsNullOrEmpty(channelName)) {
                IsConnected = false;
                return;
            }
            IsConnected = true;

            serverInterface = EasyHook.RemoteHooking.IpcConnectClient<ServerInterface>(channelName);
            messageQueue = new Queue<string>();

            // 测试连接性，如果失败会抛出异常静默的失败因此 Run 方法不会执行
            Send();
        }

        public void AddMessage(string msg) {
            if (!IsConnected) {
                return;
            }

            if (messageQueue.Count < 1000) {
                messageQueue.Enqueue(msg);
            }
        }

        public void Send() {
            if(!IsConnected) {
                return;
            }

            string[] queued = messageQueue.ToArray();
            messageQueue.Clear();

            try {
                if (queued != null && queued.Length > 0) {
                    serverInterface.ReportMessages(queued);
                } else {
                    serverInterface.Ping();
                }
            } catch {
                // IPC 服务器已关闭
                // 只会在调试环境下发生
                IsConnected = false;
                serverInterface = null;
                messageQueue = null;
            }
        }
    }
}
