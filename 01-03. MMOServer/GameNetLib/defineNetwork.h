#pragma once

namespace netlib_game
{
	constexpr int SIZE_RECV_BUFFER = 10000;

	constexpr int IO_SEND = 1;
	constexpr int IO_RECV = 2;

	struct OVERLAPPED_EX
	{
		OVERLAPPED overlapped;
		int ioType;
	};

	#pragma pack(push, 1)
	struct PacketHeader
	{
		BYTE code;
		unsigned short len;
		BYTE randKey;
		BYTE checkSum;
	};
	#pragma pack(pop)

	enum class EMsgContentsType
	{
		INITIAL_JOIN_SESSION,
		JOIN_SESSION,
		SHUTDOWN,
	};

	class CSession;
	struct MsgContents
	{
		EMsgContentsType type;
		CSession* pSession;
		PVOID data;
	};

}

