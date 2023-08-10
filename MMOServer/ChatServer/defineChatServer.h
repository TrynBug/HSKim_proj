#pragma once

namespace netlib
{
	class CPacket;
}

namespace chatserver
{
	class CObject;
	class CPlayer;

	using CPacket = netlib::CPacket;
	using CObject_t = std::shared_ptr<CObject>;
	using CPlayer_t = std::shared_ptr<CPlayer>;


	/* ���� */
	constexpr int SECTOR_MAX_Y = 50;
	constexpr int SECTOR_MAX_X = 50;

	/* �޽��� */
	constexpr int MSG_FROM_CLIENT = 1;
	constexpr int MSG_FROM_SERVER = 2;

	/* �÷��̾� */
	constexpr WORD SECTOR_NOT_SET = 0xffff;  // ���Ͱ� ���� �������� �ʾ��� ����� ���� ��ǥ��

	enum class EServerMsgType
	{
		MSG_JOIN_PLAYER,
		MSG_LEAVE_PLAYER,
		MSG_CHECK_HEART_BEAT_TIMEOUT,
		MSG_CHECK_LOGIN_TIMEOUT,
		MSG_SHUTDOWN,
		MSG_LOGIN_REDIS_GET,
	};

	struct MsgChatServer
	{
		unsigned short msgFrom;    // Ŭ���̾�Ʈ���� ���� �޽�������, ���� ���� ��Ʈ�� �޽������� ����
		__int64        sessionId;
		CPacket* pPacket;
		EServerMsgType eServerMsgType;  // ���� ���� ��Ʈ�� �޽����ϰ�� �޽��� Ÿ��
	};

	// �񵿱� redis get ȣ�� �� completion routine �Լ��� ������ ������
	class CChatServer;
	struct StAPCData
	{
		CChatServer* pChatServer;
		CPacket* pPacket;
		__int64 sessionId;
		__int64 accountNo;
		bool isNull;
		char sessionKey[64];
	};


}