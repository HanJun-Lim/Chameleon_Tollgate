﻿
namespace AuthClient.tollgate.home.main.dialog
{
    partial class CheckDialog
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.panel_table = new System.Windows.Forms.TableLayoutPanel();
            this.img_title = new System.Windows.Forms.PictureBox();
            this.panel_list = new System.Windows.Forms.Panel();
            this.img_bg = new System.Windows.Forms.PictureBox();
            this.img_button = new System.Windows.Forms.PictureBox();
            this.panel_table.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.img_title)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.img_bg)).BeginInit();
            ((System.ComponentModel.ISupportInitialize)(this.img_button)).BeginInit();
            this.SuspendLayout();
            // 
            // panel_table
            // 
            this.panel_table.ColumnCount = 4;
            this.panel_table.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 67F));
            this.panel_table.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 300F));
            this.panel_table.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.panel_table.ColumnStyles.Add(new System.Windows.Forms.ColumnStyle(System.Windows.Forms.SizeType.Absolute, 287F));
            this.panel_table.Controls.Add(this.img_title, 1, 1);
            this.panel_table.Controls.Add(this.panel_list, 1, 3);
            this.panel_table.Controls.Add(this.img_bg, 3, 0);
            this.panel_table.Controls.Add(this.img_button, 2, 4);
            this.panel_table.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel_table.Location = new System.Drawing.Point(0, 0);
            this.panel_table.Margin = new System.Windows.Forms.Padding(0);
            this.panel_table.Name = "panel_table";
            this.panel_table.RowCount = 6;
            this.panel_table.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 70F));
            this.panel_table.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 60F));
            this.panel_table.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 60F));
            this.panel_table.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Percent, 100F));
            this.panel_table.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 44F));
            this.panel_table.RowStyles.Add(new System.Windows.Forms.RowStyle(System.Windows.Forms.SizeType.Absolute, 12F));
            this.panel_table.Size = new System.Drawing.Size(800, 443);
            this.panel_table.TabIndex = 0;
            // 
            // img_title
            // 
            this.panel_table.SetColumnSpan(this.img_title, 2);
            this.img_title.Dock = System.Windows.Forms.DockStyle.Fill;
            this.img_title.Image = global::AuthClient.Properties.Resources.checkTitle;
            this.img_title.Location = new System.Drawing.Point(67, 70);
            this.img_title.Margin = new System.Windows.Forms.Padding(0);
            this.img_title.Name = "img_title";
            this.img_title.Size = new System.Drawing.Size(446, 60);
            this.img_title.TabIndex = 1;
            this.img_title.TabStop = false;
            // 
            // panel_list
            // 
            this.panel_list.AutoScroll = true;
            this.panel_table.SetColumnSpan(this.panel_list, 2);
            this.panel_list.Dock = System.Windows.Forms.DockStyle.Fill;
            this.panel_list.Location = new System.Drawing.Point(67, 190);
            this.panel_list.Margin = new System.Windows.Forms.Padding(0);
            this.panel_list.Name = "panel_list";
            this.panel_list.Size = new System.Drawing.Size(446, 197);
            this.panel_list.TabIndex = 4;
            // 
            // img_bg
            // 
            this.img_bg.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
            this.img_bg.Image = global::AuthClient.Properties.Resources.checkBg;
            this.img_bg.Location = new System.Drawing.Point(513, 22);
            this.img_bg.Margin = new System.Windows.Forms.Padding(0);
            this.img_bg.Name = "img_bg";
            this.panel_table.SetRowSpan(this.img_bg, 5);
            this.img_bg.Size = new System.Drawing.Size(287, 409);
            this.img_bg.TabIndex = 3;
            this.img_bg.TabStop = false;
            // 
            // img_button
            // 
            this.img_button.Image = global::AuthClient.Properties.Resources.checkButton;
            this.img_button.Location = new System.Drawing.Point(367, 387);
            this.img_button.Margin = new System.Windows.Forms.Padding(0);
            this.img_button.Name = "img_button";
            this.img_button.Size = new System.Drawing.Size(66, 32);
            this.img_button.TabIndex = 5;
            this.img_button.TabStop = false;
            this.img_button.Visible = false;
            this.img_button.Click += new System.EventHandler(this.img_button_Click);
            // 
            // CheckDialog
            // 
            this.AutoScaleDimensions = new System.Drawing.SizeF(7F, 12F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.BackColor = System.Drawing.Color.White;
            this.ClientSize = new System.Drawing.Size(800, 443);
            this.Controls.Add(this.panel_table);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.Name = "CheckDialog";
            this.Text = "CheckDialog";
            this.Load += new System.EventHandler(this.CheckDialog_Load);
            this.Shown += new System.EventHandler(this.CheckDialog_Shown);
            this.panel_table.ResumeLayout(false);
            ((System.ComponentModel.ISupportInitialize)(this.img_title)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.img_bg)).EndInit();
            ((System.ComponentModel.ISupportInitialize)(this.img_button)).EndInit();
            this.ResumeLayout(false);

        }

        #endregion

        private System.Windows.Forms.PictureBox img_bg;
        private System.Windows.Forms.TableLayoutPanel panel_table;
        private System.Windows.Forms.PictureBox img_title;
        private System.Windows.Forms.Panel panel_list;
        private System.Windows.Forms.PictureBox img_button;
    }
}