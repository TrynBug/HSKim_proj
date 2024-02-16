using Microsoft.VisualBasic.Devices;
using ScottPlot;
using ScottPlot.Plottable;
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
    public partial class ControlLineMulti : ControlBase
    {
        private const int paddingRight = 3;
        private const int paddingBottom = 5;
        private const int paddingTop = 5;

        private class Line
        {
            public int[] DataBuffer { get; set; }
            public string Name { get; set; }

            public int NumData { get; set; }
            public int NumTotalData { get; set; }
            public long LastUpdateTime { get; set; }
            public int LastData { get; set; }
            public int OldData { get; set; }
            public SignalPlotGeneric<int> SignalPlot { get; set; }

            public Line(string name)
            {
                DataBuffer = new int[SizeDataBuffer];
                Name = name;
                NumData = 0;
                NumTotalData = 0;
                LastUpdateTime = GetCurrentTime();
                LastData = 0;
                OldData = 0;
            }

            public void AddData(int dataValue)
            {
                LastUpdateTime = GetCurrentTime();

                // 데이터버퍼가 가득 찼으면 뒤쪽의 데이터를 앞쪽으로 옮긴다.
                if (NumData >= DataBuffer.Length)
                {
                    Array.Copy(DataBuffer, DataBuffer.Length - NumPlotData, DataBuffer, 0, NumPlotData);
                    NumData = NumPlotData;
                }

                // 데이터 추가
                DataBuffer[NumData] = dataValue;
                LastData = dataValue;
                NumData++;
                NumTotalData++;

                // 렌더링 대상데이터 지정
                SignalPlot.MinRenderIndex = NumData < NumPlotData ? 0 : NumData - NumPlotData;
                SignalPlot.MaxRenderIndex = NumData - 1;

                // 제거될 데이터 저장
                if (NumData > NumPlotData)
                    OldData = DataBuffer[NumData - NumPlotData - 1];
            }
        }



        public ControlLineMulti()
        {
            InitializeComponent();

            CurrentValue = 0;
            LastUpdateTime = GetCurrentTime();
            TitleFormat = "";
            MaxData = int.MinValue;
            MinData = int.MaxValue;

            DictLine = new Dictionary<int, Line>();

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

            formsPlot.Plot.SetAxisLimitsY(yMin: 0, yMax: 10);           // 초기 y축 범위 지정
            formsPlot.Plot.SetAxisLimitsX(xMin: 0, xMax: NumPlotData);  // 초기 x축 범위 지정

            Legend = plt.Legend();    // 범례 추가
            Legend.Orientation = ScottPlot.Orientation.Horizontal;
            Legend.Location = Alignment.UpperLeft;
            Legend.Padding = 0;

            formsPlot.Refresh();
        }


        public void AddLine(int dataType, string dataName)
        {
            Line line = new Line(dataName);
            DictLine.Add(dataType, line);

            line.SignalPlot = formsPlot.Plot.AddSignal<int>(line.DataBuffer, label: line.Name);   // 데이터버퍼 추가
            line.SignalPlot.MarkerShape = MarkerShape.none;                     // 데이터 마커 제거
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
            Legend.FontColor = colorInfo.Style.TickLabelColor;
            Legend.FillColor = colorInfo.Style.DataBackgroundColor;
            Legend.OutlineColor = colorInfo.Title;
        }

        public override void AddData(int dataType, int dataValue, long time)
        {
            Line? line;
            if (DictLine.TryGetValue(dataType, out line) == false)
                return;

            line.AddData(dataValue);

            MinData = Math.Min(MinData, dataValue);
            MaxData = Math.Max(MaxData, dataValue);

            CurrentValue = dataValue;
            LastUpdateTime = time;
        }

        //int update = 0;
        public override void UpdateMonitor(long time)
        {
            if (DictLine.Count == 0)
                return;

            foreach (var pair in DictLine)
            {
                Line line = pair.Value;
                if (time - line.LastUpdateTime > ValidTimeGap)
                {
                    AddData(pair.Key, 0, time);
                }
            }

            //// dummy data 추가
            //if (update < SizeDataBuffer)
            //{
            //    int idx = 0;
            //    foreach (var pair in DictLine)
            //    {
            //        AddData(pair.Key, (int)dummydata[idx][pair.Value.NumData], time);
            //        idx++;
            //    }
            //    update++;
            //}
            //else
            //{
            //    foreach (var pair in DictLine)
            //    {
            //        AddData(pair.Key, 0, time);
            //    }
            //}


            // refresh
            if (time - LastRefreshTime > RefreshInterval)
            {
                // 모든 Line의 데이터 수를 동일하게 맞춘다.
                int numTotalData = 0;
                int numData = 0;
                foreach (var pair in DictLine)
                {
                    Line line = pair.Value;
                    if (numTotalData < line.NumTotalData)
                        numTotalData = line.NumTotalData;
                }
                if (numTotalData == 0)
                    return;

                foreach (var pair in DictLine)
                {
                    Line line = pair.Value;
                    numData = line.NumData;
                    if (line.NumTotalData < numTotalData)
                    {
                        for (int i = 0; i < numTotalData - line.NumTotalData; i++)
                            AddData(pair.Key, line.LastData, time);
                    }
                }

                // x축 표시데이터 범위 조정
                Plot plt = formsPlot.Plot;
                plt.SetAxisLimitsX(xMin: numData < NumPlotData ? 0 : numData - NumPlotData
                                            , xMax: Math.Max(numData, NumPlotData));

                // y축 범위 조정
                if (numData > NumPlotData)
                {
                    // plot에서 사라진 데이터가 MinData 또는 MaxData 이었을 경우 MinData 또는 MaxData를 재계산한다.
                    bool bMinRenewed = false;
                    bool bMaxRenewed = false;
                    foreach (var pair in DictLine)
                    {
                        Line line = pair.Value;
                        if (bMinRenewed == false && line.OldData == MinData)
                        {
                            int minVal = int.MaxValue;
                            foreach (var pair2 in DictLine)
                            {
                                Line line2 = pair2.Value;
                                var slice = line2.DataBuffer.Skip(line2.NumData - NumPlotData).Take(NumPlotData);
                                int slice_min = slice.Min();
                                if (minVal > slice_min)
                                    minVal = slice_min;
                            }
                            MinData = minVal;
                            bMinRenewed = true;
                        }
                        else if (bMaxRenewed == false && line.OldData == MaxData)
                        {
                            int maxVal = int.MinValue;
                            foreach (var pair2 in DictLine)
                            {
                                Line line2 = pair2.Value;
                                var slice = line2.DataBuffer.Skip(line2.NumData - NumPlotData).Take(NumPlotData);
                                int slice_max = slice.Max();
                                if (maxVal < slice_max)
                                    maxVal = slice_max;
                            }
                            MaxData = maxVal;
                            bMaxRenewed = true;
                        }
                    }
                }
                UpperBoundY = Utils.CeilOfSameDigit(Utils.CeilOfSameDigit(MaxData));
                LowerBoundY = Utils.FloorOfSameDigit(MinData);
                plt.SetAxisLimitsY(yMin: LowerBoundY, yMax: UpperBoundY);   // y축 범위 조정


                // refresh
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

        private Dictionary<int, Line> DictLine { get; set; }

        private int MaxData { get; set; }
        private int MinData { get; set; }
        private int UpperBoundY { get; set; }
        private int LowerBoundY { get; set; }

        private int CurrentValue { get; set; }
        private string TitleFormat { get; set; }
        private long LastUpdateTime { get; set; }
        private long LastRefreshTime { get; set; }
        private Point PtTitleMouseDown { get; set; }
        private Point PtResizeMouseDown { get; set; }

        ScottPlot.Renderable.Legend Legend { get; set; }

        static int g_seed = (int)DateTimeOffset.Now.ToUnixTimeMilliseconds();
        double[][] dummydata = {
            DataGen.RandomWalk(new Random(g_seed++), SizeDataBuffer + 1, ((new Random()).Next(1, 10000) * (new Random()).NextDouble())),
            DataGen.RandomWalk(new Random(g_seed++), SizeDataBuffer + 1, ((new Random()).Next(1, 10000) * (new Random()).NextDouble())),
            DataGen.RandomWalk(new Random(g_seed++), SizeDataBuffer + 1, ((new Random()).Next(1, 10000) * (new Random()).NextDouble())),
        };
        //double[] dummydata = DataGen.RandomWalk(new Random(g_seed++), SizeDataBuffer + 1);
    }
}
