using Microsoft.VisualBasic.Devices;
using ScottPlot;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MonitoringClient
{
    public partial class ControlNumber : ControlBase
    {
        public ControlNumber()
        {
            InitializeComponent();
            CurrentValue = 0;
            LastUpdateTime = GetCurrentTime();
            TitleFormat = "";

            labelValue.Text = "0";
        }

        public override void SetTitle(string title)
        {
            TitleFormat = title;
            labelTitle.Text = TitleFormat.Replace("%d", CurrentValue.ToString());
        }

        public override void SetBackgroundColor(string color)
        {
            ControlColor colorInfo = GetControlColor(color);
            panelTitle.BackColor = colorInfo.Title;
            panelResize.BackColor = colorInfo.Resize;
            this.BackColor = colorInfo.Background;
        }

        public override void AddData(int dataType, int dataValue, long time)
        {
            CurrentValue = dataValue;
            LastUpdateTime = time;
        }

        public override void UpdateMonitor(long time)
        {
            if (time - LastUpdateTime > ValidTimeGap)
            {
                CurrentValue = 0;
                LastUpdateTime = time;
            }

            labelTitle.Text = TitleFormat.Replace("%d", CurrentValue.ToString());
            labelValue.Text = CurrentValue.ToString();

            this.Update();
        }


        private void panelResize_MouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                PtResizeMouseDown = new Point(e.X, e.Y);
            }
        }

        private void panelResize_MouseMove(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                this.Size = new Size(this.Size.Width + e.X, this.Size.Height + e.Y);
            }
        }

        private void panelTitle_MouseDown(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                PtTitleMouseDown = new Point(-e.X, -e.Y);
            }
        }

        private void panelTitle_MouseMove(object sender, MouseEventArgs e)
        {
            if (e.Button == MouseButtons.Left)
            {
                this.Location = new Point(this.Location.X + (PtTitleMouseDown.X + e.X), this.Location.Y + (PtTitleMouseDown.Y + e.Y));
            }
        }

        private int CurrentValue { get; set; }
        private long LastUpdateTime { get; set; }
        private string TitleFormat { get; set; }
        private Point PtTitleMouseDown { get; set; }
        private Point PtResizeMouseDown { get; set; }
    }
}
