#pragma once

#include "../LANClient/CLANClient.h"

class CLANClientMonitoring : public lanlib::CLANClient
{
	using CPacket = lanlib::CPacket;

public:
	CLANClientMonitoring(int serverNo, std::wstring targetIP, unsigned short targetPort, bool bUseNagle, BYTE packetCode, BYTE packetKey, int maxPacketSize, bool bUsePacketEncryption);
	~CLANClientMonitoring();

public:
	bool StartUp();
	void ConnectToServer();
	void Shutdown();

private:
	/* 라이브러리 callback 함수 */
	virtual bool OnRecv(CPacket& packet);
	virtual void OnError(const wchar_t* szError, ...);
	virtual void OnOutputDebug(const wchar_t* szError, ...);
	virtual void OnOutputSystem(const wchar_t* szError, ...);

private:
	int _serverNo;

	std::wstring _sTargetIP;
	unsigned short _targetPort;
	bool _bUseNagle;
	BYTE _packetCode;
	BYTE _packetKey;
	int _maxPacketSize;
	bool _bUsePacketEncryption;
};

