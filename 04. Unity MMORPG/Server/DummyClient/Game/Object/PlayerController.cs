using Google.Protobuf.Protocol;
using Google.Protobuf.WellKnownTypes;
using DummyClient.Data;
using ServerCore;
using System;
using System.Collections.Generic;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Numerics;
using System.Text;
using System.Threading.Tasks;
using static DummyClient.Define;

namespace DummyClient.Game
{
    public class PlayerController : CreatureController
    {
        public ServerSession Session { get; private set; }

        public AutoMode MyAutoMode { get; private set; } = AutoMode.ModeNone;

        C_Move _lastCMove = null;       // 마지막으로 보낸 C_Move 패킷
        int _lastCMoveSendTime = 0;     // 마지막으로 C_Move 패킷 보낸 시간 (단위:ms)



        public void Init(ServerSession session, S_EnterGame enterPacket)
        {
            session.MyPlayer = this;
            Session = session;
            RoomId = enterPacket.RoomId;
            base.Init(enterPacket.Object);


            float randAuto = new Random().NextSingle();
            if (randAuto < ConfigManager.Config.AutoProb)
            {
                MyAutoMode = AutoMode.ModeAuto;
            }
            
            ObjectType = GameObjectType.Player;
        }


        public override void Update()
        {
            GetKeyInput();
            base.Update();
        }



        // 키보드 입력 설정
        Random _rand = new Random();
        float _lastUpdateTime = 0;
        void GetKeyInput()
        {
            int tick = Environment.TickCount;
            if (tick - _lastUpdateTime < 1000)
                return;
            _lastUpdateTime = tick;


            bool bMove = (_rand.NextSingle() < 0.8f);
            bool bChangeDirection = (_rand.NextSingle() < 0.3f);


            MoveKeyDown = bMove;
            if(bChangeDirection)
            {
                Dir  = (MoveDir)(int)(_rand.NextSingle() * 8);   
            }
        }


        // Idle 상태 업데이트
        protected override void UpdateIdle()
        {
            // 방향키가 눌림
            if (MoveKeyDown == true)
            {
                // 목적지 계산
                Vector2 dir = Util.GetDirectionVector(Dir);
                Vector2 dest = Pos + dir * ConfigManager.Config.MyPlayerMinimumMove * Speed;   // 처음 움직일 때는 dest를 최소이동거리 만큼 이동시킨다.

                Dest = dest;
                State = CreatureState.Moving;

                // 서버에 전송
                SendMovePacket();
            }
            // 현재위치와 목적지가 다름
            else if (Util.Equals(Pos, Dest) == false)
            {
                State = CreatureState.Moving;
                SendMovePacket();
            }
        }

        protected override void UpdateMoving()
        {
            // 키보드 방향키가 눌려있는 동안은 Dest를 계속해서 이동시킨다.
            if (MoveKeyDown == true)
            {
                // dest 이동
                Dest += Util.GetDirectionVector(Dir) * GameManager.Instance.Time.DeltaTime * Speed;
            }
            // 키보드 방향키를 누르고있지 않다면 멈출 것이기 때문에 Dest를 이동시키지 않는다.
            else
            {
            }

            // 위치 이동
            Vector2 diff = (Dest - Pos);
            Vector2 dir = diff.normalized;
            Vector2 pos = Pos + dir * GameManager.Instance.Time.DeltaTime * Speed;

            // Dest에 도착시 현재위치를 Dest로 변경한다.
            // 만약 이동키가 눌려져있지 않다면 현재 위치에 stop 하고 상태를 idle로 바꾼다.
            if (diff.magnitude <= GameManager.Instance.Time.DeltaTime * Speed)
            {
                if (MoveKeyDown)
                {
                    Pos = Dest;
                }
                else
                {
                    StopAt();
                }
            }
            // 위치 이동
            else
            {
                Pos = pos;
            }


            SendMovePacket();
        }


        int _lastRespawnSendTime = 0;
        protected override void UpdateDead()
        {
            int tick = Environment.TickCount;
            if (tick - _lastRespawnSendTime < 1000)
                return;
            _lastRespawnSendTime = tick;

            C_Respawn packet = new C_Respawn();
            packet.BRespawn = true;
            Session.Send(packet);
        }



        // 서버에 이동패킷 전송
        // 상태 변화가 있거나, 마지막으로 전송한 시간에서 Config.MovePacketSendInterval 시간 이상 지났을 경우 전송함
        void SendMovePacket(bool forced = false)
        {
            bool bSend = false;
            int tick = Environment.TickCount;
            if (forced)
            {
                bSend = true;
            }
            else if (_lastCMove == null)
            {
                bSend = true;
            }
            else if (PosInfo.MoveDir != _lastCMove.PosInfo.MoveDir
                || PosInfo.State != _lastCMove.PosInfo.State
                || PosInfo.MoveKeyDown != _lastCMove.PosInfo.MoveKeyDown
                || tick - _lastCMoveSendTime > ConfigManager.Config.MovePacketSendInterval)
            {
                bSend = true;
            }

            if (!bSend)
                return;

            // 서버에 전송
            C_Move movePacket = new C_Move();
            movePacket.PosInfo = PosInfo.Clone();
            movePacket.MoveTime = DateTime.Now.Ticks;
            Session.Send(movePacket);

            _lastCMove = movePacket;
            _lastCMoveSendTime = Environment.TickCount;
        }


    }
}
