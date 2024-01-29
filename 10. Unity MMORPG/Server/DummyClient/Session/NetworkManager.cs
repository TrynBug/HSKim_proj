using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using ServerCore;
using System.Net;

namespace DummyClient
{
    public class NetworkManager
    {
        public static NetworkManager Instance { get; } = new NetworkManager();

        /* session */
        ServerSession _session = new ServerSession();

        public void Send(IMessage packet)
        {

            _session.Send(packet);

        }

        public void Init()
        {
            // Network Logger
            ServerCore.Logger.Level = LogLevel.Debug;

            // DNS (Domain Name System)
            string host = Dns.GetHostName();
            IPHostEntry ipHost = Dns.GetHostEntry(host);
            IPAddress ipAddr = ipHost.AddressList[0];
            IPEndPoint endPoint = new IPEndPoint(ipAddr, 7777);

            Connector connector = new Connector();
            connector.Connect(endPoint,
                () => { return _session; });
        }
    }
}
