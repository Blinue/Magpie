
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
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(MainForm));
            this.lblHotkey = new System.Windows.Forms.Label();
            this.txtHotkey = new System.Windows.Forms.TextBox();
            this.label2 = new System.Windows.Forms.Label();
            this.cbbScaleMode = new System.Windows.Forms.ComboBox();
            this.ckbNoVSync = new System.Windows.Forms.CheckBox();
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.notifyIcon = new System.Windows.Forms.NotifyIcon(this.components);
            this.cmsNotifyIcon = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.tsmiHotkey = new System.Windows.Forms.ToolStripMenuItem();
            this.tsmiMainForm = new System.Windows.Forms.ToolStripMenuItem();
            this.tsmiExit = new System.Windows.Forms.ToolStripMenuItem();
            this.ckbShowFPS = new System.Windows.Forms.CheckBox();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.cbbInjectMode = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.openFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.cmsNotifyIcon.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.SuspendLayout();
            // 
            // lblHotkey
            // 
            this.lblHotkey.AutoSize = true;
            this.lblHotkey.Location = new System.Drawing.Point(48, 33);
            this.lblHotkey.Name = "lblHotkey";
            this.lblHotkey.Size = new System.Drawing.Size(37, 15);
            this.lblHotkey.TabIndex = 0;
            this.lblHotkey.Text = "热键";
            // 
            // txtHotkey
            // 
            this.txtHotkey.Location = new System.Drawing.Point(91, 30);
            this.txtHotkey.Name = "txtHotkey";
            this.txtHotkey.Size = new System.Drawing.Size(196, 25);
            this.txtHotkey.TabIndex = 1;
            this.txtHotkey.TextChanged += new System.EventHandler(this.TxtHotkey_TextChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(6, 30);
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
            this.cbbScaleMode.Location = new System.Drawing.Point(79, 27);
            this.cbbScaleMode.Name = "cbbScaleMode";
            this.cbbScaleMode.Size = new System.Drawing.Size(196, 23);
            this.cbbScaleMode.TabIndex = 2;
            this.cbbScaleMode.SelectedIndexChanged += new System.EventHandler(this.CbbScaleMode_SelectedIndexChanged);
            // 
            // ckbNoVSync
            // 
            this.ckbNoVSync.AutoSize = true;
            this.ckbNoVSync.Location = new System.Drawing.Point(79, 90);
            this.ckbNoVSync.Name = "ckbNoVSync";
            this.ckbNoVSync.Size = new System.Drawing.Size(119, 19);
            this.ckbNoVSync.TabIndex = 4;
            this.ckbNoVSync.Text = "关闭垂直同步";
            this.ckbNoVSync.UseVisualStyleBackColor = true;
            this.ckbNoVSync.CheckedChanged += new System.EventHandler(this.CkbNoVSync_CheckedChanged);
            // 
            // textBox1
            // 
            this.textBox1.BackColor = System.Drawing.SystemColors.Control;
            this.textBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.textBox1.Location = new System.Drawing.Point(12, 314);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(292, 43);
            this.textBox1.TabIndex = 7;
            this.textBox1.Text = "使用说明：\r\n按下热键即可全屏显示激活的窗口";
            // 
            // notifyIcon
            // 
            this.notifyIcon.ContextMenuStrip = this.cmsNotifyIcon;
            this.notifyIcon.Icon = ((System.Drawing.Icon)(resources.GetObject("notifyIcon.Icon")));
            this.notifyIcon.Text = "Magpie";
            this.notifyIcon.MouseClick += new System.Windows.Forms.MouseEventHandler(this.NotifyIcon_MouseClick);
            // 
            // cmsNotifyIcon
            // 
            this.cmsNotifyIcon.ImageScalingSize = new System.Drawing.Size(20, 20);
            this.cmsNotifyIcon.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.tsmiHotkey,
            this.tsmiMainForm,
            this.tsmiExit});
            this.cmsNotifyIcon.Name = "cmsNotifyIcon";
            this.cmsNotifyIcon.Size = new System.Drawing.Size(124, 76);
            // 
            // tsmiHotkey
            // 
            this.tsmiHotkey.Enabled = false;
            this.tsmiHotkey.Name = "tsmiHotkey";
            this.tsmiHotkey.Size = new System.Drawing.Size(123, 24);
            // 
            // tsmiMainForm
            // 
            this.tsmiMainForm.Name = "tsmiMainForm";
            this.tsmiMainForm.Size = new System.Drawing.Size(123, 24);
            this.tsmiMainForm.Text = "主界面";
            this.tsmiMainForm.Click += new System.EventHandler(this.TsmiMainForm_Click);
            // 
            // tsmiExit
            // 
            this.tsmiExit.Name = "tsmiExit";
            this.tsmiExit.Size = new System.Drawing.Size(123, 24);
            this.tsmiExit.Text = "退出";
            this.tsmiExit.Click += new System.EventHandler(this.TsmiExit_Click);
            // 
            // ckbShowFPS
            // 
            this.ckbShowFPS.AutoSize = true;
            this.ckbShowFPS.Location = new System.Drawing.Point(79, 65);
            this.ckbShowFPS.Name = "ckbShowFPS";
            this.ckbShowFPS.Size = new System.Drawing.Size(89, 19);
            this.ckbShowFPS.TabIndex = 8;
            this.ckbShowFPS.Text = "显示帧率";
            this.ckbShowFPS.UseVisualStyleBackColor = true;
            this.ckbShowFPS.CheckedChanged += new System.EventHandler(this.CkbShowFPS_CheckedChanged);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.cbbScaleMode);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.ckbShowFPS);
            this.groupBox1.Controls.Add(this.ckbNoVSync);
            this.groupBox1.Location = new System.Drawing.Point(12, 61);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(302, 121);
            this.groupBox1.TabIndex = 9;
            this.groupBox1.TabStop = false;
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.cbbInjectMode);
            this.groupBox2.Controls.Add(this.label1);
            this.groupBox2.Location = new System.Drawing.Point(12, 188);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(302, 69);
            this.groupBox2.TabIndex = 10;
            this.groupBox2.TabStop = false;
            // 
            // cbbInjectMode
            // 
            this.cbbInjectMode.FormattingEnabled = true;
            this.cbbInjectMode.Items.AddRange(new object[] {
            "不注入",
            "运行时注入",
            "启动时注入"});
            this.cbbInjectMode.Location = new System.Drawing.Point(79, 24);
            this.cbbInjectMode.Name = "cbbInjectMode";
            this.cbbInjectMode.Size = new System.Drawing.Size(196, 23);
            this.cbbInjectMode.TabIndex = 1;
            this.cbbInjectMode.SelectedIndexChanged += new System.EventHandler(this.CbbInjectMode_SelectedIndexChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 27);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(67, 15);
            this.label1.TabIndex = 0;
            this.label1.Text = "注入模式";
            // 
            // openFileDialog
            // 
            this.openFileDialog.Filter = "可执行文件|*.exe";
            this.openFileDialog.Title = "请选择要启动并注入的程序";
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(327, 369);
            this.Controls.Add(this.groupBox2);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.textBox1);
            this.Controls.Add(this.txtHotkey);
            this.Controls.Add(this.lblHotkey);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
            this.MaximizeBox = false;
            this.Name = "MainForm";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "Magpie";
            this.FormClosing += new System.Windows.Forms.FormClosingEventHandler(this.MainForm_FormClosing);
            this.Resize += new System.EventHandler(this.MainForm_Resize);
            this.cmsNotifyIcon.ResumeLayout(false);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblHotkey;
        private System.Windows.Forms.TextBox txtHotkey;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox cbbScaleMode;
        private System.Windows.Forms.CheckBox ckbNoVSync;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.NotifyIcon notifyIcon;
        private System.Windows.Forms.ContextMenuStrip cmsNotifyIcon;
        private System.Windows.Forms.ToolStripMenuItem tsmiMainForm;
        private System.Windows.Forms.ToolStripMenuItem tsmiExit;
        private System.Windows.Forms.ToolStripMenuItem tsmiHotkey;
        private System.Windows.Forms.CheckBox ckbShowFPS;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.ComboBox cbbInjectMode;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.OpenFileDialog openFileDialog;
    }
}

