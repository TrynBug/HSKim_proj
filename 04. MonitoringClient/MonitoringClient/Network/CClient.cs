using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace MonitoringClient.Network
{
    internal class CClient
    {
        public CClient(string serverIP, ushort serverPort, string loginKey, byte packetHeaderCode, byte packetEncodeKey)
        {
            ServerIP = serverIP;
            ServerPort = serverPort;
            LoginKey = loginKey;
            PacketHeaderCode = packetHeaderCode;
            PacketEncodeKey = packetEncodeKey;
        }

        public async Task<bool> ConnectAsync()
        {
            string hn = Dns.GetHostName();
            IPAddress[] localIPs = Dns.GetHostAddresses(hn);
            IPEndPoint? clientAddress = null;
            foreach (IPAddress ip in localIPs)
            {
                if (ip.AddressFamily == AddressFamily.InterNetwork)
                    clientAddress = new IPEndPoint(ip, 0);
            }

            if (clientAddress == null)
                return false;

            _client = new TcpClient(clientAddress);
            IPEndPoint serverAddress = new IPEndPoint(IPAddress.Parse(ServerIP), ServerPort);
            while(true)
            {
                try
                {
                    await _client.ConnectAsync(serverAddress);
                    break;
                }
                catch (Exception)
                {
                    await Task.Delay(1000);
                }
            }
            return true;
        }

        public void Disconnect()
        {
            if (_isDisconnected == false)
            {
                _isDisconnected = true;
                _client?.Close();
            }
        }

        public async Task SendPacket(CPacket packet)
        {
            packet.EncodeHeader(PacketHeaderCode);
            packet.Encrypt(PacketEncodeKey);

            if (_client == null)
                return;
            NetworkStream stream = _client.GetStream();
            await stream.WriteAsync(packet.GetBuffer(), 0, packet.GetBufferSize());
        }

        public void RecvStart(RecvIOCompletion callback)
        {
            if (_client == null)
                return;

            lock (_lockRecv)
            {
                if (_isRecvStarted == true)
                    return;
                _isRecvStarted = true;
            }

            Thread thread = new Thread(() => ThreadRecv(callback));
            thread.Start();
        }



        private void ThreadRecv(RecvIOCompletion callback)
        {
            if (_client == null)
                return;

            int sizeHeader = Marshal.SizeOf(typeof(PacketHeader));
            PacketHeader header = new PacketHeader();
            NetworkStream stream = _client.GetStream();
            MemoryStream buffer = new MemoryStream(1000);
            while (true)
            {
                if (_isDisconnected == true)
                    return;

                try
                {
                    try
                    {
                        
                        byte[] tempBuf = new byte[1000];
                        int len = stream.Read(tempBuf, 0, 1000);
                        if (len == 0)
                        {
                            Disconnect();
                            return;
                        }
                        long prevPosition = buffer.Position;
                        buffer.Write(tempBuf, 0, len);
                        buffer.Position = prevPosition;
                        
                    }
                    catch (Exception)
                    {
                        Disconnect();
                        return;
                    }

                    // 버퍼 내의 모든 패킷을 처리한다.
                    bool isError = false;
                    while(true)
                    {
                        // 데이터가 헤더길이보다 작을 경우 break
                        if (buffer.Length - buffer.Position < sizeHeader)
                            break;

                        // 헤더를 읽음
                        header.Deserialize(buffer.GetBuffer(), (int)buffer.Position);

                        // 패킷 코드가 잘못됐을 경우 error
                        if(header.Code != PacketHeaderCode)
                        {
                            isError = true;
                            break;
                        }

                        // 1개 패킷 데이터가 모두 도착하지 않았을 경우 break
                        if (buffer.Length - buffer.Position < sizeHeader + header.Len)
                            break;

                        // 패킷에 데이터 입력 및 버퍼 position 전진
                        CPacket packet = new CPacket();
                        packet.EncodeHeader(header);
                        packet.Encode(buffer.GetBuffer(), (int)buffer.Position + sizeHeader, header.Len);
                        buffer.Position += sizeHeader + header.Len;

                        // 복호화
                        if (packet.Decipher(PacketEncodeKey) == false)
                        {
                            isError = true;
                            break;
                        }

                        // callback 함수로 패킷 전달
                        callback(packet);

                        // 버퍼의 데이터를 모두 읽었을 경우 Position을 초기화하고 break
                        if(buffer.Length == buffer.Position)
                        {
                            buffer.Position = 0;
                            buffer.SetLength(0);
                            break;
                        }
                    }

                    if(isError)
                    {
                        Disconnect();
                        return;
                    }
                }
                catch (Exception)
                {
                    _isDisconnected = true;
                    return;
                }
            }
        }


        private string ServerIP { get; set; }
        private ushort ServerPort { get; set; }
        private string LoginKey { get; set; }
        private byte PacketHeaderCode { get; set; }
        private byte PacketEncodeKey { get; set; }

        public delegate void RecvIOCompletion(CPacket packet);
        private object _lockRecv = new object();
        private bool _isRecvStarted = false;
        private TcpClient? _client = null;
        private volatile bool _isDisconnected = false;

    }
}
