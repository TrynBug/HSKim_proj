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
	// config ���� json parser ���
	const CJsonParser& json = GetConfig().GetJsonParser();

	// set config
	int AuthSessionPacketProcessLimit = json[L"ContentsAuthentication"][L"SessionPacketProcessLimit"].Int();
	int AuthSessionTransferLimit = json[L"ContentsAuthentication"][L"SessionTransferLimit"].Int();
	int AuthHeartBeatTimeout = json[L"ContentsAuthentication"][L"HeartBeatTimeout"].Int();

	SetSessionPacketProcessLimit(AuthSessionPacketProcessLimit);
	EnableHeartBeatTimeout(AuthHeartBeatTimeout);
	SetSessionTransferLimit(AuthSessionTransferLimit);

	// DB ����
	_pDBConn = std::make_unique<CDBConnector>();
	_pDBConn->Connect("127.0.0.1", "root", "vmfhzkepal!", "gamedb", 3306);

}

/* Get DB ����͸� */
__int64 CGameContentsAuth::GetQueryRunCount() const { return _pDBConn->GetQueryRunCount(); }
float CGameContentsAuth::GetMaxQueryRunTime() const { return _pDBConn->GetMaxQueryRunTime(); }
float CGameContentsAuth::GetMinQueryRunTime() const { return _pDBConn->GetMinQueryRunTime(); }
float CGameContentsAuth::GetAvgQueryRunTime() const { return _pDBConn->GetAvgQueryRunTime(); }



/* ������ Ŭ���� callback �Լ� ���� */
 // �ֱ������� ȣ���
void CGameContentsAuth::OnUpdate()
{
}

// �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
void CGameContentsAuth::OnRecv(__int64 sessionId, CPacket& packet)
{
	// �޽��� Ÿ�� Ȯ��
	WORD msgType;
	packet >> msgType;

	// �޽��� Ÿ�Կ� ���� ó��
	switch (msgType)
	{
	// �α��� ��û�� ���
	case en_PACKET_CS_GAME_REQ_LOGIN:
		PacketProc_LoginRequest(sessionId, packet);
		break;
	// ĳ���� ���� ��û
	case en_PACKET_CS_GAME_REQ_CHARACTER_SELECT:
		PacketProc_CharacterSelect(sessionId, packet);
		break;
	// ��Ʈ��Ʈ
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		PacketProc_HeartBeat(sessionId, packet);
		break;
	// �߸��� �޽����� ���� ���
	default:
		PacketProc_Default(sessionId, packet, msgType);
		break;
	}
}

// �����忡 �������� ������ ������ �� ȣ���
void CGameContentsAuth::OnInitialSessionJoin(__int64 sessionId)
{
	// ���� ������ ������ ��� �÷��̾� ��ü�� �����Ѵ�.
	CPlayer_t pPlayer = AllocPlayer();
	pPlayer->Init(sessionId);

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnInitialSessionJoin. initial join. sessionId:%lld\n", pPlayer->GetSessionId());

	// �÷��̾ map�� ����Ѵ�.
	InsertPlayerToMap(sessionId, std::move(pPlayer));
}


void CGameContentsAuth::OnPlayerJoin(__int64 sessionId, CPlayer_t pPlayer)
{
	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnPlayerJoin. transfer join. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());

	// �÷��̾ map�� ����Ѵ�.
	InsertPlayerToMap(sessionId, std::move(pPlayer));
}


// ������ ������ ������ ������ �������� ȣ���
void CGameContentsAuth::OnSessionDisconnected(__int64 sessionId)
{
	// �÷��̾� ��ü ���
	auto iter = _mapPlayer.find(sessionId);
	if (iter == _mapPlayer.end())
		return;

	CPlayer& player = *iter->second;

	LOGGING(LOGGING_LEVEL_DEBUG, L"CContentsAuth::OnSessionDisconnected. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());

	// �÷��̾ map���� ����
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

	// redis���� ����key get
	cpp_redis::client& redisClient = GetRedisClient();
	std::string strAccountNo = std::to_string(accountNo);
	std::future<cpp_redis::reply> redisFuture = redisClient.get(strAccountNo);
	redisClient.sync_commit();
	cpp_redis::reply redisReply = redisFuture.get();

	// ����key Ȯ��
	if (redisReply.is_null())
	{
		// ����key�� ������ �α��� ���� ����
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
		// ����key�� �ٸ��� �α��� ���� ����
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)0 << accountNo;
		SendPacketAndDisconnect(pPlayer->GetSessionId(), sendPacket);
		sendPacket.SubUseCount();
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_LoginRequest. login failed by invalid session key. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
		return;
	}

	// �÷��̾� �α��� ó��
	pPlayer->SetLogin(accountNo, sessionKey);


	// DB���� ��������, �÷��̾����� ��ȸ
	MYSQL_RES* myRes = _pDBConn->ExecuteQueryAndGetResult(
		L"SELECT a.`userid`, a.`usernick`, b.`charactertype`, b.`posx`, b.`posy`, b.`tilex`, b.`tiley`, b.`rotation`, b.`crystal`, b.`hp`, b.`exp`, b.`level`, b.`die`"
		L" FROM `accountdb`.`account` a"
		L" LEFT JOIN `gamedb`.`character` b"
		L" ON a.`accountno` = b.`accountno`"
		L" WHERE a.`accountno` = %lld"
		, accountNo);

	// ��ȸ�� ������ ���ٸ� ����
	int numRows = (int)_pDBConn->GetNumQueryRows(myRes);
	if (numRows == 0)
	{
		// �α��� ���� ����
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
	// �÷��̾� ID ����
	pPlayer->SetAccountInfo(myRow[0], myRow[1], GetSessionIP(sessionId), GetSessionPort(sessionId));

	// charactertype�� NULL�̶�� �ű�ĳ���� ���� ���� �� ����
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

	// �÷��̾� ���� �ε�
	pPlayer->LoadCharacterInfo((BYTE)std::stoi(myRow[2]), (float)std::stod(myRow[3]), (float)std::stod(myRow[4]), std::stoi(myRow[5]), std::stoi(myRow[6])
		, (USHORT)std::stoi(myRow[7]), std::stoi(myRow[8]), std::stoi(myRow[9]), std::stoll(myRow[10]), (USHORT)std::stoi(myRow[11]), std::stoi(myRow[12]));
	_pDBConn->FreeResult(myRes);

	// �α��� ���� ����
	CPacket& sendPacket = AllocPacket();
	sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)1 << accountNo;
	SendPacket(pPlayer->GetSessionId(), sendPacket);
	sendPacket.SubUseCount();

	// �ʵ� �������� �̵�
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

	// �α����� ������ ��� ���� ����
	if (pPlayer->IsLogIn() == false)
	{
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)0;
		SendPacketAndDisconnect(pPlayer->GetSessionId(), sendPacket);
		sendPacket.SubUseCount();
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_CharacterSelect. disconnect by not login. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
		return;
	}

	// ĳ���� Ÿ���� 1 ~ 5�� �ƴ� ��� ���� ����
	if (characterType < 1 || characterType > 5)
	{
		CPacket& sendPacket = AllocPacket();
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)0;
		SendPacketAndDisconnect(pPlayer->GetSessionId(), sendPacket);
		sendPacket.SubUseCount();
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_CharacterSelect. disconnect by invalid character type. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());
		return;
	}

	// �÷��̾� ĳ���� ���� ���� (DB�� �÷��̾� ������ �����ϴ°��� ���� ���������� ��)
	pPlayer->SetCharacterInfo(characterType);

	// ĳ���� ���� ���� ����
	CPacket& sendPacket = AllocPacket();
	sendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)1;
	SendPacket(pPlayer->GetSessionId(), sendPacket);
	sendPacket.SubUseCount();

	// �ʵ� �������� �̵�
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

	// ������ ������ ���´�. ������ ������ ���̻� ������ detach�Ǿ� ���̻� �޽����� ó������ ����
	DisconnectSession(pPlayer->GetSessionId());
	_disconnByInvalidMessageType++;

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::PacketProc_Default. sessionId:%lld, accountNo:%lld, msg type:%d\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo(), msgType);
}