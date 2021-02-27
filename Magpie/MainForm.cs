using System;
using System.Collections.Generic;
using System.Windows.Forms;
using Gma.System.MouseKeyHook;

namespace Magpie {
    public partial class MainForm : Form {
        public MainForm() {
            InitializeComponent();

            Hook.GlobalEvents().OnCombination(new Dictionary<Combination, Action> {{
                Combination.FromString("Alt+F11"), () => {
                    if(!Runtime.HasMagWindow()) {
                        Runtime.CreateMagWindow(0, @"[
  {
    ""effect"": ""scale"",
    ""type"": ""Anime4KxDenoise""
  },
  {
    ""effect"": ""scale"",
    ""type"": ""HQBicubic"",
    ""scale"": [0,0],
    ""sharpness"": 1
  }
]", false);
                    } else {
                        Runtime.DestroyMagWindow();
                    }
                }
             }});
        }
    }
}
