using System;
using System.Collections.Generic;
using System.Linq;
using System.Net;
using System.Net.Sockets;
using System.Text;
using System.Threading.Tasks;

namespace ServerCore
{
    public class Connector
    {
        // 세션을 리턴하는 사용자 지정 함수. Listener 클래스 내에서는 이 함수를 이용하여 세션을 만든다.
        // 사용자는 Session 클래스를 상속받고 추상함수들을 구현한 클래스를 만든다음, 이것을 리턴하는 함수를 제공해야 한다.
        Func<Session> _sessionFactory;

        // connect
        public void Connect(IPEndPoint endPoint, Func<Session> sessionFactory)
        {
            _sessionFactory = sessionFactory;

            // 소켓 생성
            Socket socket = new Socket(endPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);

            // 비동기 connect 세팅
            SocketAsyncEventArgs args = new SocketAsyncEventArgs();
            args.Completed += OnConnectCompleted; // 완료루틴 함수 지정
            args.RemoteEndPoint = endPoint;       // 서버 주소 지정
            args.UserToken = socket;              // 완료루틴 함수 내에서 사용할 데이터

            // 비동기 connect 등록
            RegisterConnect(args);
        }

        // 비동기 connect 등록
        void RegisterConnect(SocketAsyncEventArgs args)
        {
            Socket socket = args.UserToken as Socket;
            if (socket == null)
                return;

            // 비동기 connect 시작
            bool pending = socket.ConnectAsync(args);
            if (pending == false)
                OnConnectCompleted(null, args);
        }

        // 비동기 connect 완료루틴 함수
        void OnConnectCompleted(object? sender, SocketAsyncEventArgs args)
        {
            if (args.SocketError == SocketError.Success)
            {
                // connect 성공 시 세션 생성 및 recv 시작
                Session session = _sessionFactory.Invoke();
                session.Start(args.ConnectSocket);  // args.ConnectSocket 는 Connect 함수에서 생성한 소켓과 동일하다
                session.OnConnected(args.RemoteEndPoint);
            }
            else
            {
                Logger.WriteLog(LogLevel.Error, $"Connector.OnConnectCompleted failed. EndPoint:{args.RemoteEndPoint.ToString()}, Error:{args.SocketError.ToString()}");
            }
        }
    }
}
