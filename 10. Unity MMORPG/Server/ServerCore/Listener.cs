using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Net.Sockets;
using System.Net;
using Microsoft.Win32;

namespace ServerCore
{
    public class Listener
    {
        Socket _listenSocket;

        // 세션을 리턴하는 사용자 지정 함수. Listener 클래스 내에서는 이 함수를 이용하여 세션을 만든다.
        // 사용자는 Session 클래스를 상속받고 추상함수들을 구현한 클래스를 만든다음, 이것을 리턴하는 함수를 제공해야 한다.
        Func<Session> _sessionFactory;  

        // listen 소켓을 생성하고 accept를 시작한다.
        // register : accept 스레드 수, backlog : backlog queue 크기
        public void Init(IPEndPoint endPoint, Func<Session> sessionFactory, int register = 10, int backlog = 100)
        {
            _sessionFactory = sessionFactory;

            // listen socket 생성, bind, linsten
            _listenSocket = new Socket(endPoint.AddressFamily, SocketType.Stream, ProtocolType.Tcp);
            _listenSocket.Bind(endPoint);
            _listenSocket.Listen(backlog);
            Logger.WriteLog(LogLevel.System, $"Listener.Init Listening... register:{register}, backlog:{backlog}");

            for (int i=0; i<register; i++)
            {
                // SocketAsyncEventArgs 는 비동기 소켓 작업을 표현한다.
                // args.completed에 비동기 accept가 성공했을 때 호출될 콜백 함수를 등록한다.
                // 주의할 점은, SocketAsyncEventArgs는 재사용이 가능하지만 재사용하기 전 AcceptSocket 멤버를 null로 초기화해야 한다는 것이다.
                SocketAsyncEventArgs args = new SocketAsyncEventArgs();
                args.Completed += new EventHandler<SocketAsyncEventArgs>(OnAcceptCompleted);

                // 비동기 accept 시작
                RegisterAccept(args);
            }
        }

        // 비동기 accept 시작
        void RegisterAccept(SocketAsyncEventArgs args)
        {
            // 여기서 이것을 초기화하지 않고 사용하면 System.ObjectDisposedException: Safe handle has been closed. 예외가 발생한다.
            args.AcceptSocket = null;

            bool pending = _listenSocket.AcceptAsync(args);
            if (pending == false)
            {
                // pending이 false이면 backlog queue에 이미 연결요청이 있었다는 의미이다.
                // Accept 완료루틴을 호출한다.
                OnAcceptCompleted(null, args);
            }
            // pending이 true이면 비동기 accept가 시작되고, accept가 완료되면 OnAcceptCompleted 함수가 호출된다.
        }

        // 비동기 accept 완료루틴
        // 주의할 점은 이 완료루틴은 스레드풀의 스레드에서 실행된다는 것이다. 즉, 동기화 처리가 필요할 수 있다.
        void OnAcceptCompleted(object? sender, SocketAsyncEventArgs args)
        {
            // 소켓 오류 체크
            if(args.SocketError == SocketError.Success)
            {
                Logger.WriteLog(LogLevel.Debug, $"Listener.OnAcceptCompleted. remote:{args.AcceptSocket.RemoteEndPoint.ToString()}");

                // accept 성공시 세션 생성, recv 시작
                Session session = _sessionFactory.Invoke();
                session.Start(args.AcceptSocket);
                session.OnConnected(args.AcceptSocket.RemoteEndPoint);
            }
            else
            {
                // 소켓 오류 발생시
                Logger.WriteLog(LogLevel.Error, $"Listener.OnAcceptCompleted Error. error:{args.SocketError.ToString()}");
            }

            // 비동기 accept 재등록
            RegisterAccept(args);
        }
    }
}
