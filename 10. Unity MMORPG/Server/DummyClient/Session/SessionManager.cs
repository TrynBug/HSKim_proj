using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DummyClient
{

    internal class SessionManager
    {
        static SessionManager _session = new SessionManager();
        public static SessionManager Instance { get { return _session; } }

        int _sessionId = 0;
        Dictionary<int, ServerSession> _sessions = new Dictionary<int, ServerSession>();
        object _lock = new object();

        public int SessionCount { get { return _sessions.Count; } }

        // 세션 생성 및 세션ID 지정
        public ServerSession Generate()
        {
            lock(_lock)
            {
                int sessionId = ++_sessionId;
                ServerSession session = new ServerSession();
                session.SessionId = sessionId;
                _sessions.Add(sessionId, session);
                return session;
            }
        }

        public ServerSession Find(int id)
        {
            lock(_lock)
            {
                ServerSession session = null;
                _sessions.TryGetValue(id, out session);
                return session;
            }
        }

        public void Remove(ServerSession session)
        {
            lock(_lock)
            {
                _sessions.Remove(session.SessionId);
            }
        }
    }
}
