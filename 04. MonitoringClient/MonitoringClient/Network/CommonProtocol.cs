﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace MonitoringClient
{
    //#ifndef __GODDAMNBUG_ONLINE_PROTOCOL__
    //#define __GODDAMNBUG_ONLINE_PROTOCOL__



    enum en_PACKET_TYPE
    {
        ////////////////////////////////////////////////////////
        //
        //	Client & Server Protocol
        //
        ////////////////////////////////////////////////////////

        //------------------------------------------------------
        // Chatting Server
        //------------------------------------------------------
        en_PACKET_CS_CHAT_SERVER = 0,

        //------------------------------------------------------------
        // 채팅서버 로그인 요청
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		WCHAR	ID[20]				// null 포함
        //		WCHAR	Nickname[20]		// null 포함
        //		char	SessionKey[64];
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_CHAT_REQ_LOGIN,

        //------------------------------------------------------------
        // 채팅서버 로그인 응답
        //
        //	{
        //		WORD	Type
        //
        //		BYTE	Status				// 0:실패	1:성공
        //		INT64	AccountNo
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_CHAT_RES_LOGIN,

        //------------------------------------------------------------
        // 채팅서버 섹터 이동 요청
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		WORD	SectorX
        //		WORD	SectorY
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_CHAT_REQ_SECTOR_MOVE,

        //------------------------------------------------------------
        // 채팅서버 섹터 이동 결과
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		WORD	SectorX
        //		WORD	SectorY
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_CHAT_RES_SECTOR_MOVE,

        //------------------------------------------------------------
        // 채팅서버 채팅보내기 요청
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		WORD	MessageLen
        //		WCHAR	Message[MessageLen / 2]		// null 미포함
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_CHAT_REQ_MESSAGE,

        //------------------------------------------------------------
        // 채팅서버 채팅보내기 응답  (다른 클라가 보낸 채팅도 이걸로 받음)
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		WCHAR	ID[20]						// null 포함
        //		WCHAR	Nickname[20]				// null 포함
        //		
        //		WORD	MessageLen
        //		WCHAR	Message[MessageLen / 2]		// null 미포함
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_CHAT_RES_MESSAGE,

        //------------------------------------------------------------
        // 하트비트
        //
        //	{
        //		WORD		Type
        //	}
        //
        //
        // 클라이언트는 이를 30초마다 보내줌.
        // 서버는 40초 이상동안 메시지 수신이 없는 클라이언트를 강제로 끊어줘야 함.
        //------------------------------------------------------------	
        en_PACKET_CS_CHAT_REQ_HEARTBEAT,






        //------------------------------------------------------
        // Login Server
        //------------------------------------------------------
        en_PACKET_CS_LOGIN_SERVER = 100,

        //------------------------------------------------------------
        // 로그인 서버로 클라이언트 로그인 요청
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		char	SessionKey[64]
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_LOGIN_REQ_LOGIN,

        //------------------------------------------------------------
        // 로그인 서버에서 클라이언트로 로그인 응답
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		BYTE	Status				// 0 (세션오류) / 1 (성공) ...  하단 defines 사용
        //
        //		WCHAR	ID[20]				// 사용자 ID		. null 포함
        //		WCHAR	Nickname[20]		// 사용자 닉네임	. null 포함
        //
        //		WCHAR	GameServerIP[16]	// 접속대상 게임,채팅 서버 정보
        //		USHORT	GameServerPort
        //		WCHAR	ChatServerIP[16]
        //		USHORT	ChatServerPort
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_LOGIN_RES_LOGIN,








        //------------------------------------------------------
        // Game Server
        //------------------------------------------------------
        en_PACKET_CS_GAME_SERVER = 1000,

        //------------------------------------------------------------
        // 로그인 요청
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		char	SessionKey[64]
        //
        //		int		Version			// Major 100 + Minor 10  = 1.10
        //								// 현재 최신 버전은		0.01 (1) - 2016.03.28
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_GAME_REQ_LOGIN,

        //------------------------------------------------------------
        // 로그인 응답
        //
        //	{
        //		WORD	Type
        //
        //		BYTE	Status (0: 실패 / 1: 성공 / 2: 신규캐릭터 선택 모드 / 3:버전 다름.)
        //		INT64	AccountNo
        //	}
        //
        //  Status 가 1 로 들어오면 클라이언트는 바로 게임을 시작하며
        //  Status 가 2 로 들어오면 클라이언트는 캐릭터 선택 화면으로 전환 됨.
        //
        //  캐릭터 선택이 안된 최초접속시 Status 2 를 클라로 보내며, Status 2 의 경우는 AUTH 모드에 머무름.
        //
        //  Status 1 : 캐릭터 정보 로드, 셋팅 , GAME 모드 전환 후 게임 시작.
        //  Status 2 : AUTH 모드 유지, REQ_CHARACTER_SELECT 가 오면 다음으로 넘어감.
        //  Status 3 : 서버,클라의 버전 미스매치 
        //
        //  en_PACKET_CS_GAME_RES_LOGIN define 값 사용.
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_LOGIN,



        //------------------------------------------------------------
        // 캐릭터 선택 요청!
        //
        //	{
        //		WORD	Type
        //
        //		BYTE	CharacterType ( 1 ~ 5 )
        //	}
        //
        // RES_LOGIN 의 Status 가 2 로 들어오면 클라이언트는 캐릭터 선택 화면으로 전환이 되었으며,
        // 캐릭터 선택 후 본 패킷  REQ_CHARACTER_SELECT  을 서버로 보냄.
        //
        // 서버는 본 패킷을 받으면 신규 캐릭터 선택된 캐릭터를 저장하며, 기본 능력치로 셋팅된 정보를 저장 후 결과를 응답을 보냄.
        //------------------------------------------------------------
        en_PACKET_CS_GAME_REQ_CHARACTER_SELECT,

        //------------------------------------------------------------
        // 캐릭터 선택 응답.
        //
        //	{
        //		WORD	Type
        //
        //		BYTE	Status	( 0 : 실패 / 1 : 성공 )
        //	}
        //
        //  캐릭터 선택 결과 여부.  
        //
        //  캐릭터 선택에 성공 하였다면 Status 1 을 보냄. (실패는 CharacterType 이 1 ~ 5 범위가 아닌 경우만)
        //
        //  Status 0 은 캐릭터 생성 실패로 전송 후 접속을 끊으며,
        //  Status 1 은 신규 캐릭터 정보를 DB 에 저장하고 본 패킷 전송 후 GAME 모드로 전환, 게임 시작.
        //  
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_CHARACTER_SELECT,










        //------------------------------------------------------------
        // 내 캐릭터 생성
        //
        //	{
        //		WORD	Type
        //		
        //		INT64	ClientID			// 이는 현 서버접속자 중 고유값 (ClientID or AccontNo)
        //									// 서버와 클라이언트가 각 플레이어를 구분 할 수 있는 수치이면 됨.
        //									// AccountNo 도 가능은 하나 다른 유저들의 AccountNo 를 공유하는건 좋지는 않음.
        //		BYTE	CharacterType
        //		WCHAR	Nickname[20]
        //		float	PosX
        //		float	PosY
        //		USHORT	Rotation
        //		int		Cristal
        //		int		HP
        //		INT64	Exp
        //		USHORT	Level
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_CREATE_MY_CHARACTER,

        //------------------------------------------------------------
        // 다른 유저의  캐릭터 생성
        //
        //	{
        //		WORD	Type
        //
        //		INT64	ClientID
        //
        //		BYTE	CharacterType
        //		WCHAR	Nickname[20]
        //		float	PosX
        //		float	PosY
        //		USHORT	Rotation
        //		USHORT	Level
        //		BYTE	Respawn
        //
        //		BYTE	Sit				// 앉아있는지
        //		BYTE	Die				// 죽은 상태인지
        //
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER,


        //------------------------------------------------------------
        // 몬스터 캐릭터 생성
        //
        //	{
        //		WORD	Type
        //
        //		INT64	ClientID		// 몬스터도 모두 캐릭터로서 몬스터만의 고유 ClientID 를 사용한다.
        //								// 이에대한 관리는 알아서.
        //
        //		float	PosX
        //		float	PosY
        //		USHORT	Rotation
        //
        //		BYTE	Respawn			// 몬스터 리스폰 여부  0 : 아님(섹터 이동으로 생성) / 1: 리스폰 (이펙트 터짐)
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER,



        //------------------------------------------------------------
        // 캐릭터, 오브젝트의 삭제 (유저의 접속종료 또는 섹터에서 나감 등..)
        //
        //	{
        //		WORD	Type
        //
        //		INT64	ClientID
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_REMOVE_OBJECT,









        //------------------------------------------------------------
        // 캐릭터 이동 요청 (캐릭터의 이동,회전시 모두 보냄)
        //
        // 서버측에선 계산을 통한 자체 이동처리를 하지 않으며, 클라이언트가 주기적으로 이동 패킷을 보내서
        // 서버의 좌표 동기화를 맞추어 준다.
        //
        // 서버에서는 그냥 이동패킷이 오면 좌표를 변경후 다른 클라에게 전달 해주며
        // 정지 패킷이 오면 그냥 좌표보정 후 다른 클라에게 전달 해준다.
        //
        // VKey, HKey 는 클라이언트의 액션을 자연스럽게 처리 해주기 위한 키상태.
        // 이 정보는 서버에서 처리할 것은 없지만 저장 해 두었다가 다른 사용자에게 함께 뿌려줘야 어색하지 않음.
        //
        //	{
        //		WORD	Type
        //
        //		INT64	ClientID
        //		float	X
        //		float	Y
        //		USHORT	Rotation
        //
        //		BYTE	VKey		// 상하키 눌림여부. 0 (안눌림) - 1 (아래) - 2 (위)
        //		BYTE	HKey		// 좌우키 눌림여부. 0 (안눌림) - 1 (왼쪽) - 2 (오른)
        //	}
        //
        // en_PACKET_CS_GAME_REQ_MOVE_CHARACTER,
        // en_PACKET_CS_GAME_RES_MOVE_CHARACTER,
        //
        // 2개의 패킷 모두 내용은 같음.
        //------------------------------------------------------------
        en_PACKET_CS_GAME_REQ_MOVE_CHARACTER,
        en_PACKET_CS_GAME_RES_MOVE_CHARACTER,

        //------------------------------------------------------------
        // 캐릭터 정지 요청 
        //
        //	{
        //		WORD	Type
        //
        //		INT64	ClientID
        //		float	X
        //		float	Y
        //		USHORT	Rotation
        //	}
        //
        // en_PACKET_CS_GAME_REQ_STOP_CHARACTER,
        // en_PACKET_CS_GAME_RES_STOP_CHARACTER,
        //
        // 2개의 패킷 모두 내용은 같음.
        //------------------------------------------------------------
        en_PACKET_CS_GAME_REQ_STOP_CHARACTER,
        en_PACKET_CS_GAME_RES_STOP_CHARACTER,




        //------------------------------------------------------------
        // 몬스터 이동 
        //
        //	{
        //		WORD	Type
        //
        //		INT64	ClientID
        //		float	X
        //		float	Y
        //		USHORT	Rotation
        //	}
        //
        // 본 패킷은 서버가 클라에게만 쏨
        //
        // 몬스터의 이동은 서버 주도로 이루어지므로 최대  x,y 각 2타일 간격으로 이동하도록 함.
        // 그 이상 멀리 이동시 클라에서 보여지는 부분에 어색함은 없으나
        // 서버에서는 이미 이동이 완료된 상태이기 때문에 로직처리 (공격 등) 에서 오차가 나올 수 있음.
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_MOVE_MONSTER,




        //------------------------------------------------------------
        // 공격1 요청 / 응답
        //
        // 클라이언트 공격시 REQ 패킷을 서버로 보내며 서버는 현 캐릭터의 위치,방향을 기준으로
        // 이 공격에 데미지를 맞을 캐릭터를 찾아서 데미지를 먹이고 다른 주변 클라들에게 RES 패킷을 보낸다.
        //
        // 데미지 맞은 캐릭터가 있을시, 데미지 패킷은 별도로 따로 보낸다.
        //
        //	{
        //		WORD	Type
        //
        //		INT64	ClientID
        //	}
        //
        // en_PACKET_CS_GAME_REQ_ATTACK1,
        // en_PACKET_CS_GAME_RES_ATTACK1,
        // en_PACKET_CS_GAME_REQ_ATTACK2,
        // en_PACKET_CS_GAME_RES_ATTACK2,
        // en_PACKET_CS_GAME_RES_MONSTER_ATTACK
        //
        // 5개의 패킷 모두 내용은 같음.
        //------------------------------------------------------------
        en_PACKET_CS_GAME_REQ_ATTACK1,
        en_PACKET_CS_GAME_RES_ATTACK1,

        en_PACKET_CS_GAME_REQ_ATTACK2,
        en_PACKET_CS_GAME_RES_ATTACK2,

        en_PACKET_CS_GAME_RES_MONSTER_ATTACK,


        //------------------------------------------------------------
        // 데미지
        //
        // 서버에서 공격대상에 데미지를 먹인 후 이 패킷을 보냄.  해당 플레이어 주변에 모두 뿌림
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AttackClientID
        //		INT64	TargetClientID
        //
        //		int		Damage				// 실제 데미지 먹은 수치.
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_DAMAGE,

        //------------------------------------------------------------
        // 몬스터 사망
        //
        // 몬스터가 죽으면 클라에게 이 패킷을 보냄
        //
        //	{
        //		WORD	Type
        //
        //		INT64	MonsterClientID
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_MONSTER_DIE,







        //------------------------------------------------------------
        // 크리스탈 생성
        //
        // 몬스터가 죽으면서 크리스탈을 생성한다.
        //
        //	{
        //		WORD	Type
        //
        //		INT64	CristalClientID
        //		BYTE	byCristalType			// 1 / 2 / 3   (크리스탈 획득 10, 20, 30)
        //		float	fPosX
        //		float	fPosY
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_CREATE_CRISTAL,

        //------------------------------------------------------------
        // 크리스탈 먹기 요청
        //
        // 플레이어가 줍기 키를 눌러 크리스탈 획득을 요청 / 뿌리기.
        //
        // 플레이어가 줍기 키를 누르면 REQ_PICK 을 서버로 보내며 
        // 그때 서버는 먼저 RES_PICK 을 주변에 뿌려서 액션 동기화를 맞춰준다.
        //
        //	{
        //		WORD	Type
        //
        //		INT64	ClientID
        //	}
        //------------------------------------------------------------
        en_PACKET_CS_GAME_REQ_PICK,
        en_PACKET_CS_GAME_RES_PICK,

        //------------------------------------------------------------
        // 바닥에 앉기 동작 요청,응답
        //
        // 플레이어가 앉기 키를 누르면서 RES 패킷을 서버로 보냄
        // 서버는 해당 캐릭터를 앉혀진 모드로 바꾸고, REQ 패킷을 다른 주변 클라이언트에 뿌림.
        //
        // 앉은 캐릭터는 일정시간 마다 HP 가 증가 해야 함.
        //
        //	{
        //		WORD	Type
        //
        //		INT64	ClientID
        //	}
        //------------------------------------------------------------
        en_PACKET_CS_GAME_REQ_SIT,
        en_PACKET_CS_GAME_RES_SIT,



        //------------------------------------------------------------
        // 크리스탈 획득.
        //
        // 위의 REQ_PICK 수신시, 서버는 해당 클라이언트 주변에 획득 가능한 크리스탈을 확인하고
        // 먹을 수 있는 크리스탈이 있다면 해당 크리스탈을 획득 (객체삭제 / 타일에서 삭제 / 크리스탈 수치 증가) 후
        // 본 패킷을 주변 클라이언트 모두 에게 뿌린다. (행위 플레이어 포함)
        //
        //
        //	{
        //		WORD	Type
        //
        //		INT64	ClientID
        //		INT64	CristalClientID
        //		int		AmountCristal		// 이 크리스탈을 획득한 유저의 크리스탈 보유량.
        //	}
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_PICK_CRISTAL,



        //------------------------------------------------------------
        // 플레이어 HP 보정.
        //
        // 서버측에서는 필요시 마다 본 패킷을 클라로 보내어 HP 수치를 맞춰줄 수 있음.
        // 수시로 보내는건 안좋으며, SIT 모드에서 일어나는 순간에 좋을듯.
        //
        //
        //	{
        //		WORD	Type
        //
        //		INT		HP
        //	}
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_PLAYER_HP,

        //------------------------------------------------------------
        // 플레이어 죽음
        //
        // 서버측에서 플레이어가 죽었을 시 클라이언트에 이 패킷을 보낸다.
        // 죽는 즉시 플레이어를 타일맵에서 제거하며, 섹터에서는 빼지 않는다.
        //
        // 왜냐면 죽어서 쓰러진 후에도 주변의 움직임이나 채팅 내용은 여전히 봐야하므로
        // 타일맵에서 제거하는 이유는 공격을 당하지 않기 위해서.
        //
        // 타일맵에서 제거 하지 않고 죽은 후에도 데미지를 먹게 해도 됨.
        // 
        //
        // 죽음 처리가 됨과 동시에 DB 에는 초기 리셋된 HP & 크리스탈 차감 1000 내외 & Die 플래그 1 로 DB 저장을 쏜다.
        // 그리고 이 정보를 플레이어 객체에 셋팅을 하지만 섹터와 타일맵에는 넣지 않는다.
        //
        // 여기서 바로 DB 에 저장하는 이유는 죽은 후에 꺼버리는 유저가 있기 때문,
        // 그리고 재접속시 죽었었다는 상태를 Die 컬럼으로 확인하여 위치만 부활 위치로 서버에서 잡아준다.
        // (이처럼 죽었다가 바로 꺼버리고 다음에 로그인이 된다면 로그인 과정에서 좌표잡고 Die 0 으로 저장함)
        //
        // 이후 플레이어가 다시하기 패킷을 보내준다면 그때 타일맵과 섹터에 넣어서 처음처럼 플레이를 진행한다.
        //
        //	{
        //		WORD	Type
        //		
        //		INT64	ClientID				// 죽은 캐릭터.
        //		int		MinusCristal			// 차감 크리스탈. 
        //	}
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_PLAYER_DIE,

        //------------------------------------------------------------
        // 플레이어 죽은 후 다시하기 패킷 (클라 > 서버)
        //
        // 플레이어 사망 후 클라이언트에서 다시하기 키를 누르면 이 패킷을 서버로 보냄.
        // 죽지않은 상태에서 이 패킷이 오면 무시해야 하며, 죽은 상태에서 이 패킷이 온다면
        // 부활위치로 이동시켜서 타일맵 셋팅, 섹터셋팅 후 처음 로그인시와 똑같이 진행한다.
        //
        // CREATE_MY_CHARACTER 부터 시작~
        //
        // 부활위치의 맵과 섹터에 넣은 뒤에 RES_PLAYER_RESTART 를 보내줘야 한다.
        // 왜냐면 클라이언트는 RES_PLAYER_RESTART 를 받은 직후 기존의 모든 오브젝트를 삭제 시키기 때문에
        // RES_PLAYER_RESTART 이전에 받은 내용의 객체들은 삭제가 안될 수 있음.
        //
        //	{
        //		WORD	Type
        //	}
        //
        // en_PACKET_CS_GAME_REQ_PLAYER_RESTART,
        // en_PACKET_CS_GAME_RES_PLAYER_RESTART,
        // 
        // 두개의 내용은 같음.
        //------------------------------------------------------------
        en_PACKET_CS_GAME_REQ_PLAYER_RESTART,
        en_PACKET_CS_GAME_RES_PLAYER_RESTART,




        //------------------------------------------------------------
        // 테스트용 에코 요청
        //
        //	{
        //		WORD		Type
        //
        //		INT64		AccountoNo
        //		LONGLONG	SendTick
        //	}
        //
        //------------------------------------------------------------	
        en_PACKET_CS_GAME_REQ_ECHO = 5000,

        //------------------------------------------------------------
        // 테스트용 에코 응답 (REQ 를 그대로 돌려줌)
        //
        //	{
        //		WORD		Type
        //
        //		INT64		AccountoNo
        //		LONGLONG	SendTick
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_GAME_RES_ECHO,

        //------------------------------------------------------------
        // 하트비트
        //
        //	{
        //		WORD		Type
        //	}
        //
        //
        // 클라이언트는 이를 30초마다 보내줌.
        // 서버는 40초 이상동안 메시지 수신이 없는 클라이언트를 강제로 끊어줘야 함.
        //------------------------------------------------------------	
        en_PACKET_CS_GAME_REQ_HEARTBEAT,





        ////////////////////////////////////////////////////////
        //
        //   Server & Server Protocol  / LAN 통신은 기본으로 응답을 받지 않음.
        //
        ////////////////////////////////////////////////////////
        en_PACKET_SS_LAN = 10000,
        //------------------------------------------------------
        // GameServer & LoginServer & ChatServer Protocol
        //------------------------------------------------------

        //------------------------------------------------------------
        // 다른 서버가 로그인 서버로 로그인.
        // 이는 응답이 없으며, 그냥 로그인 됨.  
        //
        //	{
        //		WORD	Type
        //
        //		BYTE	ServerType			// dfSERVER_TYPE_GAME / dfSERVER_TYPE_CHAT
        //
        //		WCHAR	ServerName[32]		// 해당 서버의 이름.  
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_SS_LOGINSERVER_LOGIN,



        //------------------------------------------------------------
        // 로그인서버에서 게임.채팅 서버로 새로운 클라이언트 접속을 알림.
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		CHAR	SessionKey[64]
        //	}
        //
        //------------------------------------------------------------
        // en_PACKET_SS_NEW_CLIENT_LOGIN,	// 신규 접속자의 세션키 전달패킷을 요청,응답구조로 변경 2017.01.05


        //------------------------------------------------------------
        // 로그인서버에서 게임.채팅 서버로 새로운 클라이언트 접속을 알림.
        //
        // 마지막의 Parameter 는 세션키 공유에 대한 고유값 확인을 위한 어떤 값. 이는 응답 결과에서 다시 받게 됨.
        // 채팅서버와 게임서버는 Parameter 에 대한 처리는 필요 없으며 그대로 Res 로 돌려줘야 합니다.
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		CHAR	SessionKey[64]
        //		INT64	Parameter
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_SS_REQ_NEW_CLIENT_LOGIN,

        //------------------------------------------------------------
        // 게임.채팅 서버가 새로운 클라이언트 접속패킷 수신결과를 돌려줌.
        // 게임서버용, 채팅서버용 패킷의 구분은 없으며, 로그인서버에 타 서버가 접속 시 CHAT,GAME 서버를 구분하므로 
        // 이를 사용해서 알아서 구분 하도록 함.
        //
        // 플레이어의 실제 로그인 완료는 이 패킷을 Chat,Game 양쪽에서 다 받았을 시점임.
        //
        // 마지막 값 Parameter 는 이번 세션키 공유에 대해 구분할 수 있는 특정 값
        // ClientID 를 쓰던, 고유 카운팅을 쓰던 상관 없음.
        //
        // 로그인서버에 접속과 재접속을 반복하는 경우 이전에 공유응답이 새로 접속한 뒤의 응답으로
        // 오해하여 다른 세션키를 들고 가는 문제가 생김.
        //
        //	{
        //		WORD	Type
        //
        //		INT64	AccountNo
        //		INT64	Parameter
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_SS_RES_NEW_CLIENT_LOGIN,





        //------------------------------------------------------
        // Monitor Server Protocol
        //------------------------------------------------------


        ////////////////////////////////////////////////////////
        //
        //   MonitorServer & MoniterTool Protocol / 응답을 받지 않음.
        //
        ////////////////////////////////////////////////////////

        //------------------------------------------------------
        // Monitor Server  Protocol
        //------------------------------------------------------
        en_PACKET_SS_MONITOR = 20000,
        //------------------------------------------------------
        // Server -> Monitor Protocol
        //------------------------------------------------------
        //------------------------------------------------------------
        // LoginServer, GameServer , ChatServer  가 모니터링 서버에 로그인 함
        //
        // 
        //	{
        //		WORD	Type
        //
        //		int		ServerNo		//  각 서버마다 고유 번호를 부여하여 사용
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_SS_MONITOR_LOGIN,

        //------------------------------------------------------------
        // 서버가 모니터링서버로 데이터 전송
        // 각 서버는 자신이 모니터링중인 수치를 1초마다 모니터링 서버로 전송.
        //
        // 서버의 다운 및 기타 이유로 모니터링 데이터가 전달되지 못할떄를 대비하여 TimeStamp 를 전달한다.
        // 이는 모니터링 클라이언트에서 계산,비교 사용한다.
        // 
        //	{
        //		WORD	Type
        //
        //		BYTE	DataType				// 모니터링 데이터 Type 하단 Define 됨.
        //		int		DataValue				// 해당 데이터 수치.
        //		int		TimeStamp				// 해당 데이터를 얻은 시간 TIMESTAMP  (time() 함수)
        //										// 본래 time 함수는 time_t 타입변수이나 64bit 로 낭비스러우니
        //										// int 로 캐스팅하여 전송. 그래서 2038년 까지만 사용가능
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_SS_MONITOR_DATA_UPDATE,


        en_PACKET_CS_MONITOR = 25000,
        //------------------------------------------------------
        // Monitor -> Monitor Tool Protocol  (Client <-> Server 프로토콜)
        //------------------------------------------------------
        //------------------------------------------------------------
        // 모니터링 클라이언트(툴) 이 모니터링 서버로 로그인 요청
        //
        //	{
        //		WORD	Type
        //
        //		char	LoginSessionKey[32]		// 로그인 인증 키. (이는 모니터링 서버에 고정값으로 보유)
        //										// 각 모니터링 툴은 같은 키를 가지고 들어와야 함
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_MONITOR_TOOL_REQ_LOGIN,

        //------------------------------------------------------------
        // 모니터링 클라이언트(툴) 모니터링 서버로 로그인 응답
        //
        //	{
        //		WORD	Type
        //
        //		BYTE	Status					// 로그인 결과 0 / 1 / 2 ... 하단 Define
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_MONITOR_TOOL_RES_LOGIN,

        //------------------------------------------------------------
        // 모니터링 서버가 모니터링 클라이언트(툴) 에게 모니터링 데이터 전송
        // 
        // 통합 모니터링 방식을 사용 중이므로, 모니터링 서버는 모든 모니터링 클라이언트에게
        // 수집되는 모든 데이터를 바로 전송시켜 준다.
        // 
        //
        // 데이터를 절약하기 위해서는 초단위로 모든 데이터를 묶어서 30~40개의 모니터링 데이터를 하나의 패킷으로 만드는게
        // 좋으나  여러가지 생각할 문제가 많으므로 그냥 각각의 모니터링 데이터를 개별적으로 전송처리 한다.
        //
        //	{
        //		WORD	Type
        //		
        //		BYTE	ServerNo				// 서버 No
        //		BYTE	DataType				// 모니터링 데이터 Type 하단 Define 됨.
        //		int		DataValue				// 해당 데이터 수치.
        //		int		TimeStamp				// 해당 데이터를 얻은 시간 TIMESTAMP  (time() 함수)
        //										// 본래 time 함수는 time_t 타입변수이나 64bit 로 낭비스러우니
        //										// int 로 캐스팅하여 전송. 그래서 2038년 까지만 사용가능
        //	}
        //
        //------------------------------------------------------------
        en_PACKET_CS_MONITOR_TOOL_DATA_UPDATE,
    };



    enum en_PACKET_CS_LOGIN_RES_LOGIN
    {
        dfLOGIN_STATUS_NONE = -1,       // 미인증상태
        dfLOGIN_STATUS_FAIL = 0,        // 세션오류
        dfLOGIN_STATUS_OK = 1,      // 성공
        dfLOGIN_STATUS_GAME = 2,        // 게임중
        dfLOGIN_STATUS_ACCOUNT_MISS = 3,        // account 테이블에 AccountNo 없음
        dfLOGIN_STATUS_SESSION_MISS = 4,        // Session 테이블에 AccountNo 없음
        dfLOGIN_STATUS_STATUS_MISS = 5,     // Status 테이블에 AccountNo 없음
        dfLOGIN_STATUS_NOSERVER = 6,        // 서비스중인 서버가 없음.
    };


    enum en_PACKET_CS_GAME_RES_LOGIN
    {
        dfGAME_LOGIN_FAIL = 0,      // 세션키 오류 또는 Account 데이블상의 오류
        dfGAME_LOGIN_OK = 1,        // 성공
        dfGAME_LOGIN_NOCHARACTER = 2,       // 성공 / 캐릭터 없음 > 캐릭터 선택화면으로 전환. 
        dfGAME_LOGIN_VERSION_MISS = 3,      // 서버,클라 버전 다름
    };



    enum en_PACKET_SS_LOGINSERVER_LOGIN
    {
        dfSERVER_TYPE_GAME = 1,
        dfSERVER_TYPE_CHAT = 2,
        dfSERVER_TYPE_MONITOR = 3,
    };

    enum en_PACKET_SS_HEARTBEAT
    {
        dfTHREAD_TYPE_WORKER = 1,
        dfTHREAD_TYPE_DB = 2,
        dfTHREAD_TYPE_GAME = 3,
    };

    // en_PACKET_SS_MONITOR_LOGIN
    enum en_PACKET_CS_MONITOR_TOOL_SERVER_CONTROL
    {
        dfMONITOR_SERVER_TYPE_LOGIN = 1,
        dfMONITOR_SERVER_TYPE_GAME = 2,
        dfMONITOR_SERVER_TYPE_CHAT = 3,
        dfMONITOR_SERVER_TYPE_AGENT = 4,

        dfMONITOR_SERVER_CONTROL_SHUTDOWN = 1,      // 서버 정상종료 (게임서버 전용)
        dfMONITOR_SERVER_CONTROL_TERMINATE = 2,     // 서버 프로세스 강제종료
        dfMONITOR_SERVER_CONTROL_RUN = 3,       // 서버 프로세스 생성 & 실행
    };


    enum en_PACKET_SS_MONITOR_DATA_UPDATE
    {
        dfMONITOR_DATA_TYPE_LOGIN_SERVER_RUN = 1,       // 로그인서버 실행여부 ON / OFF
        dfMONITOR_DATA_TYPE_LOGIN_SERVER_CPU = 2,       // 로그인서버 CPU 사용률
        dfMONITOR_DATA_TYPE_LOGIN_SERVER_MEM = 3,       // 로그인서버 메모리 사용 MByte
        dfMONITOR_DATA_TYPE_LOGIN_SESSION = 4,      // 로그인서버 세션 수 (컨넥션 수)
        dfMONITOR_DATA_TYPE_LOGIN_AUTH_TPS = 5,     // 로그인서버 인증 처리 초당 횟수
        dfMONITOR_DATA_TYPE_LOGIN_PACKET_POOL = 6,      // 로그인서버 패킷풀 사용량


        dfMONITOR_DATA_TYPE_GAME_SERVER_RUN = 10,       // GameServer 실행 여부 ON / OFF
        dfMONITOR_DATA_TYPE_GAME_SERVER_CPU = 11,       // GameServer CPU 사용률
        dfMONITOR_DATA_TYPE_GAME_SERVER_MEM = 12,       // GameServer 메모리 사용 MByte
        dfMONITOR_DATA_TYPE_GAME_SESSION = 13,      // 게임서버 세션 수 (컨넥션 수)
        dfMONITOR_DATA_TYPE_GAME_AUTH_PLAYER = 14,      // 게임서버 AUTH MODE 플레이어 수
        dfMONITOR_DATA_TYPE_GAME_GAME_PLAYER = 15,      // 게임서버 GAME MODE 플레이어 수
        dfMONITOR_DATA_TYPE_GAME_ACCEPT_TPS = 16,       // 게임서버 Accept 처리 초당 횟수
        dfMONITOR_DATA_TYPE_GAME_PACKET_RECV_TPS = 17,      // 게임서버 패킷처리 초당 횟수
        dfMONITOR_DATA_TYPE_GAME_PACKET_SEND_TPS = 18,      // 게임서버 패킷 보내기 초당 완료 횟수
        dfMONITOR_DATA_TYPE_GAME_DB_WRITE_TPS = 19,     // 게임서버 DB 저장 메시지 초당 처리 횟수
        dfMONITOR_DATA_TYPE_GAME_DB_WRITE_MSG = 20,     // 게임서버 DB 저장 메시지 큐 개수 (남은 수)
        dfMONITOR_DATA_TYPE_GAME_AUTH_THREAD_FPS = 21,      // 게임서버 AUTH 스레드 초당 프레임 수 (루프 수)
        dfMONITOR_DATA_TYPE_GAME_GAME_THREAD_FPS = 22,      // 게임서버 GAME 스레드 초당 프레임 수 (루프 수)
        dfMONITOR_DATA_TYPE_GAME_PACKET_POOL = 23,      // 게임서버 패킷풀 사용량

        dfMONITOR_DATA_TYPE_CHAT_SERVER_RUN = 30,       // 에이전트 ChatServer 실행 여부 ON / OFF
        dfMONITOR_DATA_TYPE_CHAT_SERVER_CPU = 31,       // 에이전트 ChatServer CPU 사용률
        dfMONITOR_DATA_TYPE_CHAT_SERVER_MEM = 32,       // 에이전트 ChatServer 메모리 사용 MByte
        dfMONITOR_DATA_TYPE_CHAT_SESSION = 33,      // 채팅서버 세션 수 (컨넥션 수)
        dfMONITOR_DATA_TYPE_CHAT_PLAYER = 34,       // 채팅서버 인증성공 사용자 수 (실제 접속자)
        dfMONITOR_DATA_TYPE_CHAT_UPDATE_TPS = 35,       // 채팅서버 UPDATE 스레드 초당 초리 횟수
        dfMONITOR_DATA_TYPE_CHAT_PACKET_POOL = 36,      // 채팅서버 패킷풀 사용량
        dfMONITOR_DATA_TYPE_CHAT_UPDATEMSG_POOL = 37,       // 채팅서버 UPDATE MSG 풀 사용량


        dfMONITOR_DATA_TYPE_MONITOR_CPU_TOTAL = 40,     // 서버컴퓨터 CPU 전체 사용률
        dfMONITOR_DATA_TYPE_MONITOR_NONPAGED_MEMORY = 41,       // 서버컴퓨터 논페이지 메모리 MByte
        dfMONITOR_DATA_TYPE_MONITOR_NETWORK_RECV = 42,      // 서버컴퓨터 네트워크 수신량 KByte
        dfMONITOR_DATA_TYPE_MONITOR_NETWORK_SEND = 43,      // 서버컴퓨터 네트워크 송신량 KByte
        dfMONITOR_DATA_TYPE_MONITOR_AVAILABLE_MEMORY = 44,      // 서버컴퓨터 사용가능 메모리

        dfMONITOR_DATA_TYPE_END                             // enum 마지막 값
    };


    enum en_PACKET_CS_MONITOR_TOOL_RES_LOGIN
    {
        dfMONITOR_TOOL_LOGIN_OK = 1,        // 로그인 성공
        dfMONITOR_TOOL_LOGIN_ERR_NOSERVER = 2,      // 서버이름 오류 (매칭미스)
        dfMONITOR_TOOL_LOGIN_ERR_SESSIONKEY = 3,        // 로그인 세션키 오류
    };



    //#endif
}
