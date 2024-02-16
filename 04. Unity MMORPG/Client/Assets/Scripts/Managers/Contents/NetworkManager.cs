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
    public bool IsDelayedPacket       // true로 세팅하면 전용 스레드를 run 한다.
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
    public long PacketDelay { get; set; } = 50;    // 단위:milliseconds
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
            // delayed packet 모드일 경우의 send
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
            // delayed packet 모드일 경우의 recv
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
			// _delayedSendQueue 에 패킷이 들어오기를 기다린다.
			_eventSendQueue.WaitOne();

			long currentTime = DateTime.Now.Ticks;
			long nextTime = 0;
			// _delayedSendQueue 에서 시간이 다된 패킷을 꺼내 qSendPacket에 넣는다.
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

			// qSendPacket 의 패킷을 모두 전송한다.
			foreach (IMessage packet in qSendPacket)
				_session.Send(packet);
			qSendPacket.Clear();

			// 다음 패킷 전송시간까지 sleep 하고 깨어나면 즉시 작업을 시작한다.
			if (nextTime > 0)
			{
				int sleepTime = Math.Max((int)((nextTime - DateTime.Now.Ticks) / TimeSpan.TicksPerMillisecond), 0);
				Thread.Sleep(sleepTime);
				_eventSendQueue.Set();
			}
		
        }
    }

}
