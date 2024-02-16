using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using Google.Protobuf;
using Google.Protobuf.Protocol;
using MySql.Data.MySqlClient;
using MySqlX.XDevAPI;
using Server.Data;
using ServerCore;

namespace Server.Game
{
    public class LoginRoom : RoomBase
    {
        // object
        Dictionary<int, ClientSession> _sessions = new Dictionary<int, ClientSession>();
        public int SessionCount { get { return _sessions.Count; } }

        // login
        public long LoginCount { get; private set; } = 0;
        public long LoginCount1s { get; private set; } = 0;
        long _prevLoginCount = 0;


        /* push job */
        public void Init() { Push(_init); }
        public void EnterRoom(ClientSession session) { Push(_enterRoom, session); }
        public void LeaveRoom(int sessionId) { Push(_leaveRoom, sessionId); }


        // init
        public override void _init()
        {
            base._init();
            RoomId = -1;
        }




        // frame update
        List<ClientSession> _disconnectedSessions = new List<ClientSession>();
        List<ClientSession> _leavingSessions = new List<ClientSession>();
        public override void _update()
        {
            base._update();

            Flush();  // job queue 내의 모든 job 실행

            // 플레이어 업데이트
            _leavingSessions.Clear();
            _disconnectedSessions.Clear();
            foreach (ClientSession session in _sessions.Values)
            {
                // 연결끊긴 세션
                if (session.IsDisconnected == true)
                {
                    _disconnectedSessions.Add(session);
                    continue;
                }

                // 로그인 요청 처리
                if(session.LoginRequested == true && session.Login == false)
                {
                    _login(session);
                }

                // id, password가 일치함
                if (session.Login == true)
                {
                    Player dupPlayer = ObjectManager.Instance.FindByAccountNo(session.AccountNo);
                    if (dupPlayer != null)
                    {
                        // 동일 accountNo인 플레이어가 있을 경우 로그인 못함
                        session.Login = false;
                    }
                    else
                    {
                        // 로그인 성공시 이동
                        _leavingSessions.Add(session);
                    }
                }
            }

            // 연결끊긴 세션 처리
            foreach (ClientSession session in _disconnectedSessions)
            {
                _leaveRoom(session.SessionId);
                SessionManager.Instance.Remove(session);
            }

            // 로그인 완료된 세션을 게임룸으로 이동
            foreach (ClientSession session in _leavingSessions)
            {
                // 플레이어 생성
                Player myPlayer = _loadPlayer(session);
                ObjectManager.Instance.AddPlayer(myPlayer);

                // 현재 room에서 나감
                _leaveRoom(session.SessionId);

                // 로그인 성공 패킷 전송
                S_LoginResponse resPacket = new S_LoginResponse();
                resPacket.Result = 1;
                resPacket.Name = session.Name;
                session.Send(resPacket);

                // 게임룸에 플레이어 추가
                myPlayer.Room.EnterGame(myPlayer);

                LoginCount++;
            }
        }

        // 1초 간격 업데이트
        public override void _update1s()
        {
            base._update1s();

            long loginCount = LoginCount;
            LoginCount1s = loginCount - _prevLoginCount;
            _prevLoginCount = loginCount;
        }



        // 세션이 로그인룸에 들어옴
        public void _enterRoom(ClientSession session)
        {
            if (session == null)
            {
                Logger.WriteLog(LogLevel.Error, $"LoginRoom.EnterRoom. session is null");
                return;
            }

            _sessions.Add(session.SessionId, session);

            Logger.WriteLog(LogLevel.Debug, $"LoginRoom.EnterRoom. {session.SessionId}");
        }


        // 세션이 로그인룸에서 나감
        public void _leaveRoom(int sessionId)
        {
            _sessions.Remove(sessionId);

            Logger.WriteLog(LogLevel.Debug, $"LoginRoom.LeaveRoom. session:{sessionId}");
        }



        // 로그인 처리
        public void _login(ClientSession session)
        {
            if (string.IsNullOrEmpty(session.Name) || string.IsNullOrEmpty(session.Password))
                return;

            // id, password 조회
            MySqlParameter param = new MySqlParameter("@name", MySqlDbType.VarChar);
            param.Value = session.Name;
            int dataCount = 0;
            int accountNo = 0;
            string name = string.Empty;
            string password = string.Empty;
            foreach (MySqlDataReader reader in DBWriter.ExecuteReaderSync("SELECT accountno, name, password FROM unityaccount.account WHERE name = @name", param))
            {
                accountNo = Convert.ToInt32(reader[0]);
                name = reader[1].ToString();
                password = reader[2].ToString();
                dataCount++;
                break;
            }

            if (dataCount == 0)
            {
                // 데이터가 없다면 insert 함
                MySqlParameter parName = new MySqlParameter("@name", MySqlDbType.VarChar);
                parName.Value = session.Name;
                MySqlParameter parPassword = new MySqlParameter("@password", MySqlDbType.VarChar);
                parPassword.Value = session.Password;
                DBWriter.ExecuteNonQuerySync("INSERT INTO unityaccount.account (name, password) VALUES (@name, @password)",
                    new List<MySqlParameter> { parName, parPassword });

                foreach (MySqlDataReader reader in DBWriter.ExecuteReaderSync("SELECT accountno FROM unityaccount.account WHERE name = @name", parName))
                {
                    session.AccountNo = Convert.ToInt32(reader[0]);
                    break;
                }
            }
            else
            {
                // 데이터가 있다면 name, password가 일치하는지 확인
                if (session.Name == name && session.Password == password)
                {
                    session.AccountNo = accountNo;
                    session.LoginRequested = false;
                    session.Login = true;
                }
                else
                {
                    session.LoginRequested = false;
                    session.Login = false;
                }
            }
        }


        // 플레이어 데이터 로드
        public Player _loadPlayer(ClientSession session)
        {
            // 플레이어객체 생성
            Player myPlayer = new Player();
            session.MyPlayer = myPlayer;

            // 플레이어 데이터 로드
            MySqlParameter parAccount = new MySqlParameter("@accountno", MySqlDbType.Int32);
            parAccount.Value = session.AccountNo;
            bool hasPlayerData = false;
            int roomId;
            int spumId;
            int level;
            Vector2 pos;
            int hp;
            // select
            foreach (MySqlDataReader reader in DBWriter.ExecuteReaderSync(
                "SELECT roomid, spumid, level, posx, posy, hp FROM unitygame.character WHERE accountno = @accountno", parAccount))
            {
                hasPlayerData = true;
                roomId = Convert.ToInt32(reader[0]);
                spumId = Convert.ToInt32(reader[1]);
                level = Convert.ToInt32(reader[2]);
                pos = new Vector2(Convert.ToSingle(reader[3]), Convert.ToSingle(reader[4]));
                hp = Convert.ToInt32(reader[5]);


                // 플레이어 객체 초기화
                GameRoom room = RoomManager.Instance.Find(roomId);
                if(room == null)
                    room = RoomManager.Instance.GetRandomRoom();
                myPlayer.Init(session, room);
                myPlayer.InitSPUM(spumId);
                myPlayer.Pos = pos;
                myPlayer.Hp = hp;

                break;
            }

            // 캐릭터 데이터가 없다면 insert 함
            if (hasPlayerData == false)
            {
                // 플레이어 객체 초기화
                GameRoom room = RoomManager.Instance.GetRandomRoom();
                myPlayer.Init(session, room);

                // 캐릭터 데이터 insert
                DBWriter.ExecuteNonQuerySync("INSERT INTO unitygame.character (accountno, roomid, spumid, level, posx, posy, hp) VALUES (@accountno, @roomid, @spumid, @level, @posx, @posy, @hp)",
                    new List<MySqlParameter> {
                        new MySqlParameter("@accountno", session.AccountNo),
                        new MySqlParameter("@roomid", room.RoomId),
                        new MySqlParameter("@spumid", myPlayer.SPUMId),
                        new MySqlParameter("@level", myPlayer.Stat.Level),
                        new MySqlParameter("@posx", myPlayer.Pos.x),
                        new MySqlParameter("@posy", myPlayer.Pos.y),
                        new MySqlParameter("@hp", myPlayer.Hp)
                    });
            }

            return myPlayer;
        }
        int _count = 30;


        // ToString
        public override string ToString()
        {
            return $"room:login";
        }
    }
}
