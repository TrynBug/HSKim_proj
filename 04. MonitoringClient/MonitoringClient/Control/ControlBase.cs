using ScottPlot.Drawing.Colormaps;
using ScottPlot;
using ScottPlot.Styles;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Drawing;
using System.Drawing.Imaging;

namespace MonitoringClient
{
    public enum EBackgroundColor
    {
        DEFAULT,
        BLACK,
        BLUE1,
        BLUE2,
        BLUE3,
        BURGUNDY,
        CONTROL,
        GRAY1,
        GRAY2,
        HAZEL,
        LIGHT1,
        LIGHT2,
        MONOSPACE,
        PINK,
        SEABORN,
    }

    public readonly struct ControlColor
    {
        public Color Title { get; }
        public Color Background { get; }
        public Color Resize { get; }
        public IStyle Style { get; }
        public ControlColor(IStyle style)
        {
            Title = ControlPaint.Dark(style.FigureBackgroundColor, 0.1f);
            Background = style.FigureBackgroundColor;
            Resize = ControlPaint.Light(style.DataBackgroundColor, 0.1f);
            Style = style;
        }
    }

    public class ControlBase : UserControl
    {
        /* property */
        public const int SizeDataBuffer = 1200;
        public const int NumPlotData = 400;
        public const int ValidTimeGap = 1100;
        public const int RefreshInterval = 1000;


        /* virtual method */
        public virtual void SetTitle(string title) { }
        public virtual void SetBackgroundColor(string color) { }
        public virtual void UpdateMonitor(long time) { }
        public virtual void AddData(int dataType, int dataValue, long time) { }

        /* method */
        static public long GetCurrentTime()
        {
            return DateTimeOffset.Now.ToUnixTimeMilliseconds();
        }

        static public ControlColor GetControlColor(string color)
        {
            EBackgroundColor eColor = EBackgroundColor.DEFAULT;
            try
            {
                eColor = (EBackgroundColor)Enum.Parse(typeof(EBackgroundColor), color);
            }
            catch (Exception)
            {
                eColor = EBackgroundColor.DEFAULT;
            }
            return GetControlColor(eColor);
        }

        static public ControlColor GetControlColor(EBackgroundColor eColor)
        {
            ControlColor color = eColor switch
            {
                EBackgroundColor.BLACK      => new ControlColor(Style.Black),
                EBackgroundColor.BLUE1      => new ControlColor(Style.Blue1),
                EBackgroundColor.BLUE2      => new ControlColor(Style.Blue2),
                EBackgroundColor.BLUE3      => new ControlColor(Style.Blue3),
                EBackgroundColor.BURGUNDY   => new ControlColor(Style.Burgundy),
                EBackgroundColor.CONTROL    => new ControlColor(Style.Control),
                EBackgroundColor.GRAY1      => new ControlColor(Style.Gray1),
                EBackgroundColor.GRAY2      => new ControlColor(Style.Gray2),
                EBackgroundColor.HAZEL      => new ControlColor(Style.Hazel),
                EBackgroundColor.LIGHT1     => new ControlColor(Style.Light1),
                EBackgroundColor.LIGHT2     => new ControlColor(Style.Light2),
                EBackgroundColor.MONOSPACE  => new ControlColor(Style.Monospace),
                EBackgroundColor.PINK       => new ControlColor(Style.Pink),
                EBackgroundColor.SEABORN    => new ControlColor(Style.Seaborn),
                _                           => new ControlColor(Style.Default)
            };

            return color;
        }
    }
}
