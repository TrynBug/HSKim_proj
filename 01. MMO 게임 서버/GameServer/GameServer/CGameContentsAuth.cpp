
#include "CGameServer.h"
#include "CGameContents.h"
#include "CommonProtocol.h"

#include "CDBConnector.h"

//#define ENABLE_PROFILER
#include "profiler.h"

#include "CGameContentsAuth.h"
#include "logger.h"

CGameContentsAuth::CGameContentsAuth(EContentsThread myContentsType, CGameServer* pGameServer, int FPS)
	//: CGameContents(myContentsType, pGameServer, FPS, CONTENTS_MODE_SEND_PACKET_IMMEDIATELY)
	: CGameContents(myContentsType, pGameServer, FPS)
	, _sessionTransferLimit(1000000)
{
	_pDBConn = new CDBConnector;
	_pDBConn->Connect("127.0.0.1", "root", "vmfhzkepal!", "gamedb", 3306);
}

CGameContentsAuth::~CGameContentsAuth()
{
	_pDBConn->Disconnect();
	delete _pDBConn;
}



/* virtual Get */
// @Override
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
void CGameContentsAuth::OnRecv(__int64 sessionId, game_netserver::CPacket* pPacket)
{
	// ����ID�� �÷��̾� ��ü�� ��´�.
	auto iter = _mapPlayer.find(sessionId);
	CPlayer* pPlayer = iter->second;
	pPlayer->SetHeartBeatTime();

	// �޽��� Ÿ�� Ȯ��
	WORD msgType;
	*pPacket >> msgType;

	game_netserver::CPacket* pSendPacket = AllocPacket();

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

		*pPacket >> accountNo;
		pPacket->TakeData(sessionKey, 64);
		*pPacket >> version;


		// redis���� ����key get
		cpp_redis::client* pRedisClient = GetRedisClient();
		std::string strAccountNo = std::to_string(accountNo);
		std::future<cpp_redis::reply> redisFuture = pRedisClient->get(strAccountNo);
		pRedisClient->sync_commit();
		cpp_redis::reply redisReply = redisFuture.get();

		// ����key Ȯ��
		if (redisReply.is_null())
		{
			// ����key�� ������ �α��� ���� ����
			*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)0 << accountNo;
			SendPacketAndDisconnect(pPlayer->_sessionId, pSendPacket);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN. login failed by no session key. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
			break;
		}
		std::string redisSessionKey = redisReply.as_string();
		if (memcmp(redisSessionKey.c_str(), sessionKey, 64) != 0)
		{
			// ����key�� �ٸ��� �α��� ���� ����
			*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)0 << accountNo;
			SendPacketAndDisconnect(pPlayer->_sessionId, pSendPacket);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN. login failed by invalid session key. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
			break;
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
			*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)0 << accountNo;
			SendPacketAndDisconnect(pPlayer->_sessionId, pSendPacket);
			_pDBConn->FreeResult(myRes);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN. login failed by no account data. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
			break;
		}

		// fetch row
		MYSQL_ROW myRow = _pDBConn->FetchRow(myRes);
		// �÷��̾� ID ����
		pPlayer->SetAccountInfo(myRow[0], myRow[1], GetSessionIP(sessionId), GetSessionPort(sessionId));

		// charactertype�� NULL�̶�� �ű�ĳ���� ���� ���� �� ����
		if (myRow[2] == nullptr)
		{
			*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)2 << accountNo;
			SendPacket(pPlayer->_sessionId, pSendPacket);
			_pDBConn->FreeResult(myRes);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN. select character. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
			break;
		}

		// �÷��̾� ���� �ε�
		pPlayer->LoadCharacterInfo((BYTE)std::stoi(myRow[2]), (float)std::stod(myRow[3]), (float)std::stod(myRow[4]), std::stoi(myRow[5]), std::stoi(myRow[6])
			, (USHORT)std::stoi(myRow[7]), std::stoi(myRow[8]), std::stoi(myRow[9]), std::stoll(myRow[10]), (USHORT)std::stoi(myRow[11]), std::stoi(myRow[12]));
		_pDBConn->FreeResult(myRes);

		// �α��� ���� ����
		*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_LOGIN << (BYTE)1 << accountNo;
		SendPacket(pPlayer->_sessionId, pSendPacket);

		// �ʵ� �������� �̵�
		_mapPlayer.erase(iter);
		TransferSession(pPlayer->_sessionId, EContentsThread::FIELD, pPlayer);

		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_LOGIN. login succeed. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
		break;
	}

	// ĳ���� ���� ��û
	case en_PACKET_CS_GAME_REQ_CHARACTER_SELECT:
	{
		BYTE characterType;
		*pPacket >> characterType;

		// �α����� ������ ��� ���� ����
		if (pPlayer->_bLogin == false)
		{
			*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)0;
			SendPacketAndDisconnect(pPlayer->_sessionId, pSendPacket);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_CHARACTER_SELECT. disconnect by not login. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
			break;
		}

		// ĳ���� Ÿ���� 1 ~ 5�� �ƴ� ��� ���� ����
		if (characterType < 1 || characterType > 5)
		{
			*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)0;
			SendPacketAndDisconnect(pPlayer->_sessionId, pSendPacket);
			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_CHARACTER_SELECT. disconnect by invalid character type. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
			break;
		}

		// �÷��̾� ĳ���� ���� ���� (DB�� �÷��̾� ������ �����ϴ°��� ���� ���������� ��)
		pPlayer->SetCharacterInfo(characterType);

		// ĳ���� ���� ���� ����
		*pSendPacket << (WORD)en_PACKET_CS_GAME_RES_CHARACTER_SELECT << (BYTE)1;
		SendPacket(pPlayer->_sessionId, pSendPacket);

		// �ʵ� �������� �̵�
		_mapPlayer.erase(iter);
		TransferSession(pPlayer->_sessionId, EContentsThread::FIELD, pPlayer);

		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnRecv::en_PACKET_CS_GAME_REQ_CHARACTER_SELECT. succeed character select. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
		break;
	}

	// ��Ʈ��Ʈ
	case en_PACKET_CS_GAME_REQ_HEARTBEAT:
	{
		pPlayer->SetHeartBeatTime();
		break;
	}

	// �߸��� �޽����� ���� ���
	default:
	{
		// ������ ������ ���´�. ������ ������ ���̻� ������ detach�Ǿ� ���̻� �޽����� ó������ ����
		DisconnectSession(pPlayer->_sessionId);
		_disconnByInvalidMessageType++; // ����͸�

		LOGGING(LOGGING_LEVEL_DEBUG, L"CContentsAuth::OnRecv::DEFAULT. sessionId:%lld, accountNo:%lld, msg type:%d\n", pPlayer->_sessionId, pPlayer->_accountNo, msgType);
		break;
	}
	}

	pSendPacket->SubUseCount();

}

// �����忡 ������ ������ �� ȣ���
void CGameContentsAuth::OnSessionJoin(__int64 sessionId, PVOID data)
{
	CPlayer* pPlayer;
	if (data == nullptr)
	{
		// ���� ������ ������ ��� �÷��̾� ��ü�� �����Ѵ�.
		pPlayer = AllocPlayer();
		pPlayer->Init(sessionId);
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnSessionJoin. initial join. sessionId:%lld\n", pPlayer->_sessionId);
	}
	else
	{
		// �Ķ���Ϳ��� �÷��̾� ��ü�� ��´�.
		pPlayer = (CPlayer*)data;
		LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsAuth::OnSessionJoin. transfer join. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);
	}
	
	// �÷��̾ map�� ����Ѵ�.
	_mapPlayer.insert(std::make_pair(sessionId, pPlayer));
}


// ������ ������ ������ ������ �������� ȣ���
void CGameContentsAuth::OnSessionDisconnected(__int64 sessionId)
{
	// �÷��̾� ��ü ���
	auto iter = _mapPlayer.find(sessionId);
	if (iter == _mapPlayer.end())
		return;

	CPlayer* pPlayer = iter->second;

	LOGGING(LOGGING_LEVEL_DEBUG, L"CContentsAuth::OnSessionDisconnected. sessionId:%lld, accountNo:%lld\n", pPlayer->_sessionId, pPlayer->_accountNo);

	// �÷��̾ map���� �����ϰ� free
	_mapPlayer.erase(iter);
	FreePlayer(pPlayer);

}





