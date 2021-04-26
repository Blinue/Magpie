using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Magpie.CursorHook {
    // 无锁，需使用方同步
    class IpcServer {
        private readonly ServerInterface serverInterface;
        private readonly Queue<string> messageQueue = new Queue<string>();


        public IpcServer(string channelName) {
            serverInterface = EasyHook.RemoteHooking.IpcConnectClient<ServerInterface>(channelName);

            // 测试连接性，如果失败会抛出异常静默的失败因此 Run 方法不会执行
            serverInterface.Ping();
        }

        public void AddMessage(string msg) {
            if (messageQueue.Count < 1000) {
                messageQueue.Enqueue(msg);
            }
        }

        public void Send() {
            string[] queued = messageQueue.ToArray();
            messageQueue.Clear();

            if (queued != null && queued.Length > 0) {
                serverInterface.ReportMessages(queued);
            } else {
                serverInterface.Ping();
            }
        }
    }
}
