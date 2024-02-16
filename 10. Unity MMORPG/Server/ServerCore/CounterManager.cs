using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Data;

namespace ServerCore
{
    public class CounterManager
    {
        struct CounterValue
        {
            public long value;
            public long prev;
            public long value1s;
            public void Update()
            {
                long curr = value;
                value1s = curr - prev;
                prev = curr;
            }
        }
        class PerformanceCounterValue
        {
            public PerformanceCounter pc;
            public float value;
            public float prev;
            public float value1s;
            public void Update()
            {
                value = pc.NextValue();
                value1s = value - prev;
                prev = value;
            }
        }

        /* instance */
        public static CounterManager Instance { get; } = new CounterManager();

        /* get counter */
        public long Accept { get { return _accept.value; } }
        public long Disconnect { get { return _disconnect.value; } }
        public long Recv { get { return _recv.value; } }
        public long Send { get { return _send.value; } }
        public long RecvBytes { get { return _recvBytes.value; } }
        public long SendBytes { get { return _sendBytes.value; } }

        public long AcceptError { get { return _acceptError.value; } }
        public long DisconnectError { get { return _disconnectError.value; } }
        public long RecvError { get { return _recvError.value; } }
        public long SendError { get { return _sendError.value; } }

        public long Accept1s { get { return _accept.value1s; } }
        public long Disconnect1s { get { return _disconnect.value1s; } }
        public long Recv1s { get { return _recv.value1s; } }
        public long Send1s { get { return _send.value1s; } }
        public long RecvBytes1s { get { return _recvBytes.value1s; } }
        public long SendBytes1s { get { return _sendBytes.value1s; } }

        public long AcceptError1s { get { return _acceptError.value1s; } }
        public long DisconnectError1s { get { return _disconnectError.value1s; } }
        public long RecvError1s { get { return _recvError.value1s; } }
        public long SendError1s { get { return _sendError.value1s; } }

        public int CurrentRecvThread { get { return _currentRecvThread; } }
        public int MaxCurrentRecvThread { get { return _maxCurrentRecvThread; } }
        public int CurrentSendThread { get { return _currentSendThread; } }
        public int MaxCurrentSendThread { get { return _maxCurrentSendThread; } }

        public float CPUTotal { get { return _pcCPUTotal.value; } }
        public float CPUUser { get { return _pcCPUUser.value; } }
        public float CPUKernel { get { return _pcCPUTotal.value - _pcCPUUser.value; } }

        public float TCPSegmentSent1s { get { return MathF.Round(_pcSegmentSent.value); } }
        public float TCPSegmentRetransmitted1s { get { return MathF.Round(_pcSegmentRetransmitted.value); } }

        public float MemoryAvailableMB { get { return _pcMemoryAvailableBytes.value / 1024f / 1024f; } }
        public float MemoryCommittedMB { get { return _pcMemoryAvailableBytes.value / 1024f / 1024f; } }
        public float MemoryNonpagedMB { get { return _pcMemoryPoolNonpagedBytes.value / 1024f / 1024f; } }
        public float MemoryPagedMB { get { return _pcMemoryPoolPagedBytes.value / 1024f / 1024f; } }

        public float ProcessPageFault1s { get { return _pcProcessPageFault.value; } }
        public float ProcessMemoryNonpagedMB { get { return _pcProcessPoolNonpagedBytes.value / 1024f / 1024f; } }
        public float ProcessMemoryPagedMB { get { return _pcProcessPoolPagedBytes.value / 1024f / 1024f; } }
        public float ProcessMemoryPrivateMB { get { return _pcProcessPrivateBytes.value / 1024f / 1024f; } }
        public float ProcessMemoryWorkingSetMB { get { return _pcProcessWorkingSet.value / 1024f / 1024f; } }
        public float ProcessMemoryWorkingSetPrivateMB { get { return _pcProcessWorkingSetPrivate.value / 1024f / 1024f; } }


        /* add counter */
        public void AddAccept() { Interlocked.Increment(ref _accept.value); }
        public void AddDisconnect() { Interlocked.Increment(ref _disconnect.value); }
        public void AddRecv() { Interlocked.Increment(ref _recv.value); }
        public void AddRecv(long amount) { Interlocked.Add(ref _recv.value, amount); }
        public void AddSend() { Interlocked.Increment(ref _send.value); }
        public void AddSend(long amount) { Interlocked.Add(ref _send.value, amount); }
        public void AddRecvBytes(long amount) { Interlocked.Add(ref _recvBytes.value, amount); }
        public void AddSendBytes(long amount) { Interlocked.Add(ref _sendBytes.value, amount); }

        public void AddAcceptError() { Interlocked.Increment(ref _acceptError.value); }
        public void AddDisconnectError() { Interlocked.Increment(ref _disconnectError.value); }
        public void AddRecvError() { Interlocked.Increment(ref _recvError.value); }
        public void AddRecvError(long amount) { Interlocked.Add(ref _recvError.value, amount); }
        public void AddSendError() { Interlocked.Increment(ref _sendError.value); }
        public void AddSendError(long amount) { Interlocked.Add(ref _sendError.value, amount); }

        public void AddCurrentRecvThread() 
        { 
            int val = Interlocked.Increment(ref _currentRecvThread);
            if (val > _maxCurrentRecvThread)
                _maxCurrentRecvThread = val;
        }
        public void SubCurrentRecvThread() { Interlocked.Decrement(ref _currentRecvThread); }
        public void AddCurrentSendThread() 
        { 
            int val = Interlocked.Increment(ref _currentSendThread);
            if (val > _maxCurrentSendThread)
                _maxCurrentSendThread = val;
        }
        public void SubCurrentSendThread() { Interlocked.Decrement(ref _currentSendThread); }

        /* counter */
        CounterValue _accept;
        CounterValue _disconnect;
        CounterValue _recv;
        CounterValue _send;
        CounterValue _recvBytes;
        CounterValue _sendBytes;

        CounterValue _acceptError;
        CounterValue _disconnectError;
        CounterValue _recvError;
        CounterValue _sendError;

        int _currentRecvThread = 0;
        int _maxCurrentRecvThread = 0;
        int _currentSendThread = 0;
        int _maxCurrentSendThread = 0;

        PerformanceCounterValue _pcCPUTotal = new PerformanceCounterValue();
        PerformanceCounterValue _pcCPUUser = new PerformanceCounterValue();
        PerformanceCounterValue _pcSegmentSent = new PerformanceCounterValue();
        PerformanceCounterValue _pcSegmentRetransmitted = new PerformanceCounterValue();
        PerformanceCounterValue _pcMemoryAvailableBytes = new PerformanceCounterValue();
        PerformanceCounterValue _pcMemoryCommittedBytes = new PerformanceCounterValue();
        PerformanceCounterValue _pcMemoryPoolNonpagedBytes = new PerformanceCounterValue();
        PerformanceCounterValue _pcMemoryPoolPagedBytes = new PerformanceCounterValue();
        PerformanceCounterValue _pcProcessPageFault = new PerformanceCounterValue();
        PerformanceCounterValue _pcProcessPoolNonpagedBytes = new PerformanceCounterValue();
        PerformanceCounterValue _pcProcessPoolPagedBytes = new PerformanceCounterValue();
        PerformanceCounterValue _pcProcessPrivateBytes = new PerformanceCounterValue();
        PerformanceCounterValue _pcProcessWorkingSet = new PerformanceCounterValue();
        PerformanceCounterValue _pcProcessWorkingSetPrivate = new PerformanceCounterValue();
        //PerformanceCounterValue _pcGCGen0Collection = new PerformanceCounterValue();
        //PerformanceCounterValue _pcGCGen1Collection = new PerformanceCounterValue();
        //PerformanceCounterValue _pcGCGen2Collection = new PerformanceCounterValue();
        //PerformanceCounterValue _pcGCTime = new PerformanceCounterValue();

        /* timer */
        System.Timers.Timer _timer = new System.Timers.Timer();

        
        public void Init()
        {
            try
            {
                //_pcCPU.pc = new PerformanceCounter("Processor", "% Processor Time", "_Total");
                //_pcRAM.pc = new PerformanceCounter("Memory", "Available MBytes");

                /* CPU 사용시간 쿼리 등록하기 */
                _pcCPUTotal.pc = new PerformanceCounter("Processor", "% Processor Time", "_Total");
                _pcCPUUser.pc = new PerformanceCounter("Processor", "% User Time", "_Total");

                /* TCP segment, 재전송 쿼리 등록하기 */
                // Segments Sent/sec는 다시 전송되는 바이트만 들어있는 세그먼트는 제외하고, 현재 연결에 있는 세그먼트를 포함하여 세그먼트를 보내는 비율입니다.
                // Segments Retransmitted/sec는 세그먼트를 다시 전송하는 비율입니다. 즉 이전에 전송된 바이트가 하나 이상 포함된 세그먼트를 전송하는 것입니다.
                _pcSegmentSent.pc = new PerformanceCounter("TCPv4", "Segments Sent/sec");
                _pcSegmentRetransmitted.pc = new PerformanceCounter("TCPv4", "Segments Retransmitted/sec");

                /* 시스템 메모리 카운트값 얻는 쿼리 등록하기 */
                // Available Bytes는 컴퓨터에서 실행되는 프로세스에 할당하거나 시스템에서 사용할 수 있는 실제 메모리의 양(바이트)입니다. 이것은 대기 중이거나 비어 있거나 0으로 채워진 페이지 목록에 할당된 메모리의 총계입니다.
                // Committed Bytes는 커밋된 가상 메모리의 양(바이트)입니다. 커밋된 메모리는 디스크 페이징 파일을 디스크에 다시 써야 할 필요가 있을 경우를 위해 예약된 실제 메모리입니다. 각 실제 드라이브에 하나 이상의 페이징 파일이 있을 수 있습니다. 이 카운터는 최근에 관찰된 값만 표시하며 평균값은 아닙니다.
                // Pool Nonpaged Bytes는 비페이징 풀의 크기(바이트)이고, 비페이징 풀은 디스크에 쓸 수 없지만 할당되어 있는 동안 실제 메모리에 남아 있어야 하는 개체에 사용되는 시스템 가상 메모리 영역입니다. Memory\\Pool Nonpaged Bytes는 Process\\Pool Nonpaged Bytes와 다르게 계산되므로 Process(_Total)\\Pool Nonpaged Bytes과 같지 않을 수 있습니다. 이 카운터는 최근에 관찰된 값만 표시하며 평균값은 아닙니다.
                // Pool Paged Bytes는 페이징 풀의 크기(바이트)이고, 페이징 풀은 사용되지 않고 있을 때 디스크에 쓸 수 있는 개체에 사용되는 시스템 가상 메모리 영역입니다. Memory\\Pool Paged Bytes는 Process\\Pool Paged Bytes와 다르게 계산되므로 Process(_Total)\\Pool Paged Bytes와 같지 않을 수 있습니다. 이 카운터는 최근에 관찰된 값만 표시하며 평균값은 아닙니다.
                _pcMemoryAvailableBytes.pc = new PerformanceCounter("Memory", "Available Bytes");
                _pcMemoryCommittedBytes.pc = new PerformanceCounter("Memory", "Committed Bytes");
                _pcMemoryPoolNonpagedBytes.pc = new PerformanceCounter("Memory", "Pool Nonpaged Bytes");
                _pcMemoryPoolPagedBytes.pc = new PerformanceCounter("Memory", "Pool Paged Bytes");


                /* 프로세스 메모리 카운트값 얻는 쿼리 등록하기 */
                // Page Faults/sec는 이 프로세스에서 실행 중인 스레드에서 발생하는 페이지 폴트 오류 비율입니다. 페이지 폴트는 프로세스가 주 메모리의 작업 집합에 없는 가상 메모리 페이지를 참조할 때 발생합니다. 이것은 페이지가 실행 대기 목록에 있지 않고 이미 주 메모리에 있거나 페이지를 공유하는 다른 프로세스에서 사용되고 있으면 디스크에서 페이지를 가져오지 않습니다.
                // Pool Nonpaged Bytes는 비페이징 풀의 크기(바이트)이고, 비페이징 풀은 디스크에 쓸 수 없지만 할당되어 있는 동안 실제 메모리에 남아 있어야 하는 개체에 사용되는 시스템 가상 메모리 영역입니다. Memory\\Pool Nonpaged Bytes는 Process\\Pool Nonpaged Bytes와 다르게 계산되므로 Process(_Total)\\Pool Nonpaged Bytes과 같지 않을 수 있습니다. 이 카운터는 최근에 관찰된 값만 표시하며 평균값은 아닙니다.
                // Pool Paged Bytes는 페이징 풀의 크기(바이트)이고, 페이징 풀은 사용되지 않고 있을 때 디스크에 쓸 수 있는 개체에 사용되는 시스템 가상 메모리 영역입니다. Memory\\Pool Paged Bytes는 Process\\Pool Paged Bytes와 다르게 계산되므로 Process(_Total)\\Pool Paged Bytes와 같지 않을 수 있습니다. 이 카운터는 최근에 관찰된 값만 표시하며 평균값은 아닙니다.
                // Private Bytes는 이 프로세스가 할당하여 다른 프로세스와는 공유할 수 없는 메모리의 현재 크기(바이트)입니다.
                // Working Set는 이 프로세스에 대한 작업 집합의 현재 크기(바이트)입니다. 작업 집합은 프로세스의 스레드가 최근에 작업한 메모리 페이지의 집합입니다. 컴퓨터에 있는 빈 메모리가 한계를 초과하면 페이지는 사용 중이 아니라도 프로세스의 작업 집합에 남아 있습니다. 빈 메모리가 한계 미만이면 페이지는 작업 집합에서 지워집니다. 이 페이지가 필요하면 주 메모리에서 없어지기 전에 소프트 오류 처리되어 다시 작업 집합에 있게 됩니다.
                // Working Set - Private는 다른 프로세서가 공유하거나 공유할 수 있는 작업 집합이 아니라 이 프로세서만 사용하고 있는 작업 집합의 크기(바이트)입니다.
                string processName = System.Diagnostics.Process.GetCurrentProcess().ProcessName;
                _pcProcessPageFault.pc = new PerformanceCounter("Process", "Page Faults/sec", processName);
                _pcProcessPoolNonpagedBytes.pc = new PerformanceCounter("Process", "Pool Nonpaged Bytes", processName);
                _pcProcessPoolPagedBytes.pc = new PerformanceCounter("Process", "Pool Paged Bytes", processName);
                _pcProcessPrivateBytes.pc = new PerformanceCounter("Process", "Private Bytes", processName);
                _pcProcessWorkingSet.pc = new PerformanceCounter("Process", "Working Set", processName);
                _pcProcessWorkingSetPrivate.pc = new PerformanceCounter("Process", "Working Set - Private", processName);

                /* GC 카운트값 얻는 쿼리 등록하기 */
                //_pcGCGen0Collection.pc = new PerformanceCounter(".NET CLR Memory", "# Gen 0 Collections", processName);
                //_pcGCGen1Collection.pc = new PerformanceCounter(".NET CLR Memory", "# Gen 1 Collections", processName); ;
                //_pcGCGen2Collection.pc = new PerformanceCounter(".NET CLR Memory", "# Gen 2 Collections", processName);
                //_pcGCTime.pc = new PerformanceCounter(".NET CLR Memory", "% Time in GC", processName);

                // 타이머 등록
                _timer.Elapsed += ((s, e) => Update());
                _timer.Interval = 990;
                _timer.AutoReset = true;
                _timer.Enabled = true;
            }
            catch (Exception ex)
            {
                Logger.WriteLog(LogLevel.Error, $"CounterManager.Init. {ex}");
            }

        }

        void Update()
        {
            try
            {
                _accept.Update();
                _disconnect.Update();
                _recv.Update();
                _send.Update();
                _recvBytes.Update();
                _sendBytes.Update();

                _acceptError.Update();
                _disconnectError.Update();
                _recvError.Update();
                _sendError.Update();

                _pcCPUTotal.Update();
                _pcCPUUser.Update();
                _pcSegmentSent.Update();
                _pcSegmentRetransmitted.Update();
                _pcMemoryAvailableBytes.Update();
                _pcMemoryCommittedBytes.Update();
                _pcMemoryPoolNonpagedBytes.Update();
                _pcMemoryPoolPagedBytes.Update();
                _pcProcessPageFault.Update();
                _pcProcessPoolNonpagedBytes.Update();
                _pcProcessPoolPagedBytes.Update(); ;
                _pcProcessPrivateBytes.Update();
                _pcProcessWorkingSet.Update();
                _pcProcessWorkingSetPrivate.Update();
                //_pcGCGen0Collection.Update();
                //_pcGCGen1Collection.Update();
                //_pcGCGen2Collection.Update();
                //_pcGCTime.Update();
            }
            catch (Exception ex)
            {
                Logger.WriteLog(LogLevel.Error, $"CounterManager.Update. {ex}");
            }
        }


    }
}
