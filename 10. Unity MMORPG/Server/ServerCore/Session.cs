using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net.Sockets;
using System.Net;
using System.Threading;

namespace ServerCore
{
    // 패킷으로 데이터를 주고받는 세션
    public abstract class PacketSession : Session
    {
        public static readonly int HeaderSize = 2;  // 헤더 크기

        public abstract void OnRecvPacket(ArraySegment<byte> buffer);


        public sealed override int OnRecv(ArraySegment<byte> buffer)
        {
            int processLen = 0;  // buffer 에서 read한 데이터 수
            int recvCount = 0;
            while(true)
            {
                // 헤더가 모두 수신되었는지 확인
                if (buffer.Count < HeaderSize)
                    break;

                // 패킷 데이터가 모두 수신되었는지 확인
                ushort dataSize = BitConverter.ToUInt16(buffer.Array, buffer.Offset);
                if (buffer.Count < dataSize)
                    break;

                // 패킷 수신됨
                recvCount++;
                OnRecvPacket(new ArraySegment<byte>(buffer.Array, buffer.Offset, dataSize));

                // buffer가 다음 패킷 데이터를 가르키도록 한다.
                processLen += dataSize;
                buffer = new ArraySegment<byte>(buffer.Array, buffer.Offset + dataSize, buffer.Count - dataSize);
            }

            if (recvCount > 0)
            {
                CounterManager.Instance.AddRecv(recvCount);
                CounterManager.Instance.AddRecvBytes(processLen);

            }
            return processLen;
        }
    }

    // 세션
    public abstract class Session
    {
        public bool IsDisconnected { get { return _disconnected != 0; } }


        Socket _socket;
        EndPoint _endPoint;    // _socket.RemoteEndPoint 를 사용하는 시점에는 _socket이 dispose 됐을 수 있기 때문에 RemoteEndPoint를 얻으려면 _endPoint 을 사용해야함
        int _disconnected = 0;   // disconnect 됨 여부

        
        object _lock = new object();       // lock
        Queue<ArraySegment<byte>> _sendQueue = new Queue<ArraySegment<byte>>();  // send할 데이터 모아놓는 큐
        List<ArraySegment<byte>> _pendingList = new List<ArraySegment<byte>>(); // send하기 위해 sendQueue에서 꺼낸 버퍼들을 모아놓는 리스트
        SocketAsyncEventArgs _sendArgs = new SocketAsyncEventArgs();  // send용 EventArgs

        RecvBuffer _recvBuffer = new RecvBuffer(65535);                // recv 버퍼
        SocketAsyncEventArgs _recvArgs = new SocketAsyncEventArgs();  // recv용 EventArgs


        public abstract void OnConnected(EndPoint endPoint);
        public abstract int OnRecv(ArraySegment<byte> buffer);
        public abstract void OnSend(int numOfBytes);
        public abstract void OnDisconnected(EndPoint endPoint);

        public string GetSocketInfo()
        {
            return _endPoint.ToString();
        }


        void Clear()
        {
            lock(_lock)
            {
                _sendQueue.Clear();
                _pendingList.Clear();
            }
        }

        public void Start(Socket socket)
        {
            // 소켓 생성
            _socket = socket;
            _endPoint = socket.RemoteEndPoint;

            // send 준비
            _sendArgs.Completed += new EventHandler<SocketAsyncEventArgs>(OnSendCompleted);

            // recv 준비
            _recvArgs.Completed += new EventHandler<SocketAsyncEventArgs>(OnRecvCompleted);

            // 비동기 recv 시작
            RegisterRecv();
        }

        public void Send(ArraySegment<byte> sendBuff)
        {
            lock (_lock)
            {
                // _sendQueue에 데이터를 넣고, 현재 send가 진행중이 아니라면 send 한다.
                _sendQueue.Enqueue(sendBuff);
                if (_pendingList.Count == 0)
                    RegisterSend();
            }
        }

        public void Send(List<ArraySegment<byte>> listSendBuff)
        {
            if (listSendBuff.Count == 0)
                return;

            lock (_lock)
            {
                // _sendQueue에 데이터를 넣고, 현재 send가 진행중이 아니라면 send 한다.
                foreach (ArraySegment<byte> sendBuff in listSendBuff)
                    _sendQueue.Enqueue(sendBuff);
                if (_pendingList.Count == 0)
                    RegisterSend();
            }
        }

        public void Disconnect()
        {
            if (Interlocked.Exchange(ref _disconnected, 1) == 0)
            {
                CounterManager.Instance.AddDisconnect();
                Logger.WriteLog(LogLevel.Debug, $"Session.Disconnect. remote:{_endPoint}");
                OnDisconnected(_endPoint);

                //_socket.Shutdown(SocketShutdown.Both);
                _socket.Close();
                Clear();
            }
        }

        #region network communication
        // 비동기 recv 등록하기
        void RegisterRecv()
        {
            // recv 버퍼의 write 가능한 영역을 recvArgs의 버퍼로 등록한다.
            _recvBuffer.Clean();
            ArraySegment<byte> segment = _recvBuffer.WriteSegment;
            _recvArgs.SetBuffer(segment.Array, segment.Offset, segment.Count);

            // recv
            try
            {
                bool pending = _socket.ReceiveAsync(_recvArgs);
                if (pending == false)
                {
                    OnRecvCompleted(null, _recvArgs);
                }
            }
            catch(ObjectDisposedException ex)
            {
                return;
            }
            catch(Exception ex)
            {
                CounterManager.Instance.AddRecvError();
                Logger.WriteLog(LogLevel.Error, $"Session.RegisterRecv Exception. remote:{_endPoint}, exception:{ex.ToString()}");
            }
        }

        // recv 완료루틴 함수. 이 함수는 스레드풀 내에서 실행될 수 있다.
        void OnRecvCompleted(object? sender, SocketAsyncEventArgs args)
        {
            // recv한 byte가 0보다 크고 소켓 에러가 없으면 성공
            if(args.BytesTransferred > 0 && args.SocketError == SocketError.Success)
            {
                try
                {
                    // recvBuffer의 write 커서 이동
                    if(_recvBuffer.OnWrite(args.BytesTransferred) == false)
                    {
                        Disconnect();
                        return;
                    }

                    // OnRecv
                    int processLen = OnRecv(_recvBuffer.ReadSegment);
                    if (processLen < 0 || _recvBuffer.DataSize < processLen)
                    {
                        Disconnect();
                        return;
                    }

                    // recvBuffer의 read 커서 이동
                    if(_recvBuffer.OnRead(processLen) == false)
                    {
                        Disconnect();
                        return;
                    }

                    // 비동기 recv 재시작
                    RegisterRecv();
                }
                catch (Exception ex)
                {
                    CounterManager.Instance.AddRecvError();
                    Logger.WriteLog(LogLevel.Error, $"Session.OnRecvCompleted Exception. remote:{_endPoint.ToString()}, byteTransfer:{args.BytesTransferred}, error:{args.SocketError}, exception:{ex.ToString()}");
                }
            }
            else
            {
                if (args.BytesTransferred == 0 && args.SocketError == SocketError.Success) { }
                else if (args.SocketError == SocketError.OperationAborted) { }
                else if (args.SocketError == SocketError.ConnectionReset) { }
                else
                {
                    CounterManager.Instance.AddRecvError();
                    Logger.WriteLog(LogLevel.Error, $"Session.OnRecvCompleted Error. remote:{_endPoint.ToString()}, byteTransfer:{args.BytesTransferred}, error:{args.SocketError}");
                }
                Disconnect();
            }
        }

        // 비동기 send 등록
        // 이 함수는 _lock을 lock 한 뒤에 호출된다.
        void RegisterSend()
        {
            if (_disconnected == 1)
                return;

            // _sendQueue의 모든 버퍼를 꺼내 _sendArgs.BufferList에 추가한다.
            int sendBytes = 0;
            _pendingList.Clear();
            while(_sendQueue.Count > 0)
            {
                // _sendQueue에서 버퍼를 꺼내 _pendingList에 추가한다.
                // 이 때 list에는 ArraySegment 구조체가 들어있어야 한다. ArraySegment 는 배열의 일부를 가르키는 구조체이다.
                // ArraySegment 구조체가 존재하는 이유는 pointer가 없어서 배열의 중간을 직접 가르킬 수 없기 때문이다.
                ArraySegment<byte> buff = _sendQueue.Dequeue();
                _pendingList.Add(buff);
                sendBytes += buff.Count;
            }
            _sendArgs.BufferList = _pendingList;
            CounterManager.Instance.AddSend(_pendingList.Count);
            CounterManager.Instance.AddSendBytes(sendBytes);

            // send
            try
            {
                bool pending = _socket.SendAsync(_sendArgs);
                if (pending == false)
                {
                    OnSendCompleted(null, _sendArgs);
                }
            }
            catch (ObjectDisposedException ex)
            {
                return;
            }
            catch (Exception ex)
            {
                CounterManager.Instance.AddSendError();
                Logger.WriteLog(LogLevel.Error, $"Session.RegisterSend Exception. remote:{_endPoint.ToString()}, exception:{ex.ToString()}");
            }
        }

        // 비동기 send 완료루틴 함수
        void OnSendCompleted(object? sender, SocketAsyncEventArgs args)
        {
            lock (_lock)
            {
                // send한 byte가 0보다 크고 소켓 에러가 없으면 성공
                if (args.BytesTransferred > 0 && args.SocketError == SocketError.Success)
                {
                    try
                    {
                        // 이전 send 정보 초기화
                        _sendArgs.BufferList = null;   // 이거는 해도되고 안해도됨
                        _pendingList.Clear();

                        OnSend(_sendArgs.BytesTransferred);

                        // _sendQueue에 보낼 데이터가 있다면 보냄
                        if (_sendQueue.Count > 0)
                            RegisterSend();
                    }
                    catch (Exception ex)
                    {
                        CounterManager.Instance.AddSendError();
                        Logger.WriteLog(LogLevel.Error, $"Session.OnSendCompleted Exception. remote:{_endPoint.ToString()}, byteTransfer:{args.BytesTransferred}, error:{args.SocketError}, exception:{ex.ToString()}");
                    }
                }
                else
                {
                    if (args.SocketError == SocketError.OperationAborted) { }
                    else if (args.SocketError == SocketError.ConnectionReset) { }
                    else
                    {
                        CounterManager.Instance.AddSendError();
                        Logger.WriteLog(LogLevel.Error, $"Session.OnSendCompleted Error. remote:{_endPoint.ToString()}, byteTransfer:{args.BytesTransferred}, error:{args.SocketError}");
                    }
                    Disconnect();
                }
            }
        }
        #endregion
    }
}
