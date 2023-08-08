#pragma once

#include <iostream>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <Windows.h>
#include <process.h>
#include <vector>
#include <unordered_map>

#include "CPacket.h"
#include "CRingbuffer.h"
#include "CLockfreeQueue.h"

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

/* Network */
class alignas(64) CLANClient
{
private:
	/* network */
	HANDLE _hIOCP;
	SOCKET _socket;

	/* config */
	wchar_t _szServerIP[16];
	unsigned short _serverPort;
	bool _bUseNagle;
	bool _bUsePacketEncryption;  // ��Ŷ ��ȣȭ ��� ����.

	BYTE _packetCode;
	BYTE _packetKey;
	int  _maxPacketSize;

	/* thread */
	HANDLE _hThreadWorker;
	unsigned int _idThreadWorker;

	/* client */
	CLockfreeQueue<CPacket*> _sendQ;
	CRingbuffer _recvQ;
	LAN_OVERLAPPED_EX _sendOverlapped;
	LAN_OVERLAPPED_EX _recvOverlapped;
	CPacket* _arrPtrPacket[LAN_CLIENT_SIZE_ARR_PACKTE];
	int _numSendPacket;

	volatile bool _sendIoFlag;
	volatile bool _isClosed;

	/* output */
	bool _bOutputDebug;    // OnOutputDebug �Լ� Ȱ��ȭ����(default:false)
	bool _bOutputSystem;   // OnOutputSystem �Լ� Ȱ��ȭ����(default:true)

	/* ����͸� */
	volatile __int64 _sendCompletionCount = 0;    // send �Ϸ����� ó��Ƚ��
	volatile __int64 _recvCompletionCount = 0;    // recv �Ϸ����� ó��Ƚ��
	volatile __int64 _disconnByKnownRecvIoError = 0;    // recv �Ϸ��������� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
	volatile __int64 _disconnByUnknownRecvIoError = 0;  // recv �Ϸ��������� ���� �߻�(���κҸ�)
	volatile __int64 _disconnByKnownSendIoError = 0;    // send �Ϸ��������� ���� �߻�
	volatile __int64 _disconnByUnknownSendIoError = 0;  // send �Ϸ��������� ���� �߻�(���κҸ�)
	volatile __int64 _disconnByNormalProcess = 0;        // ������ ���������� ������ ����
	volatile __int64 _disconnByPacketCode = 0;    // ��Ŷ�ڵ尡 �߸���
	volatile __int64 _disconnByPacketLength = 0;  // ��Ŷ���̰� �߸���
	volatile __int64 _disconnByPacketDecode = 0;  // ��Ŷ ���ڵ��� ������

	volatile __int64 _sendCount = 0;              // SendPost �Լ����� WSASend �Լ��� ������ ��Ŷ�� ��
	volatile __int64 _sendAsyncCount = 0;         // SendPostAsync �Լ����� WSASend �Լ��� ������ ��Ŷ�� ��
	volatile __int64 _recvCount = 0;              // WSARecv �Լ� ȣ�� Ƚ��
	volatile __int64 _disconnectCount = 0;        // disconnect Ƚ�� (���� release Ƚ��)
	volatile __int64 _WSARecvKnownError = 0;      // WSARecv �Լ����� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
	volatile __int64 _WSARecvUnknownError = 0;    // WSARecv �Լ����� ���� �߻�(���κҸ�)
	volatile __int64 _WSASendKnownError = 0;      // WSASend �Լ����� ���� �߻�
	volatile __int64 _WSASendUnknownError = 0;    // WSASend �Լ����� ���� �߻�(���κҸ�)


public:
	CLANClient();
	virtual ~CLANClient();

public:
	/* Set */
	void SetOutputDebug(bool enable) { _bOutputDebug = enable; }    // OnOutputDebug �Լ� Ȱ��ȭ, ��Ȱ��ȭ (�⺻��:��Ȱ��)
	void SetOutputSystem(bool enable) { _bOutputSystem = enable; }  // OnOutputSystem �Լ� Ȱ��ȭ, ��Ȱ��ȭ (�⺻��:Ȱ��)

	/* Get ���� ���� */
	int GetPacketPoolSize() { return CPacket::GetPoolSize(); };
	int GetPacketAllocCount() { return CPacket::GetAllocCount(); }
	int GetPacketActualAllocCount() { return CPacket::GetActualAllocCount(); }
	int GetPacketFreeCount() { return CPacket::GetFreeCount(); }

	/* Get ����͸� */
	__int64 GetSendCompletionCount() { return _sendCompletionCount; }         // send �Ϸ����� ó��Ƚ��
	__int64 GetRecvCompletionCount() { return _recvCompletionCount; }         // recv �Ϸ����� ó��Ƚ��
	__int64 GetSendCount() { return _sendCount + _sendAsyncCount; }           // WSASend �Լ��� ������ ��Ŷ�� ��
	__int64 GetRecvCount() { return _recvCount; }                             // WSARecv �Լ� ȣ�� Ƚ��
	__int64 GetDisconnectCount() { return _disconnectCount; }                 // disconnect Ƚ�� (���� release Ƚ��)
	__int64 GetDisconnByKnownIoError() { return _disconnByKnownRecvIoError + _disconnByKnownSendIoError + _WSARecvKnownError + _WSASendKnownError; }             // WSARecv, WSASend, recv �Ϸ�����, send �Ϸ��������� ���� �߻�(Ŭ���̾�Ʈ �Ǵ� �������� ������ ��������)
	__int64 GetDisconnByUnknownIoError() { return _disconnByUnknownRecvIoError + _disconnByUnknownSendIoError + _WSARecvUnknownError + _WSASendUnknownError; }   // WSARecv, WSASend, recv �Ϸ�����, send �Ϸ��������� ���� �߻�(���κҸ�)
	__int64 GetDisconnByNormalProcess() { return _disconnByNormalProcess; }      // Ŭ���̾�Ʈ�� ���������� ������ ����
	__int64 GetDisconnByPacketCode() { return _disconnByPacketCode; }            // ��Ŷ�ڵ尡 �߸���
	__int64 GetDisconnByPacketLength() { return _disconnByPacketLength; }        // ��Ŷ���̰� �߸���
	__int64 GetDisconnByPacketDecode() { return _disconnByPacketDecode; }        // ��Ŷ ���ڵ��� ������

	/* client */
	bool StartUp(const wchar_t* serverIP, unsigned short serverPort, bool bUseNagle, BYTE packetCode, BYTE packetKey, int maxPacketSize, bool bUsePacketEncryption);
	void Shutdown();    // ��Ʈ��ũ ����
	bool Disconnect();

	/* packet */
	CPacket* AllocPacket();
	bool SendPacket(CPacket* pPacket);  // send�� ����� ����� �� ���� ���� ����� ����ǰ�, �׷��� ���� ��� �񵿱�� ����ȴ�.
	bool SendPacketAsync(CPacket* pPacket);  // send�� �ݵ�� �񵿱�� ����ȴ�.
	bool PostInnerRequest(CPacket* pPacket);   // IOCP �Ϸ����� ť�� �۾��� �����Ѵ�. �۾��� ȹ��Ǹ� OnInnerRequest �Լ��� ȣ��ȴ�.

	/* dynamic alloc */
	// 64byte aligned ��ü ������ ���� new, delete overriding
	void* operator new(size_t size);
	void operator delete(void* p);

private:
	/* Send, Recv */
	void RecvPost();   // WSARecv�� �Ѵ�.	
	void SendPost();   // WSASend�� �õ��Ѵ�. 
	void SendPostAsync();  // SendPacketAsync �Լ��� ���� �񵿱� send ��û�� �޾��� �� worker ������ ������ ȣ��ȴ�. WSASend�� ȣ���Ѵ�.

	/* ��Ŷ ��ȣȭ */
	void EncryptPacket(CPacket* pPacket);
	bool DecipherPacket(CPacket* pPacket);

	/* thread */
	static unsigned WINAPI ThreadWorker(PVOID pParam);
	void UpdateWorkerThread();

	/* Crash */
	void Crash();

private:
	/* ���̺귯�� callback �Լ� */
	virtual bool OnRecv(CPacket& packet) = 0;
	virtual bool OnInnerRequest(CPacket& packet) { return false; }
	virtual void OnError(const wchar_t* szError, ...) = 0;            // ���� �߻�
	virtual void OnOutputDebug(const wchar_t* szError, ...) = 0;      // debug ���ڿ��� �Լ��� ����. �� �Լ��� ���� ȣ���������� NET_OUTPUT_DEBUG ��ũ�� ����� ������.
	virtual void OnOutputSystem(const wchar_t* szError, ...) = 0;     // system ���ڿ��� �Լ��� ����. �� �Լ��� ���� ȣ���������� NET_OUTPUT_SYSTEM ��ũ�� ����� ������.
};



