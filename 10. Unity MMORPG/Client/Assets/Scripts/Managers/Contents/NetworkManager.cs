using ServerCore;
using System;
using System.Collections;
using System.Collections.Generic;
using System.Net;
using UnityEngine;
using Google.Protobuf;
using System.Net.Sockets;
using System.Threading;

public class NetworkManager
{
	/* session */
	ServerSession _session = new ServerSession();


	/* Delayed Packet */
	int _isDelayedPacket = 0;
    public bool IsDelayedPacket       // true�� �����ϸ� ���� �����带 run �Ѵ�.
	{
		get { return _isDelayedPacket == 1; }
		set
		{
			if (value == true)
			{
				if (Interlocked.Exchange(ref _isDelayedPacket, 1) == 0)
					RunDelayedSendThread();
			}
        }
	}
    public long PacketDelay { get; set; } = 50;    // ����:milliseconds
    struct DelayedPacket
    {
		public ushort id;
        public IMessage packet;
        public long time;
    }

	// to send delayed packet
    object _lockDelayedSendQueue = new object();
	Queue<DelayedPacket> _delayedSendQueue = new Queue<DelayedPacket>();
	AutoResetEvent _eventSendQueue = new AutoResetEvent(false);




    public void Send(IMessage packet)
	{
		if (IsDelayedPacket == false)
		{
			_session.Send(packet);
		}
		else
		{
            // delayed packet ����� ����� send
            lock (_lockDelayedSendQueue)
			{
				_delayedSendQueue.Enqueue(new DelayedPacket() { packet = packet, time = DateTime.Now.Ticks + (PacketDelay * TimeSpan.TicksPerMillisecond) });
			}
			_eventSendQueue.Set();

        }
    }

	public void Init()
	{
		// Network Logger
        ServerCore.Logger.Level = LogLevel.Debug;
        ServerCore.Logger.OnLog = Util.OnNetworkLog;

        IPAddress ipAddr = IPAddress.Parse(Managers.Config.ServerIP);
        IPEndPoint endPoint = new IPEndPoint(ipAddr, Managers.Config.ServerPort);

        Connector connector = new Connector();
		connector.Connect(endPoint,
			() => { return _session; });
    }

    public void OnUpdate()
	{
        
        if (IsDelayedPacket == false)
		{
            List<PacketMessage> list = PacketQueue.Instance.PopAll();
            foreach (PacketMessage packet in list)
			{
				Action<PacketSession, IMessage> handler = PacketManager.Instance.GetPacketHandler(packet.Id);
				if (handler != null)
					handler.Invoke(_session, packet.Message);
			}
		}
		else
		{
            // delayed packet ����� ����� recv
            long currentTime = DateTime.Now.Ticks;
            while (true)
			{
				PacketMessage packet = PacketQueue.Instance.Peek();
				if (packet == null)
					break;

				if (packet.Time + (PacketDelay * TimeSpan.TicksPerMillisecond) > currentTime)
					break;
				
				PacketQueue.Instance.Pop();
                Action<PacketSession, IMessage> handler = PacketManager.Instance.GetPacketHandler(packet.Id);
                if (handler != null)
                    handler.Invoke(_session, packet.Message);
			}
        }
	}



	/* Delayed Packet */
	void RunDelayedSendThread()
	{
		Thread sendThread = new Thread(_delayedSendThread);
		sendThread.Start();
	}

	void _delayedSendThread()
	{
        Queue<IMessage> qSendPacket = new Queue<IMessage>();
		while (true)
		{
			// _delayedSendQueue �� ��Ŷ�� �����⸦ ��ٸ���.
			_eventSendQueue.WaitOne();

			long currentTime = DateTime.Now.Ticks;
			long nextTime = 0;
			// _delayedSendQueue ���� �ð��� �ٵ� ��Ŷ�� ���� qSendPacket�� �ִ´�.
			lock (_lockDelayedSendQueue)
			{
				while (true)
				{
					if (_delayedSendQueue.Count == 0)
					{
						nextTime = 0;
						break;
					}

					DelayedPacket dPacket = _delayedSendQueue.Peek();
					if (currentTime > dPacket.time)
					{
						_delayedSendQueue.Dequeue();
						qSendPacket.Enqueue(dPacket.packet);
					}
					else
					{
						nextTime = dPacket.time;
						break;
					}
				}
			}

			// qSendPacket �� ��Ŷ�� ��� �����Ѵ�.
			foreach (IMessage packet in qSendPacket)
				_session.Send(packet);
			qSendPacket.Clear();

			// ���� ��Ŷ ���۽ð����� sleep �ϰ� ����� ��� �۾��� �����Ѵ�.
			if (nextTime > 0)
			{
				int sleepTime = Math.Max((int)((nextTime - DateTime.Now.Ticks) / TimeSpan.TicksPerMillisecond), 0);
				Thread.Sleep(sleepTime);
				_eventSendQueue.Set();
			}
		
        }
    }

}
