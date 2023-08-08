#pragma once

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <process.h>
#include <vector>
#include <unordered_map>

#include "CPacket.h"
#include "../utils/CRingbuffer.h"
#include "../utils/CLockfreeQueue.h"

#include "defineLANClient.h"

// OnOutputDebug �Լ� ȣ�� ��ũ��
#define LAN_OUTPUT_DEBUG(szOutputFormat, ...) do { \
	if (_bOutputDebug) \
		OnOutputDebug(szOutputFormat, ##__VA_ARGS__); \
} while(0) 

// OnOutputSystem �Լ� ȣ�� ��ũ��
#define LAN_OUTPUT_SYSTEM(szOutputFormat, ...) do { \
	if (_bOutputSystem) \
		OnOutputSystem(szOutputFormat, ##__VA_ARGS__); \
} while(0) 

namespace lanlib
{
	class alignas(64) CLANClient
	{
	public:
		class Monitor; 

	public:
		CLANClient();
		virtual ~CLANClient();

		CLANClient(const CLANClient&) = delete;
		CLANClient(CLANClient&&) = delete;

		/* dynamic alloc */
		// 64byte aligned ��ü ������ ���� new, delete overriding
		void* operator new(size_t size);
		void operator delete(void* p);

	public:
		/* client */
		bool StartUp(const wchar_t* serverIP, unsigned short serverPort, bool bUseNagle, BYTE packetCode, BYTE packetKey, int maxPacketSize, bool bUsePacketEncryption);
		void Shutdown();    // ��Ʈ��ũ ����
		bool Disconnect();

		/* packet */
		CPacket& AllocPacket();
		bool SendPacket(CPacket& packet);  // send�� ����� ����� �� ���� ���� ����� ����ǰ�, �׷��� ���� ��� �񵿱�� ����ȴ�.
		bool SendPacketAsync(CPacket& packet);  // send�� �ݵ�� �񵿱�� ����ȴ�.
		bool PostInnerRequest(CPacket& packet);   // IOCP �Ϸ����� ť�� �۾��� �����Ѵ�. �۾��� ȹ��Ǹ� OnInnerRequest �Լ��� ȣ��ȴ�.

	private:
		/* Send, Recv */
		void RecvPost();   // WSARecv�� �Ѵ�.	
		void SendPost();   // WSASend�� �õ��Ѵ�. 
		void SendPostAsync();  // SendPacketAsync �Լ��� ���� �񵿱� send ��û�� �޾��� �� worker ������ ������ ȣ��ȴ�. WSASend�� ȣ���Ѵ�.

		/* ��Ŷ ��ȣȭ */
		void EncryptPacket(CPacket& packet);
		bool DecipherPacket(CPacket& packet);

	private:
		/* ���̺귯�� callback �Լ� */
		virtual bool OnRecv(CPacket& packet) = 0;
		virtual bool OnInnerRequest(CPacket& packet) { return false; }
		virtual void OnError(const wchar_t* szError, ...) = 0;            // ���� �߻�
		virtual void OnOutputDebug(const wchar_t* szError, ...) = 0;      // debug ���ڿ��� �Լ��� ����. �� �Լ��� ���� ȣ���������� NET_OUTPUT_DEBUG ��ũ�� ����� ������.
		virtual void OnOutputSystem(const wchar_t* szError, ...) = 0;     // system ���ڿ��� �Լ��� ����. �� �Լ��� ���� ȣ���������� NET_OUTPUT_SYSTEM ��ũ�� ����� ������.

	private:
		/* thread */
		static unsigned WINAPI ThreadWorker(PVOID pParam);
		void UpdateWorkerThread();

		/* Crash */
		void Crash();

	public:
		/* Set */
		void SetOutputDebug(bool enable) { _bOutputDebug = enable; }    // OnOutputDebug �Լ� Ȱ��ȭ, ��Ȱ��ȭ (�⺻��:��Ȱ��)
		void SetOutputSystem(bool enable) { _bOutputSystem = enable; }  // OnOutputSystem �Լ� Ȱ��ȭ, ��Ȱ��ȭ (�⺻��:Ȱ��)

		/* Get ���� ���� */
		int GetPacketPoolSize() { return CPacket::GetPoolSize(); };
		int GetPacketAllocCount() { return CPacket::GetAllocCount(); }
		int GetPacketActualAllocCount() { return CPacket::GetActualAllocCount(); }
		int GetPacketFreeCount() { return CPacket::GetFreeCount(); }


	private:
		/* network */
		HANDLE _hIOCP;

		/* config */
		struct Config
		{
			wchar_t szServerIP[16];
			unsigned short serverPort;
			bool bUseNagle;
			bool bUsePacketEncryption;  // ��Ŷ ��ȣȭ ��� ����.

			BYTE packetCode;
			BYTE packetKey;
			int  maxPacketSize;

			Config();
		};
		Config _config;

		/* thread info */
		struct ThreadInfo
		{
			HANDLE handle;
			unsigned int id;
			ThreadInfo() :handle(NULL), id(0) {}
		};
		ThreadInfo _thWorker;


		/* session */
		class Session
		{
			friend class CLANClient;

			SOCKET socket;
			CLockfreeQueue<CPacket*> sendQ;
			CRingbuffer recvQ;
			OVERLAPPED_EX sendOverlapped;
			OVERLAPPED_EX recvOverlapped;
			CPacket* arrPtrPacket[SIZE_ARR_PACKTE];
			int numSendPacket;

			volatile bool sendIoFlag;
			volatile bool isClosed;

			Session();
		};
		Session _session;


		/* output */
		bool _bOutputDebug;    // OnOutputDebug �Լ� Ȱ��ȭ����(default:false)
		bool _bOutputSystem;   // OnOutputSystem �Լ� Ȱ��ȭ����(default:true)

		
	public:
		/* ����͸� counter */
		class alignas(64) Monitor
		{
			friend class CLANClient;
		private:
			volatile __int64 sendCompletionCount = 0;    // send �Ϸ����� ó��Ƚ��
			volatile __int64 recvCompletionCount = 0;    // recv �Ϸ����� ó��Ƚ��
			volatile __int64 disconnByKnownRecvIoError = 0;    // recv �Ϸ��������� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
			volatile __int64 disconnByUnknownRecvIoError = 0;  // recv �Ϸ��������� ���� �߻�(���κҸ�)
			volatile __int64 disconnByKnownSendIoError = 0;    // send �Ϸ��������� ���� �߻�
			volatile __int64 disconnByUnknownSendIoError = 0;  // send �Ϸ��������� ���� �߻�(���κҸ�)
			volatile __int64 disconnByNormalProcess = 0;        // ������ ���������� ������ ����
			volatile __int64 disconnByPacketCode = 0;    // ��Ŷ�ڵ尡 �߸���
			volatile __int64 disconnByPacketLength = 0;  // ��Ŷ���̰� �߸���
			volatile __int64 disconnByPacketDecode = 0;  // ��Ŷ ���ڵ��� ������

			std::atomic<__int64> sendCount = 0;              // SendPost �Լ����� WSASend �Լ��� ������ ��Ŷ�� ��
			std::atomic<__int64> sendAsyncCount = 0;         // SendPostAsync �Լ����� WSASend �Լ��� ������ ��Ŷ�� ��
			std::atomic<__int64> recvCount = 0;              // WSARecv �Լ� ȣ�� Ƚ��
			std::atomic<__int64> disconnectCount = 0;        // disconnect Ƚ�� (���� release Ƚ��)
			std::atomic<__int64> WSARecvKnownError = 0;      // WSARecv �Լ����� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
			std::atomic<__int64> WSARecvUnknownError = 0;    // WSARecv �Լ����� ���� �߻�(���κҸ�)
			std::atomic<__int64> WSASendKnownError = 0;      // WSASend �Լ����� ���� �߻�
			std::atomic<__int64> WSASendUnknownError = 0;    // WSASend �Լ����� ���� �߻�(���κҸ�)

		public:
			__int64 GetSendCompletionCount() { return sendCompletionCount; }         // send �Ϸ����� ó��Ƚ��
			__int64 GetRecvCompletionCount() { return recvCompletionCount; }         // recv �Ϸ����� ó��Ƚ��
			__int64 GetSendCount() { return sendCount + sendAsyncCount; }           // WSASend �Լ��� ������ ��Ŷ�� ��
			__int64 GetRecvCount() { return recvCount; }                             // WSARecv �Լ� ȣ�� Ƚ��
			__int64 GetDisconnectCount() { return disconnectCount; }                 // disconnect Ƚ�� (���� release Ƚ��)
			__int64 GetDisconnByKnownIoError() { return disconnByKnownRecvIoError + disconnByKnownSendIoError + WSARecvKnownError + WSASendKnownError; }             // WSARecv, WSASend, recv �Ϸ�����, send �Ϸ��������� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
			__int64 GetDisconnByUnknownIoError() { return disconnByUnknownRecvIoError + disconnByUnknownSendIoError + WSARecvUnknownError + WSASendUnknownError; }   // WSARecv, WSASend, recv �Ϸ�����, send �Ϸ��������� ���� �߻�(���κҸ�)
			__int64 GetDisconnByNormalProcess() { return disconnByNormalProcess; }      // Ŭ���̾�Ʈ�� ���������� ������ ����
			__int64 GetDisconnByPacketCode() { return disconnByPacketCode; }            // ��Ŷ�ڵ尡 �߸���
			__int64 GetDisconnByPacketLength() { return disconnByPacketLength; }        // ��Ŷ���̰� �߸���
			__int64 GetDisconnByPacketDecode() { return disconnByPacketDecode; }        // ��Ŷ ���ڵ��� ������
		};
		Monitor _monitor;
	};



}