
#include "CGameServer.h"
#include "CGameContents.h"
#include "CommonProtocol.h"

#include "../utils/CDBConnector.h"
//#define ENABLE_PROFILER
#include "../utils/profiler.h"
#include "../utils/logger.h"

#include "CGameContentsAuth.h"


CGameContentsAuth::CGameContentsAuth(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS)
	//: CGameContents(myContentsType, pGameServer, FPS, CONTENTS_MODE_SEND_PACKET_IMMEDIATELY)
	: CGameContents(myContentsType, std::move(pAccessor), FPS)
	, _sessionTransferLimit(1000000)
{
	_pDBConn = std::make_unique<CDBConnector>();
	_pDBConn->Connect("127.0.0.1", "root", "vmfhzkepal!", "gamedb", 3306);
}

CGameContentsAuth::~CGameContentsAuth()
{
	_pDBConn->Disconnect();
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
	// ����ID�� �÷��̾� ��ü�� ��´�.
	auto iter = _mapPlayer.find(sessionId);
	CPlayer& player = *iter->second;
	player.SetHeartBeatTime();

	// �޽��� Ÿ�� Ȯ��
	WORD msgType;
	packet >> msgType;

	CPacket& sendPacket = AllocPacket();

	// �޽��� Ÿ�Կ� ���� ó��
	switch (msgType)
	{
	// �α��� ��û�� ���
	case en_PACKET_CS_GAME_REQ_LOGIN:
	{
		PROFILE_BEGIN("ContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN");

		__int64 accountNo;
		char  sessionKey[64];
		int version;

		packet >> accountNo;
		packet.TakeData(sessionKey, 64);
		packet >> version;


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
			sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)0 << accountNo;
			SendPacketAndDisconnect(player.GetSessionId(), sendPacket);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN. login failed by no session key. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());
			break;
		}
		std::string redisSessionKey = redisReply.as_string();
		if (memcmp(redisSessionKey.c_str(), sessionKey, 64) != 0)
		{
			// ����key�� �ٸ��� �α��� ���� ����
			sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)0 << accountNo;
			SendPacketAndDisconnect(player.GetSessionId(), sendPacket);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN. login failed by invalid session key. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());
			break;
		}

		// �÷��̾� �α��� ó��
		player.SetLogin(accountNo, sessionKey);


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
			sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)0 << accountNo;
			SendPacketAndDisconnect(player.GetSessionId(), sendPacket);
			_pDBConn->FreeResult(myRes);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN. login failed by no account data. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());
			break;
		}

		// fetch row
		MYSQL_ROW myRow = _pDBConn->FetchRow(myRes);
		// �÷��̾� ID ����
		player.SetAccountInfo(myRow[0], myRow[1], GetSessionIP(sessionId), GetSessionPort(sessionId));

		// charactertype�� NULL�̶�� �ű�ĳ���� ���� ���� �� ����
		if (myRow[2] == nullptr)
		{
			sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)2 << accountNo;
			SendPacket(player.GetSessionId(), sendPacket);
			_pDBConn->FreeResult(myRes);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN. select character. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());
			break;
		}

		// �÷��̾� ���� �ε�
		player.LoadCharacterInfo((BYTE)std::stoi(myRow[2]), (float)std::stod(myRow[3]), (float)std::stod(myRow[4]), std::stoi(myRow[5]), std::stoi(myRow[6])
			, (USHORT)std::stoi(myRow[7]), std::stoi(myRow[8]), std::stoi(myRow[9]), std::stoll(myRow[10]), (USHORT)std::stoi(myRow[11]), std::stoi(myRow[12]));
		_pDBConn->FreeResult(myRes);

		// �α��� ���� ����
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)1 << accountNo;
		SendPacket(player.GetSessionId(), sendPacket);

		// �ʵ� �������� �̵�
		_mapPlayer.erase(iter);
		TransferSession(player.GetSessionId(), EContentsThread::FIELD, (PVOID)&player);

		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN. login succeed. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());
		break;
	}

	// ĳ���� ���� ��û
	case en_PACKET_CS_GAME_REQ_CHARACTER_SELECT:
	{
		BYTE characterType;
		packet >> characterType;

		// �α����� ������ ��� ���� ����
		if (player.IsLogIn() == false)
		{
			sendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)0;
			SendPacketAndDisconnect(player.GetSessionId(), sendPacket);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_CHARACTER_SELECT. disconnect by not login. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());
			break;
		}

		// ĳ���� Ÿ���� 1 ~ 5�� �ƴ� ��� ���� ����
		if (characterType < 1 || characterType > 5)
		{
			sendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)0;
			SendPacketAndDisconnect(player.GetSessionId(), sendPacket);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_CHARACTER_SELECT. disconnect by invalid character type. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());
			break;
		}

		// �÷��̾� ĳ���� ���� ���� (DB�� �÷��̾� ������ �����ϴ°��� ���� ���������� ��)
		player.SetCharacterInfo(characterType);

		// ĳ���� ���� ���� ����
		sendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)1;
		SendPacket(player.GetSessionId(), sendPacket);

		// �ʵ� �������� �̵�
		_mapPlayer.erase(iter);
		TransferSession(player.GetSessionId(), EContentsThread::FIELD, (PVOID)&player);

		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_CHARACTER_SELECT. succeed character select. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());
		break;
	}

	// ��Ʈ��Ʈ
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
	{
		player.SetHeartBeatTime();
		break;
	}

	// �߸��� �޽����� ���� ���
	default:
	{
		// ������ ������ ���´�. ������ ������ ���̻� ������ detach�Ǿ� ���̻� �޽����� ó������ ����
		DisconnectSession(player.GetSessionId());
		_disconnByInvalidMessageType++; // ����͸�

		LOGGING(LOGGING_LEVEL_DEBUG, L"CContentsAuth::OnRecv::DEFAULT. sessionId:%lld, accountNo:%lld, msg type:%d\n", player.GetSessionId(), player.GetAccountNo(), msgType);
		break;
	}
	}

	sendPacket.SubUseCount();

}

// �����忡 ������ ������ �� ȣ���
void CGameContentsAuth::OnSessionJoin(__int64 sessionId, PVOID data)
{
	if (data == nullptr)
	{
		// ���� ������ ������ ��� �÷��̾� ��ü�� �����Ѵ�.
		CPlayer& player = AllocPlayer();
		player.Init(sessionId);

		// �÷��̾ map�� ����Ѵ�.
		_mapPlayer.insert(std::make_pair(sessionId, &player));

		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnSessionJoin. initial join. sessionId:%lld\n", player.GetSessionId());
	}
	else
	{
		// �Ķ���Ϳ��� �÷��̾� ��ü�� ��´�.
		CPlayer& player = *(CPlayer*)data;

		// �÷��̾ map�� ����Ѵ�.
		_mapPlayer.insert(std::make_pair(sessionId, &player));

		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnSessionJoin. transfer join. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());
	}
	
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

	// �÷��̾ map���� �����ϰ� free
	_mapPlayer.erase(iter);
	FreePlayer(player);

}


void CGameContentsAuth::OnError(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_ERROR, szError, vaList);
	va_end(vaList);
}



