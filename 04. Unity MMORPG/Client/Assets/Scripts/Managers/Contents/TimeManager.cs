using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


public class TimeManager
{
    public const int TargetSpan = 60;

    public long CurrentTime { get { return DateTime.Now.Ticks - (long)_avgTimeDiff; } }  // 서버 기준의 현재시간

    public float RTT { get { return RTTs; } }
    public float RTTs { get { return (float)(_avgRTT / TimeSpan.TicksPerSecond); } }
    public float RTTms { get { return (float)(_avgRTT / TimeSpan.TicksPerMillisecond); } }
    public long RTTtick { get { return (long)_avgRTT; } }


    // private
    long _lastSendTime = 0;       // 마지막으로 SyncTime 보낸시간

    object _lock = new object();
    Queue<double> _qRTT = new Queue<double>();  // RTT 기록. 최대 TargetSpan 수만큼 저장된다.
    double _avgRTT = 0;            
    double _stdRTT = 0;

    Queue<double> _qTimeDiff = new Queue<double>();   // 서버와 클라의 시간차이 기록. 최대 TargetSpan 수만큼 저장된다.
    double _avgTimeDiff = 0;



    public void Init()
    {
        //// 네트워크 warming up?
        //for (int i = 0; i < 10; i++)
        //{
        //    S_SyncTimeResponse dummyPacket = new S_SyncTimeResponse();
        //    Managers.Network.Send(dummyPacket);
        //}
    }



    public void SyncTime(S_SyncTimeResponse packet)
    {
        lock (_lock)
        {
            // 평균 RTT 계산
            double rtt = (double)(DateTime.Now.Ticks - packet.ClientTime);
            if (_qRTT.Count < TargetSpan)
            {
                _qRTT.Enqueue(rtt);
                _avgRTT = (_avgRTT * (_qRTT.Count - 1) + rtt) / _qRTT.Count;  
            }
            else
            {
                if (rtt < _avgRTT - _stdRTT * 2 || rtt > _avgRTT + _stdRTT * 2)
                {
                    // 이상값일 경우 return
                    return;
                }
                else
                {
                    // 이상값이 아닐 경우 기록
                    double first = _qRTT.Dequeue();
                    _qRTT.Enqueue(rtt);
                    _avgRTT = ((_avgRTT * _qRTT.Count - first) + rtt) / _qRTT.Count;
                }
            }

            // RTT 표준편차 계산
            double stdRTT = 0;
            foreach (double v in _qRTT)
                stdRTT += (v - _avgRTT) * (v - _avgRTT);
            stdRTT = Math.Sqrt(stdRTT / _qRTT.Count);
            _stdRTT = stdRTT;


            // 서버와 클라이언트 평균 시간차이 계산
            double timeDiff = (packet.ClientTime + rtt / 2) - packet.ServerTime;
            if (_qTimeDiff.Count < TargetSpan)
            {
                _qTimeDiff.Enqueue(timeDiff);
                _avgTimeDiff = (_avgTimeDiff * (_qTimeDiff.Count - 1) + timeDiff) / _qTimeDiff.Count;
            }
            else
            {
                double first = _qTimeDiff.Dequeue();
                _qTimeDiff.Enqueue(timeDiff);
                _avgTimeDiff = ((_avgTimeDiff * _qTimeDiff.Count - first) + timeDiff) / _qTimeDiff.Count;
            }
        }

        //ServerCore.Logger.WriteLog(ServerCore.LogLevel.Debug, $"TimeManager.SyncTime. timeDiffms:{_avgTimeDiff / TimeSpan.TicksPerMillisecond :f3}, RTTms:{RTTms :f3}, RTTs:{RTTs :f3}");
    }


    int _passCount = 0;
    public void OnUpdate()
    {
        // 1초마다 C_SyncTime 패킷 전송
        if (DateTime.Now.Ticks - _lastSendTime < TimeSpan.TicksPerSecond)
            return;
        _lastSendTime = DateTime.Now.Ticks;

        // 3번 정도는 pass한 뒤 RTT를 계산한다. 안그러면 처음 값이 너무 높게나옴
        _passCount++;
        if (_passCount < 3)
            return;

        C_SyncTime packet = new C_SyncTime();
        packet.ClientTime = DateTime.Now.Ticks;
        Managers.Network.Send(packet);
    }

}

