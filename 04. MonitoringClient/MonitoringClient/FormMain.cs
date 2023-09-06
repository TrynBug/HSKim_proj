using System.ComponentModel.DataAnnotations;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text.Json;
using System.Text.Json.Nodes;
using MonitoringClient.Network;

namespace MonitoringClient
{
    public partial class FormMain : Form
    {
        public FormMain()
        {
            InitializeComponent();

            Config = null;
            DictControl = new Dictionary<int, List<ControlBase>>();
            ListControl = new List<ControlBase>();

            InitializeMainForm();
        }

        private void InitializeMainForm()
        {
            this.FormBorderStyle = FormBorderStyle.Sizable;
        }

        private void FormMain_Load(object sender, EventArgs e)
        {
            string configFile = "Config/config.json";
            string jsonString = File.ReadAllText(configFile);
            Config = JsonSerializer.Deserialize<CConfig>(jsonString);
            if (Config == null)
            {
                throw new SystemException("Failed to load config file.");
            }

            CreateMonitorControl();
            ConnectToMonitoringServer();
            RunUpdateThread();
        }



        private void FormMain_FormClosing(object sender, FormClosingEventArgs e)
        {
            _isClosed = true;

            if (NetClient != null)
                NetClient.Disconnect();
        }


        private void AddControlToDict(int? dataNumber, ControlBase control)
        {
            int key = dataNumber ?? -1;

            List<ControlBase>? listControl;
            if (DictControl.TryGetValue(key, out listControl) == false)
            {
                listControl = new List<ControlBase> { control };
                DictControl.Add(key, listControl);
            }
            else
            {
                listControl.Add(control);
            }
        }

        private void CreateMonitorControl()
        {
            if (Config?.ListMonitor == null)
                return;

            int padding = 10;
            // row 별 (현재 X좌표, 현재 Y좌표, 다음 row의 Y좌표) 데이터
            var listPosition = new List<(int X, int Y, int NextY)>();

            // 모든 컨트롤 생성
            foreach (MonitorConfig config in Config.ListMonitor)
            {
                if (listPosition.ElementAtOrDefault(config.Row) == default)
                {
                    // 현재 row 에 대한 첫번째 컨트롤 생성이면 좌표값을 초기화한다.
                    if (config.Row == 0)
                    {
                        listPosition.Add((padding, padding, padding + config.Height));
                    }
                    else
                    {
                        listPosition.Add((padding
                                          , padding + listPosition[config.Row - 1].NextY
                                          , padding + listPosition[config.Row - 1].NextY + config.Height));
                    }
                }

                // 컨트롤 생성
                ControlBase? control = null;
                EDisplayType eDisplayType = Enum.Parse<EDisplayType>(config.Display);
                switch (eDisplayType)
                {
                    case EDisplayType.EMPTY:
                        break;

                    case EDisplayType.ONOFF:
                        control = new ControlOnOff();
                        AddControlToDict(config.DataEnum, control);
                        break;
                    case EDisplayType.NUMBER:
                        control = new ControlNumber();
                        AddControlToDict(config.DataEnum, control);
                        break;
                    case EDisplayType.LINE_SINGLE:
                        control = new ControlLineSingle();
                        AddControlToDict(config.DataEnum, control);
                        break;

                    case EDisplayType.LINE_MULTI:
                        control = new ControlLineMulti();
                        if (config.ListMultiData != null)
                            foreach (MultiLineData data in config.ListMultiData)
                            {
                                AddControlToDict(data.DataEnum, control);
                                (control as ControlLineMulti)?.AddLine(data.DataEnum, data.DataName);
                            }
                        break;
                };

                if (control != null)
                {
                    ListControl.Add(control);
                    control.SetTitle(config.Title);
                    control.SetBackgroundColor(config.BackColor);
                    control.Location = new Point(listPosition[config.Row].X, listPosition[config.Row].Y);
                    control.Size = new Size(config.Width, config.Height);
                    control.Parent = this;
                    this.Controls.Add(control);
                }

                // 현재 X 좌표를 오른쪽으로 민다.
                listPosition[config.Row] = (listPosition[config.Row].X + config.Width + padding
                                            , listPosition[config.Row].Y
                                            , listPosition[config.Row].NextY);
            }
        }


        private async void ConnectToMonitoringServer()
        {
            NetClient = new Network.CClient(Config.MonitoringServerIP, Config.MonitoringServerPort, Config.LoginKey, Config.PacketHeaderCode, Config.PacketEncodeKey);
            bool connected = await NetClient.ConnectAsync();
            if (connected == false)
                return;

            CPacket packet = new CPacket();
            packet.Encode((ushort)en_PACKET_TYPE.en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN);
            packet.Encode(Config.LoginKey);
            await NetClient.SendPacket(packet);

            NetClient.RecvStart(FuncRecvIoCallback);

        }


        private void RunUpdateThread()
        {
            ThreadUpdate = new Thread(() => RoutineUpdateThread());
            ThreadUpdate?.Start();
        }


        private void FuncRecvIoCallback(CPacket packet)
        {
            ushort packetType;
            packet.Decode(out packetType);

            switch ((en_PACKET_TYPE)packetType)
            {
                case en_PACKET_TYPE.en_PACKET_CS_MONITOR_TOOL_RES_LOGIN:
                    break;

                case en_PACKET_TYPE.en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE:
                    {
                        byte serverNo;
                        byte dataType;
                        int dataValue;
                        int timeStamp;
                        packet.Decode(out serverNo).Decode(out dataType).Decode(out dataValue).Decode(out timeStamp);

                        List<ControlBase>? listControl;
                        if (DictControl.TryGetValue(dataType, out listControl) == false)
                            break;

                        long currentTime = GetCurrentTime();
                        foreach (ControlBase control in listControl)
                        {
                            // threadSafe
                            if (control.InvokeRequired)
                            {
                                control.Invoke(delegate { control.AddData((int)dataType, dataValue, currentTime); });
                            }
                            else
                            {
                                control.AddData((int)dataType, dataValue, currentTime);
                            }
                        }

                        break;
                    }

            }

        }



        private void RoutineUpdateThread()
        {
            //int sleepTime = 1000;
            int sleepTime = 100;
            long startTime = GetCurrentTime();
            long endTime = startTime;
            while (true)
            {
                if (_isClosed) break;
                Thread.Sleep(Math.Max(0, sleepTime - (int)(endTime - startTime)));
                if (_isClosed) break;
                startTime = GetCurrentTime();

                long currentTime = GetCurrentTime();
                foreach (ControlBase control in ListControl)
                {
                    // threadSafe
                    if (control.InvokeRequired)
                    {
                        control.Invoke(delegate { control.UpdateMonitor(currentTime); });
                    }
                    else
                    {
                        control.UpdateMonitor(currentTime);
                    }
                }

                endTime = GetCurrentTime();
            }
        }


        public long GetCurrentTime()
        {
            return DateTimeOffset.Now.ToUnixTimeMilliseconds();
        }


        private CConfig? Config { get; set; }
        private Dictionary<int, List<ControlBase>> DictControl { get; }
        private List<ControlBase> ListControl { get; }

        private CClient? NetClient { get; set; }
        private Thread? ThreadUpdate { get; set; }

        private volatile bool _isClosed;

    }
}