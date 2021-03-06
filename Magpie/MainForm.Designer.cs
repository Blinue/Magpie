﻿
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
            this.textBox1 = new System.Windows.Forms.TextBox();
            this.notifyIcon = new System.Windows.Forms.NotifyIcon(this.components);
            this.cmsNotifyIcon = new System.Windows.Forms.ContextMenuStrip(this.components);
            this.tsmiHotkey = new System.Windows.Forms.ToolStripMenuItem();
            this.tsmiScale = new System.Windows.Forms.ToolStripMenuItem();
            this.tsmiMainForm = new System.Windows.Forms.ToolStripMenuItem();
            this.tsmiExit = new System.Windows.Forms.ToolStripMenuItem();
            this.ckbShowFPS = new System.Windows.Forms.CheckBox();
            this.cbbInjectMode = new System.Windows.Forms.ComboBox();
            this.label1 = new System.Windows.Forms.Label();
            this.openFileDialog = new System.Windows.Forms.OpenFileDialog();
            this.cbbCaptureMode = new System.Windows.Forms.ComboBox();
            this.label3 = new System.Windows.Forms.Label();
            this.groupBox2 = new System.Windows.Forms.GroupBox();
            this.btnScale = new System.Windows.Forms.Button();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.timerScale = new System.Windows.Forms.Timer(this.components);
            this.cmsNotifyIcon.SuspendLayout();
            this.groupBox2.SuspendLayout();
            this.groupBox1.SuspendLayout();
            this.SuspendLayout();
            // 
            // lblHotkey
            // 
            this.lblHotkey.AutoSize = true;
            this.lblHotkey.Location = new System.Drawing.Point(12, 24);
            this.lblHotkey.Name = "lblHotkey";
            this.lblHotkey.Size = new System.Drawing.Size(37, 15);
            this.lblHotkey.TabIndex = 0;
            this.lblHotkey.Text = "热键";
            // 
            // txtHotkey
            // 
            this.txtHotkey.Location = new System.Drawing.Point(55, 21);
            this.txtHotkey.Name = "txtHotkey";
            this.txtHotkey.Size = new System.Drawing.Size(164, 25);
            this.txtHotkey.TabIndex = 1;
            this.txtHotkey.TextChanged += new System.EventHandler(this.TxtHotkey_TextChanged);
            // 
            // label2
            // 
            this.label2.AutoSize = true;
            this.label2.Location = new System.Drawing.Point(6, 27);
            this.label2.Name = "label2";
            this.label2.Size = new System.Drawing.Size(67, 15);
            this.label2.TabIndex = 2;
            this.label2.Text = "缩放模式";
            // 
            // cbbScaleMode
            // 
            this.cbbScaleMode.DropDownWidth = 230;
            this.cbbScaleMode.ItemHeight = 15;
            this.cbbScaleMode.Location = new System.Drawing.Point(79, 24);
            this.cbbScaleMode.Name = "cbbScaleMode";
            this.cbbScaleMode.Size = new System.Drawing.Size(187, 23);
            this.cbbScaleMode.TabIndex = 2;
            this.cbbScaleMode.SelectedIndexChanged += new System.EventHandler(this.CbbScaleMode_SelectedIndexChanged);
            // 
            // textBox1
            // 
            this.textBox1.BackColor = System.Drawing.SystemColors.Control;
            this.textBox1.BorderStyle = System.Windows.Forms.BorderStyle.None;
            this.textBox1.Location = new System.Drawing.Point(15, 194);
            this.textBox1.Multiline = true;
            this.textBox1.Name = "textBox1";
            this.textBox1.Size = new System.Drawing.Size(179, 48);
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
            this.tsmiScale,
            this.tsmiMainForm,
            this.tsmiExit});
            this.cmsNotifyIcon.Name = "cmsNotifyIcon";
            this.cmsNotifyIcon.Size = new System.Drawing.Size(148, 100);
            // 
            // tsmiHotkey
            // 
            this.tsmiHotkey.Enabled = false;
            this.tsmiHotkey.Name = "tsmiHotkey";
            this.tsmiHotkey.Size = new System.Drawing.Size(147, 24);
            // 
            // tsmiScale
            // 
            this.tsmiScale.Name = "tsmiScale";
            this.tsmiScale.Size = new System.Drawing.Size(147, 24);
            this.tsmiScale.Text = "5秒后放大";
            this.tsmiScale.Click += new System.EventHandler(this.TsmiScale_Click);
            // 
            // tsmiMainForm
            // 
            this.tsmiMainForm.Name = "tsmiMainForm";
            this.tsmiMainForm.Size = new System.Drawing.Size(147, 24);
            this.tsmiMainForm.Text = "主界面";
            this.tsmiMainForm.Click += new System.EventHandler(this.TsmiMainForm_Click);
            // 
            // tsmiExit
            // 
            this.tsmiExit.Name = "tsmiExit";
            this.tsmiExit.Size = new System.Drawing.Size(147, 24);
            this.tsmiExit.Text = "退出";
            this.tsmiExit.Click += new System.EventHandler(this.TsmiExit_Click);
            // 
            // ckbShowFPS
            // 
            this.ckbShowFPS.AutoSize = true;
            this.ckbShowFPS.Location = new System.Drawing.Point(17, 26);
            this.ckbShowFPS.Name = "ckbShowFPS";
            this.ckbShowFPS.Size = new System.Drawing.Size(89, 19);
            this.ckbShowFPS.TabIndex = 8;
            this.ckbShowFPS.Text = "显示帧率";
            this.ckbShowFPS.UseVisualStyleBackColor = true;
            this.ckbShowFPS.CheckedChanged += new System.EventHandler(this.CkbShowFPS_CheckedChanged);
            // 
            // cbbInjectMode
            // 
            this.cbbInjectMode.FormattingEnabled = true;
            this.cbbInjectMode.Items.AddRange(new object[] {
            "不注入",
            "运行时注入",
            "启动时注入"});
            this.cbbInjectMode.Location = new System.Drawing.Point(79, 82);
            this.cbbInjectMode.Name = "cbbInjectMode";
            this.cbbInjectMode.Size = new System.Drawing.Size(187, 23);
            this.cbbInjectMode.TabIndex = 1;
            this.cbbInjectMode.SelectedIndexChanged += new System.EventHandler(this.CbbInjectMode_SelectedIndexChanged);
            // 
            // label1
            // 
            this.label1.AutoSize = true;
            this.label1.Location = new System.Drawing.Point(6, 85);
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
            // cbbCaptureMode
            // 
            this.cbbCaptureMode.DropDownWidth = 187;
            this.cbbCaptureMode.ItemHeight = 15;
            this.cbbCaptureMode.Items.AddRange(new object[] {
            "WinRT Capture",
            "GDI"});
            this.cbbCaptureMode.Location = new System.Drawing.Point(79, 53);
            this.cbbCaptureMode.Name = "cbbCaptureMode";
            this.cbbCaptureMode.Size = new System.Drawing.Size(187, 23);
            this.cbbCaptureMode.TabIndex = 3;
            this.cbbCaptureMode.SelectedIndexChanged += new System.EventHandler(this.CbbCaptureMode_SelectedIndexChanged);
            // 
            // label3
            // 
            this.label3.AutoSize = true;
            this.label3.Location = new System.Drawing.Point(6, 56);
            this.label3.Name = "label3";
            this.label3.Size = new System.Drawing.Size(67, 15);
            this.label3.TabIndex = 4;
            this.label3.Text = "抓取模式";
            // 
            // groupBox2
            // 
            this.groupBox2.Controls.Add(this.ckbShowFPS);
            this.groupBox2.Location = new System.Drawing.Point(200, 182);
            this.groupBox2.Name = "groupBox2";
            this.groupBox2.Size = new System.Drawing.Size(116, 60);
            this.groupBox2.TabIndex = 12;
            this.groupBox2.TabStop = false;
            this.groupBox2.Text = "高级";
            // 
            // btnScale
            // 
            this.btnScale.AutoSize = true;
            this.btnScale.Location = new System.Drawing.Point(225, 21);
            this.btnScale.Name = "btnScale";
            this.btnScale.Size = new System.Drawing.Size(91, 25);
            this.btnScale.TabIndex = 13;
            this.btnScale.Text = "5秒后放大";
            this.btnScale.UseVisualStyleBackColor = true;
            this.btnScale.Click += new System.EventHandler(this.BtnScale_Click);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.cbbScaleMode);
            this.groupBox1.Controls.Add(this.label3);
            this.groupBox1.Controls.Add(this.cbbCaptureMode);
            this.groupBox1.Controls.Add(this.label1);
            this.groupBox1.Controls.Add(this.label2);
            this.groupBox1.Controls.Add(this.cbbInjectMode);
            this.groupBox1.Location = new System.Drawing.Point(15, 52);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(301, 121);
            this.groupBox1.TabIndex = 14;
            this.groupBox1.TabStop = false;
            // 
            // timerScale
            // 
            this.timerScale.Interval = 1000;
            this.timerScale.Tick += new System.EventHandler(this.TimerScale_Tick);
            // 
            // MainForm
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(8F, 15F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(331, 259);
            this.Controls.Add(this.groupBox1);
            this.Controls.Add(this.btnScale);
            this.Controls.Add(this.groupBox2);
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
            this.Load += new System.EventHandler(this.MainForm_Load);
            this.Resize += new System.EventHandler(this.MainForm_Resize);
            this.cmsNotifyIcon.ResumeLayout(false);
            this.groupBox2.ResumeLayout(false);
            this.groupBox2.PerformLayout();
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.Label lblHotkey;
        private System.Windows.Forms.TextBox txtHotkey;
        private System.Windows.Forms.Label label2;
        private System.Windows.Forms.ComboBox cbbScaleMode;
        private System.Windows.Forms.TextBox textBox1;
        private System.Windows.Forms.NotifyIcon notifyIcon;
        private System.Windows.Forms.ContextMenuStrip cmsNotifyIcon;
        private System.Windows.Forms.ToolStripMenuItem tsmiMainForm;
        private System.Windows.Forms.ToolStripMenuItem tsmiExit;
        private System.Windows.Forms.ToolStripMenuItem tsmiHotkey;
        private System.Windows.Forms.CheckBox ckbShowFPS;
        private System.Windows.Forms.ComboBox cbbInjectMode;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.OpenFileDialog openFileDialog;
        private System.Windows.Forms.ComboBox cbbCaptureMode;
        private System.Windows.Forms.Label label3;
        private System.Windows.Forms.GroupBox groupBox2;
        private System.Windows.Forms.Button btnScale;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.Timer timerScale;
        private System.Windows.Forms.ToolStripMenuItem tsmiScale;
    }
}

