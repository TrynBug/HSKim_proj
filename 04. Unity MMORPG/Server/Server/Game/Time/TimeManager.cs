using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    public static class TimeManager
    {
        public static long ServerTime { get { return DateTime.Now.Ticks; } }
        public static long OneSecondTick { get { return TimeSpan.TicksPerSecond; } }
    }
}
