using Newtonsoft.Json.Linq;
using System;
using System.IO;
using System.Linq;
using System.Threading;
using System.Windows;
using System.Windows.Resources;

namespace Magpie {
	internal class ScaleModelManager {
		private static NLog.Logger Logger { get; } = NLog.LogManager.GetCurrentClassLogger();

		private readonly FileSystemWatcher scaleModelsWatcher = new FileSystemWatcher();

		private ScaleModel[] scaleModels = null;

		public event Action ScaleModelsChanged;

		public ScaleModelManager() {
			LoadFromLocal();

			// 监视ScaleModels.json的更改
			scaleModelsWatcher.Path = App.APPLICATION_DIR;
			scaleModelsWatcher.NotifyFilter = NotifyFilters.LastWrite | NotifyFilters.FileName;
			scaleModelsWatcher.Filter = App.SCALE_MODELS_JSON_PATH.Substring(App.SCALE_MODELS_JSON_PATH.LastIndexOf('\\') + 1);
			scaleModelsWatcher.Changed += ScaleModelsWatcher_Changed;
			scaleModelsWatcher.Deleted += ScaleModelsWatcher_Changed;
			try {
				scaleModelsWatcher.EnableRaisingEvents = true;
				Logger.Info("正在监视 " + scaleModelsWatcher.Filter + " 的更改");
			} catch (FileNotFoundException e) {
				Logger.Error(e, "监视失败：" + scaleModelsWatcher.Filter + "不存在");
			}
		}

		public ScaleModel[] GetScaleModels() {
			return scaleModels;
		}

		public bool IsValid() {
			return scaleModels != null && scaleModels.Length > 0;
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
					Uri uri = new Uri("pack://application:,,,/Magpie;component/Resources/BuiltInScaleModels.json", UriKind.Absolute);
					StreamResourceInfo info = Application.GetResourceStream(uri);
					using (StreamReader reader = new StreamReader(info.Stream)) {
						json = reader.ReadToEnd();
					}
					File.WriteAllText(App.SCALE_MODELS_JSON_PATH, json);
					Logger.Info("已创建默认缩放配置文件");
				} catch (Exception e) {
					Logger.Error(e, "创建默认缩放配置文件失败");
				}
			}

			try {
				// 解析缩放配置
				scaleModels = JArray.Parse(json)
					 .Select(t => {
						 string name = t["name"]?.ToString(Newtonsoft.Json.Formatting.None);
						 string effects = t["effects"]?.ToString(Newtonsoft.Json.Formatting.None);
						 return name == null || effects == null
							 ? throw new Exception("未找到 name 或 model 属性")
							 : new ScaleModel {
								 Name = name.Substring(1, name.Length - 2),
								 Effects = effects
							 };
					 })
					 .ToArray();

				if (scaleModels.Length == 0) {
					throw new Exception("数组为空");
				}
			} catch (Exception e) {
				Logger.Error(e, "解析缩放配置失败");
				scaleModels = null;
			}

			if (ScaleModelsChanged != null) {
				ScaleModelsChanged.Invoke();
				Logger.Info("已引发 ScaleModelsChanged 事件");
			}
		}

		private void ScaleModelsWatcher_Changed(object sender, FileSystemEventArgs e) {
			Logger.Info("缩放配置文件已更改");

			// 立即读取可能会访问冲突
			Thread.Sleep(10);
			Application.Current.Dispatcher.Invoke(LoadFromLocal);
		}

		public class ScaleModel {
			public string Name { get; set; }

			public string Effects { get; set; }
		}
	}
}
