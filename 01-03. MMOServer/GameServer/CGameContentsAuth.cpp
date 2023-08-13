#include "stdafx.h"

#include "CGameServer.h"
#include "CGameContents.h"
#include "../common/CommonProtocol.h"

#include "CObject.h"
#include "CPlayer.h"

#include "../utils/CDBConnector.h"
//#define ENABLE_PROFILER
#include "../utils/profiler.h"
#include "../utils/logger.h"
#include "../utils/CJsonParser.h"

#include "CGameContentsAuth.h"

using namespace gameserver;

CGameContentsAuth::CGameContentsAuth(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS)
	//: CGameContents(myContentsType, pGameServer, FPS, CONTENTS_MODE_SEND_PACKET_IMMEDIATELY)
	: CGameContents(myContentsType, std::move(pAccessor), FPS)
	, _sessionTransferLimit(1000000)
{
}

CGameContentsAuth::~CGameContentsAuth()
{
	_pDBConn->Disconnect();
}


void CGameContentsAuth::Init()
{
	// config 파일 json parser 얻기
	const CJsonParser& json = GetConfig().GetJsonParser();

	// set config
	int AuthSessionPacketProcessLimit = json[L"ContentsAuthentication"][L"SessionPacketProcessLimit"].Int();
	int AuthSessionTransferLimit = json[L"ContentsAuthentication"][L"SessionTransferLimit"].Int();
	int AuthHeartBeatTimeout = json[L"ContentsAuthentication"][L"HeartBeatTimeout"].Int();

	SetSessionPacketProcessLimit(AuthSessionPacketProcessLimit);
	EnableHeartBeatTimeout(AuthHeartBeatTimeout);
	SetSessionTransferLimit(AuthSessionTransferLimit);

	// DB 연결
	_pDBConn = std::make_unique<CDBConnector>();
	_pDBConn->Connect("127.0.0.1", "root", "vmfhzkepal!", "gamedb", 3306);

}

/* Get DB 모니터링 */
__int64 CGameContentsAuth::GetQueryRunCount() const { return _pDBConn->GetQueryRunCount(); }
float CGameContentsAuth::GetMaxQueryRunTime() const { return _pDBConn->GetMaxQueryRunTime(); }
float CGameContentsAuth::GetMinQueryRunTime() const { return _pDBConn->GetMinQueryRunTime(); }
float CGameContentsAuth::GetAvgQueryRunTime() const { return _pDBConn->GetAvgQueryRunTime(); }



/* 컨텐츠 클래스 callback 함수 구현 */
 // 주기적으로 호출됨
void CGameContentsAuth::OnUpdate()
{
}

// 컨텐츠가 관리하는 세션의 메시지큐에 메시지가 들어왔을 때 호출됨
void CGameContentsAuth::OnRecv(__int64 sessionId, CPacket& packet)
{
	// 메시지 타입 확인
	WORD msgType;
	packet >> msgType;

	// 메시지 타입에 따른 처리
	switch (msgType)
	{
	// 로그인 요청일 경우
	case en_PACKET_CS_GAME_REQ_LOGIN:
		PacketProc_LoginRequest(sessionId, packet);
		break;
	// 캐릭터 선택 요청
	case en_PACKET_CS_GAME_REQ_CHARACTER_SELECT:
		PacketProc_CharacterSelect(sessionId, packet);
		break;
	// 하트비트
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		PacketProc_HeartBeat(sessionId, packet);
		break;
	// 잘못된 메시지를 받은 경우
	default:
		PacketProc_Default(sessionId, packet, msgType);
		break;
	}
}

// 스레드에 최초접속 세션이 들어왔을 때 호출됨
void CGameContentsAuth::OnInitialSessionJoin(__int64 sessionId)
{
	// 최초 접속한 세션일 경우 플레이어 객체를 생성한다.
	CPlayer_t pPlayer = AllocPlayer();
	pPlayer->Init(sessionId);

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnInitialSessionJoin. initial join. sessionId:%lld\n", pPlayer->GetSessionId());

	// 플레이어를 map에 등록한다.
	InsertPlayerToMap(sessionId, std::move(pPlayer));
}


void CGameContentsAuth::OnPlayerJoin(__int64 sessionId, CPlayer_t pPlayer)
{
	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnPlayerJoin. transfer join. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());

	// 플레이어를 map에 등록한다.
	InsertPlayerToMap(sessionId, std::move(pPlayer));
}


// 스레드 내에서 세션의 연결이 끊겼을때 호출됨
void CGameContentsAuth::OnSessionDisconnected(__int64 sessionId)
{
	// 플레이어 객체 얻기
	auto iter = _mapPlayer.find(sessionId);
	if (iter == _mapPlayer.end())
		return;

	CPlayer& player = *iter->second;

	LOGGING(LOGGING_LEVEL_DEBUG, L"CContentsAuth::OnSessionDisconnected. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());

	// 플레이어를 map에서 삭제
	ErasePlayerFromMap(sessionId);
}


void CGameContentsAuth::OnError(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_ERROR, szError, vaList);
	va_end(vaList);
}





/* packet processing */
void CGameContentsAuth::PacketProc_LoginRequest(__int64 sessionId, CPacket& packet)
{
	PROFILE_BEGIN("ContentsAuth::PacketProc_LoginRequest");

	__int64 accountNo;
	char  sessionKey[64];
	int version;

	packet >> accountNo;
	packet.TakeData(sessionKey, 64);
	packet >> version;

	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"CGameContentsAuth::PacketProc_LoginRequest. No player in the map. sessionId:%lld, accountNo:%lld\n", sessionId, accountNo);
		return;
	}

	// redis에서 세션key get
	cpp_redis::client& redisClient = GetRedisClient();
	std::string strAccountNo = std::to_string(accountNo);
	std::future<cpp_redis::reply> redisFuture = redisClient.get(strAccountNo);
	redisClient.sync_commit();
	cpp_redis::reply redisReply = redisFuture.get();

	// 세션key 확인
	if (redisReply.is_null())
	{
		// 세션key가 없으면 로그인 실패 응답
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)0 << accountNo;
		SendPacketAndDisconnect(pPlayer->GetSessionId(), sendPacket);
		sendPacket.SubUseCount();
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_LoginRequest. login failed by no session key. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
		return;
	}
	std::string redisSessionKey = redisReply.as_string();
	if (memcmp(redisSessionKey.c_str(), sessionKey, 64) != 0)
	{
		// 세션key가 다르면 로그인 실패 응답
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)0 << accountNo;
		SendPacketAndDisconnect(pPlayer->GetSessionId(), sendPacket);
		sendPacket.SubUseCount();
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_LoginRequest. login failed by invalid session key. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
		return;
	}

	// 플레이어 로그인 처리
	pPlayer->SetLogin(accountNo, sessionKey);


	// DB에서 계정정보, 플레이어정보 조회
	MYSQL_RES* myRes = _pDBConn->ExecuteQueryAndGetResult(
		L"SELECT a.`userid`, a.`usernick`, b.`charactertype`, b.`posx`, b.`posy`, b.`tilex`, b.`tiley`, b.`rotation`, b.`crystal`, b.`hp`, b.`exp`, b.`level`, b.`die`"
		L" FROM `accountdb`.`account` a"
		L" LEFT JOIN `gamedb`.`character` b"
		L" ON a.`accountno` = b.`accountno`"
		L" WHERE a.`accountno` = %lld"
		, accountNo);

	// 조회된 정보가 없다면 오류
	int numRows = (int)_pDBConn->GetNumQueryRows(myRes);
	if (numRows == 0)
	{
		// 로그인 실패 응답
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)0 << accountNo;
		SendPacketAndDisconnect(pPlayer->GetSessionId(), sendPacket);
		sendPacket.SubUseCount();
		_pDBConn->FreeResult(myRes);
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_LoginRequest. login failed by no account data. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
		return;
	}

	// fetch row
	MYSQL_ROW myRow = _pDBConn->FetchRow(myRes);
	// 플레이어 ID 세팅
	pPlayer->SetAccountInfo(myRow[0], myRow[1], GetSessionIP(sessionId), GetSessionPort(sessionId));

	// charactertype이 NULL이라면 신규캐릭터 선택 응답 후 종료
	if (myRow[2] == nullptr)
	{
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)2 << accountNo;
		SendPacket(pPlayer->GetSessionId(), sendPacket);
		sendPacket.SubUseCount();
		_pDBConn->FreeResult(myRes);
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_LoginRequest. select character. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
		return;
	}

	// 플레이어 정보 로드
	pPlayer->LoadCharacterInfo((BYTE)std::stoi(myRow[2]), (float)std::stod(myRow[3]), (float)std::stod(myRow[4]), std::stoi(myRow[5]), std::stoi(myRow[6])
		, (USHORT)std::stoi(myRow[7]), std::stoi(myRow[8]), std::stoi(myRow[9]), std::stoll(myRow[10]), (USHORT)std::stoi(myRow[11]), std::stoi(myRow[12]));
	_pDBConn->FreeResult(myRes);

	// 로그인 성공 응답
	CPacket& sendPacket = AllocPacket();
	sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)1 << accountNo;
	SendPacket(pPlayer->GetSessionId(), sendPacket);
	sendPacket.SubUseCount();

	// 필드 컨텐츠로 이동
	ErasePlayerFromMap(sessionId);
	TransferSession(pPlayer->GetSessionId(), EContentsThread::FIELD, pPlayer);

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_LoginRequest. login succeed. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
}


void CGameContentsAuth::PacketProc_CharacterSelect(__int64 sessionId, CPacket& packet)
{
	BYTE characterType;
	packet >> characterType;

	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"CGameContentsAuth::PacketProc_CharacterSelect. No player in the map. sessionId:%lld\n", sessionId);
		return;
	}

	// 로그인을 안했을 경우 실패 응답
	if (pPlayer->IsLogIn() == false)
	{
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)0;
		SendPacketAndDisconnect(pPlayer->GetSessionId(), sendPacket);
		sendPacket.SubUseCount();
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_CharacterSelect. disconnect by not login. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
		return;
	}

	// 캐릭터 타입이 1 ~ 5가 아닐 경우 실패 응답
	if (characterType < 1 || characterType > 5)
	{
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)0;
		SendPacketAndDisconnect(pPlayer->GetSessionId(), sendPacket);
		sendPacket.SubUseCount();
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_CharacterSelect. disconnect by invalid character type. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
		return;
	}

	// 플레이어 캐릭터 정보 세팅 (DB에 플레이어 정보를 저장하는것은 다음 컨텐츠에서 함)
	pPlayer->SetCharacterInfo(characterType);

	// 캐릭터 선택 성공 응답
	CPacket& sendPacket = AllocPacket();
	sendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)1;
	SendPacket(pPlayer->GetSessionId(), sendPacket);
	sendPacket.SubUseCount();

	// 필드 컨텐츠로 이동
	ErasePlayerFromMap(sessionId);
	TransferSession(pPlayer->GetSessionId(), EContentsThread::FIELD, pPlayer);

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_CharacterSelect. succeed character select. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
	return;
}

void CGameContentsAuth::PacketProc_HeartBeat(__int64 sessionId, CPacket& packet)
{
	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"CGameContentsAuth::PacketProc_HeartBeat. No player in the map. sessionId:%lld\n", sessionId);
		return;
	}
	pPlayer->SetHeartBeatTime();
}

void CGameContentsAuth::PacketProc_Default(__int64 sessionId, CPacket& packet, WORD msgType)
{
	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"CGameContentsAuth::PacketProc_Default. No player in the map. sessionId:%lld\n", sessionId);
		return;
	}

	// 세션의 연결을 끊는다. 연결을 끊으면 더이상 세션이 detach되어 더이상 메시지가 처리되지 않음
	DisconnectSession(pPlayer->GetSessionId());
	_disconnByInvalidMessageType++;

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_Default. sessionId:%lld, accountNo:%lld, msg type:%d\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo(), msgType);
}