using Microsoft.VisualBasic.Devices;
using ScottPlot;
using ScottPlot.Plottable;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace MonitoringClient
{
    public partial class ControlLineSingle : ControlBase
    {
        private const int paddingRight = 3;
        private const int paddingBottom = 5;
        private const int paddingTop = 5;

        public ControlLineSingle()
        {
            InitializeComponent();
            CurrentValue = 0;
            LastUpdateTime = GetCurrentTime();
            TitleFormat = "";
            DataBuffer = new int[SizeDataBuffer];
            NumData = 0;
            MaxData = int.MinValue;
            MinData = int.MaxValue;

            InitializePlot();
        }

        private void InitializePlot()
        {
            Plot plt = formsPlot.Plot;
            plt.XAxis.Ticks(false);  // x축 tick 제거
            plt.YAxis.AxisTicks.MajorTickVisible = false;  // y 축 tick 눈금 제거
            plt.YAxis.AxisTicks.MinorTickVisible = false;

            plt.YAxis.Color(Color.WhiteSmoke);  // y 축 tick 색상 설정

            plt.XAxis.Line(false);   // plot의 아래, 오른쪽, 위 border 제거
            plt.YAxis2.Line(false);
            plt.XAxis2.Line(false);

            plt.XAxis.Layout(minimumSize: paddingBottom, maximumSize: paddingBottom);    // padding 제거
            plt.XAxis2.Layout(minimumSize: paddingTop, maximumSize: paddingTop);
            plt.YAxis2.Layout(minimumSize: paddingRight, maximumSize: paddingRight);
            plt.Layout(left: null, right: paddingRight, bottom: paddingBottom, top: paddingTop, padding: 0);

            plt.XAxis.MajorGrid(enable: false);  // plot의 x축 grid 제거

            SignalPlot = formsPlot.Plot.AddSignal<int>(DataBuffer);     // 데이터버퍼 지정
            SignalPlot.MarkerShape = MarkerShape.none;                  // 데이터 마커 제거
            formsPlot.Plot.SetAxisLimitsY(yMin: 0, yMax: 10);           // 초기 y축 범위 지정
            formsPlot.Plot.SetAxisLimitsX(xMin: 0, xMax: NumPlotData);  // 초기 x축 범위 지정

            formsPlot.Refresh();
        }

        override public void SetTitle(string title)
        {
            TitleFormat = title;
            labelTitle.Text = TitleFormat.Replace("%d", CurrentValue.ToString());
        }


        public override void SetBackgroundColor(string color)
        {
            ControlColor colorInfo = GetControlColor(color);
            panelTitle.BackColor = colorInfo.Title;
            this.BackColor = colorInfo.Background;
            panelResize.BackColor = colorInfo.Resize;
            formsPlot.Plot.Style(colorInfo.Style);
        }


        public override void AddData(int dataType, int dataValue, long time)
        {
            // 데이터버퍼가 가득 찼으면 뒤쪽의 데이터를 앞쪽으로 옮긴다.
            if (NumData >= DataBuffer.Length)
            {
                Array.Copy(DataBuffer, DataBuffer.Length - NumPlotData, DataBuffer, 0, NumPlotData);
                NumData = NumPlotData;
            }

            // 데이터 추가
            DataBuffer[NumData] = dataValue;
            NumData++;
            MinData = Math.Min(MinData, dataValue);
            MaxData = Math.Max(MaxData, dataValue);
            

            // x축 표시데이터 범위 조정
            Plot plt = formsPlot.Plot;
            SignalPlot.MinRenderIndex = NumData < NumPlotData ? 0 : NumData - NumPlotData;
            SignalPlot.MaxRenderIndex = NumData - 1;
            plt.SetAxisLimitsX(xMin: NumData < NumPlotData ? 0 : NumData - NumPlotData
                                        , xMax: Math.Max(NumData, NumPlotData));

            // y축 범위 조정
            int oldData;
            if (NumData > NumPlotData)
            {
                // plot에서 사라진 데이터가 MinData 또는 MaxData 이었을 경우 MinData 또는 MaxData를 재계산한다.
                oldData = DataBuffer[NumData - NumPlotData - 1];
                if (oldData == MinData)
                {
                    var slice = DataBuffer.Skip(NumData - NumPlotData).Take(NumPlotData);
                    MinData = slice.Min();
                }
                if (oldData == MaxData)
                {
                    var slice = DataBuffer.Skip(NumData - NumPlotData).Take(NumPlotData);
                    MaxData = slice.Max();
                }
            }
            UpperBoundY = Utils.CeilOfSameDigit(MaxData);
            LowerBoundY = Utils.FloorOfSameDigit(MinData); 
            plt.SetAxisLimitsY(yMin: LowerBoundY, yMax: UpperBoundY);   // y축 범위 조정

            // 객체 상태 갱신
            CurrentValue = dataValue;
            LastUpdateTime = time;
        }

        //int update = 0;
        public override void UpdateMonitor(long time)
        {
            if (time - LastUpdateTime > ValidTimeGap)
            {
                AddData(0, 0, time);
            }

            //// dummy data 추가
            //if(update < SizeDataBuffer)
            //    AddData(0, (int)dummydata[NumData], time);
            //else
            //    AddData(0, 0, time);
            //update++;

            // refresh
            if (time - LastRefreshTime > RefreshInterval)
            {
                labelTitle.Text = TitleFormat.Replace("%d", CurrentValue.ToString());
                LastRefreshTime = time;
                formsPlot.Refresh();
                this.Update();
            }
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


        SignalPlotGeneric<int> SignalPlot { get; set; }

        private int[] DataBuffer { get; set; }
        private int NumData { get; set; }
        private int MaxData { get; set; }
        private int MinData { get; set; }
        private int UpperBoundY { get; set; }
        private int LowerBoundY { get; set; }

        private int CurrentValue { get; set; }
        private long LastUpdateTime { get; set; }
        private long LastRefreshTime { get; set; }
        private string TitleFormat { get; set; }
        private Point PtTitleMouseDown { get; set; }
        private Point PtResizeMouseDown { get; set; }

        static int g_seed = (int)DateTimeOffset.Now.ToUnixTimeMilliseconds();
        double[] dummydata = DataGen.RandomWalk(new Random(g_seed++), SizeDataBuffer + 1, ((new Random()).Next(1, 10000) * (new Random()).NextDouble()));
        //double[] dummydata = DataGen.RandomWalk(new Random(g_seed++), SizeDataBuffer + 1);
    }
}
