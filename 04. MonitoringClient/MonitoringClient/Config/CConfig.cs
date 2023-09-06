using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MonitoringClient
{
	enum EServer
	{
        CHAT_SERVER = 1,
        GAME_SERVER = 2,
		LOGIN_SERVER = 3,
	}

	enum EDisplayType
	{
		EMPTY,
        ONOFF,
        NUMBER,
        LINE_SINGLE,
		LINE_MULTI,
	}

    internal class CConfig
    {
		public string MonitoringServerIP { get; set; }
		public ushort MonitoringServerPort { get; set; }
		public string LoginKey { get; set; }
		public byte PacketHeaderCode { get; set; }
		public byte PacketEncodeKey { get; set; }
		public bool AutoConnect { get; set; }
		public List<MonitorConfig>? ListMonitor { get; set; }

    }

    class MonitorConfig
    {
		public string Title { get; set; }
		public string Server { get; set; }
		public string Display { get; set; }
		public int? DataEnum { get; set; }
		public string BackColor { get; set; }
		public int Row { get; set; }
		public int Width { get; set; }
		public int Height { get; set; }
        public List<MultiLineData>? ListMultiData { get; set; }
    }

    class MultiLineData
    {
        public int DataEnum { get; set; }
        public string DataName { get; set; }
    }
}
