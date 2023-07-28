#pragma once

/* ���� */
//#define CHAT_SERVER_PORT    12001
//#define dfPACKET_CODE		0x77
//#define dfPACKET_KEY		0x32

/* ���� */
#define dfSECTOR_MAX_Y      50
#define dfSECTOR_MAX_X      50

/* �޽��� */
#define MSG_FROM_CLIENT     1
#define MSG_FROM_SERVER     2

/* ��Ŷ */
//#define MAX_PACKET_SIZE     522 


enum class EServerMsgType
{
	MSG_JOIN_PLAYER,
	MSG_LEAVE_PLAYER,
	MSG_CHECK_HEART_BEAT_TIMEOUT,
	MSG_CHECK_LOGIN_TIMEOUT,
	MSG_SHUTDOWN,
	MSG_LOGIN_REDIS_GET,
};

namespace netserver
{
	class CPacket;
}
struct MsgChatServer
{
	unsigned short msgFrom;    // Ŭ���̾�Ʈ���� ���� �޽�������, ���� ���� ��Ʈ�� �޽������� ����
	__int64        sessionId;
	netserver::CPacket* pPacket;
	EServerMsgType eServerMsgType;  // ���� ���� ��Ʈ�� �޽����ϰ�� �޽��� Ÿ��
};


// �񵿱� redis get ȣ�� �� completion routine �Լ��� ������ ������
class CChatServer;
struct StAPCData
{
	CChatServer* pChatServer;
	netserver::CPacket* pPacket;
	__int64 sessionId;
	__int64 accountNo;
	bool isNull;
	char sessionKey[64];
};

/* �÷��̾� */
#define SECTOR_NOT_SET      0xffff   // ���Ͱ� ���� �������� �ʾ��� ����� ���� ��ǥ��




