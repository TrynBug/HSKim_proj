#pragma once

namespace netlib_s
{
	constexpr int SIZE_RECV_BUFFER = 5000;
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

}