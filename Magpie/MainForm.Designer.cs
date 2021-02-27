
namespace Magpie {
    partial class MainForm {
        /// <summary>
        /// 必需的设计器变量。
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// 清理所有正在使用的资源。
        /// </summary>
        /// <param name="disposing">如果应释放托管资源，为 true；否则为 false。</param>
        protected override void Dispose(bool disposing) {
            if (disposing && (components != null)) {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows 窗体设计器生成的代码

        /// <summary>
        /// 设计器支持所需的方法 - 不要修改
        /// 使用代码编辑器修改此方法的内容。
        /// </summary>
        private void InitializeComponent() {
            this.lblHotkey = new System.Windows.Forms.Label();
            this.txtHotkey = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.cbbScaleMode = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.tkbFrameRate = new System.Windows.Forms.TrackBar();
            this.lblFrameRate = new System.Windows.Forms.Label();
            this.ckbMaxFrameRate = new System.Windows.Forms.CheckBox();
            this.textBox1 = new System.Windows.Forms.TextBox();
            ((System.ComponentModel.ISupportInitialize)(this.tkbFrameRate)).BeginInit();
            this.SuspendLayout();
            // 
            // lblHotkey
            // 
            this.lblHotkey.AutoSize = true;
            this.lblHotkey.Location = new System.Drawing.Point(42, 26);
            this.lblHotkey.Name = "lblHotkey";
            this.lblHotkey.Size = new System.Drawing.Size(37, 15);
            this.lblHotkey.TabIndex = 0;
            this.lblHotkey.Text = "热键";
            // 
            // txtHotkey
            // 
            this.txtHotkey.Location = new System.Drawing.Point(85, 23);
            this.txtHotkey.Name = "txtHotkey";
            this.txtHotkey.Size = new System.Drawing.Size(196, 25);
            this.txtHotkey.TabIndex = 1;
            this.txtHotkey.TextChanged += new System.EventHandler(this.TxtHotkey_TextChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(12, 69);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(67, 15);
            this.label2.TabIndex = 2;
            this.label2.Text = "缩放模式";
            // 
            // cbbScaleMode
            // 
            this.cbbScaleMode.DropDownWidth = 196;
            this.cbbScaleMode.ItemHeight = 15;
            this.cbbScaleMode.Items.AddRange(new object[] {
            "通用（jinc2+锐化）",
            "动漫（Anime4K）"});
            this.cbbScaleMode.Location = new System.Drawing.Point(85, 66);
            this.cbbScaleMode.Name = "cbbScaleMode";
            this.cbbScaleMode.Size = new System.Drawing.Size(196, 23);
            this.cbbScaleMode.TabIndex = 2;
            this.cbbScaleMode.SelectedIndexChanged += new System.EventHandler(this.CbbScaleMode_SelectedIndexChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(42, 119);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(37, 15);
            this.label3.TabIndex = 4;
            this.label3.Text = "帧率";
            // 
            // tkbFrameRate
            // 
            this.tkbFrameRate.LargeChange = 10;
            this.tkbFrameRate.Location = new System.Drawing.Point(85, 105);
            this.tkbFrameRate.Maximum = 120;
            this.tkbFrameRate.Minimum = 30;
            this.tkbFrameRate.Name = "tkbFrameRate";
            this.tkbFrameRate.Size = new System.Drawing.Size(196, 56);
            this.tkbFrameRate.SmallChange = 5;
            this.tkbFrameRate.TabIndex = 3;
            this.tkbFrameRate.TickFrequency = 10;
            this.tkbFrameRate.Value = 60;
            this.tkbFrameRate.Scroll += new System.EventHandler(this.TkbFrameRate_Scroll);
            // 
            // lblFrameRate
            // 
            this.lblFrameRate.AutoSize = true;
            this.lblFrameRate.Location = new System.Drawing.Point(93, 147);
            this.lblFrameRate.Name = "lblFrameRate";
            this.lblFrameRate.Size = new System.Drawing.Size(23, 15);
            this.lblFrameRate.TabIndex = 6;
            this.lblFrameRate.Text = "60";
            // 
            // ckbMaxFrameRate
            // 
            this.ckbMaxFrameRate.AutoSize = true;
            this.ckbMaxFrameRate.Location = new System.Drawing.Point(207, 146);
            this.ckbMaxFrameRate.Name = "ckbMaxFrameRate";
            this.ckbMaxFrameRate.Size = new System.Drawing.Size(74, 19);
            this.ckbMaxFrameRate.TabIndex = 4;
            this.ckbMaxFrameRate.Text = "无限制";
            this.ckbMaxFrameRate.UseVisualStyleBackColor = true;
            this.ckbMaxFrameRate.CheckedChanged += new System.EventHandler(this.CkbMaxFrameRate_CheckedChanged);
            // 
            // textBox1
            // 
            this.textBox1.BackColor = System.Drawing.SystemColors.Control;
            this.textBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.textBox1.Location = new System.Drawing.Point(12, 194);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(292, 43);
            this.textBox1.TabIndex = 7;
            this.textBox1.Text = "使用说明：\r\n按下热键即可全屏显示激活的窗口";
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(316, 249);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.ckbMaxFrameRate);
            this.Controls.Add(this.lblFrameRate);
            this.Controls.Add(this.tkbFrameRate);
            this.Controls.Add(this.label3);
            this.Controls.Add(this.cbbScaleMode);
            this.Controls.Add(this.label2);
            this.Controls.Add(this.txtHotkey);
            this.Controls.Add(this.lblHotkey);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Magpie";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainForm_FormClosing);
            ((System.ComponentModel.ISupportInitialize)(this.tkbFrameRate)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblHotkey;
        private System.Windows.Forms.TextBox txtHotkey;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox cbbScaleMode;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.TrackBar tkbFrameRate;
        private System.Windows.Forms.Label lblFrameRate;
        private System.Windows.Forms.CheckBox ckbMaxFrameRate;
        private System.Windows.Forms.TextBox textBox1;
    }
}

