using Newtonsoft.Json.Linq;
using System;
using System.ComponentModel;
using System.IO;
using System.Threading;
using System.Windows.Threading;

namespace Magpie {
	internal class ScaleModelManager : BindingList<ScaleModelManager.ScaleModel> {
		private static NLog.Logger Logger { get; } = NLog.LogManager.GetCurrentClassLogger();

		private readonly FileSystemWatcher scaleModelsWatcher = new FileSystemWatcher();
		private readonly Dispatcher mainThreadDispatcher;

		public ScaleModelManager(Dispatcher dispatcher) {
			mainThreadDispatcher = dispatcher;

			// 监视ScaleModels.json的更改
			scaleModelsWatcher.Path = App.APPLICATION_DIR;
			scaleModelsWatcher.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.FileName;
			scaleModelsWatcher.Filter = App.SCALE_MODELS_JSON_PATH.Substring(App.SCALE_MODELS_JSON_PATH.LastIndexOf('\\') + 1);
			scaleModelsWatcher.Changed += ScaleModelsWatcher_Changed;
			scaleModelsWatcher.Deleted += ScaleModelsWatcher_Changed;
			try {
				scaleModelsWatcher.EnableRaisingEvents = true;
				Logger.Info("正在监视" + scaleModelsWatcher.Filter + "的更改");
			} catch (FileNotFoundException e) {
				Logger.Error(e, "监视失败：" + scaleModelsWatcher.Filter + "不存在");
			}

			LoadFromLocal();
		}

		private void LoadFromLocal() {
			string json = "";
			if (File.Exists(App.SCALE_MODELS_JSON_PATH)) {
				try {
					json = File.ReadAllText(App.SCALE_MODELS_JSON_PATH);
					Logger.Info("已读取缩放配置");
				} catch (Exception e) {
					Logger.Error(e, "读取缩放配置失败");
				}
			} else {
				try {
					File.WriteAllText(App.SCALE_MODELS_JSON_PATH, Properties.Resources.BuiltInScaleModels);
					Logger.Info("已创建默认缩放配置文件");
				} catch (Exception e) {
					Logger.Error(e, "创建默认缩放配置文件失败");
				}
				return;
			}

			try {
				// 解析缩放配置
				foreach (JToken item in JArray.Parse(json)) {
					string name = item["name"]?.ToString();
					string model = item["model"]?.ToString();
					if (name == null || model == null) {
						throw new Exception();
					}

					Add(new ScaleModel {
						Name = name,
						Model = model
					});
				}
			} catch (Exception e) {
				Logger.Error(e, "解析缩放配置失败");
				Clear();
			}
		}

		private void ScaleModelsWatcher_Changed(object sender, FileSystemEventArgs e) {
			Logger.Info("缩放配置文件已更改");

			// 立即读取可能会访问冲突
			Thread.Sleep(10);
			mainThreadDispatcher.Invoke(LoadFromLocal);
		}

		public class ScaleModel {
			public string Name { get; set; }

			public string Model { get; set; }
		}
	}
}
