﻿using System;
using System.Drawing;
using System.Windows.Forms;

namespace AuthClient.tollgate.account.dialog
{
    public partial class LoginButton : UserControl
    {
        public new event EventHandler Click
        {
            add
            {
                base.Click += value;
                foreach (Control control in Controls)
                {
                    control.Click += value;
                }
            }
            remove
            {
                base.Click -= value;
                foreach (Control control in Controls)
                {
                    control.Click -= value;
                }
            }
        }

        public LoginButton()
        {
            InitializeComponent();

            Font font = new Font(MainForm.FONT_BOLD, 11);
            label.Font = font;
        }
    }
}
