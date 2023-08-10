#include "CLANClientMonitoring.h"

#include "../common/CommonProtocol.h"
#include "../utils/logger.h"




CLANClientMonitoring::CLANClientMonitoring(int serverNo, std::wstring targetIP, unsigned short targetPort, bool bUseNagle, BYTE packetCode, BYTE packetKey, int maxPacketSize, bool bUsePacketEncryption)
	: _serverNo(serverNo)
	, _sTargetIP(targetIP)
	, _targetPort(targetPort)
	, _bUseNagle(bUseNagle)
	, _packetCode(packetCode)
	, _packetKey(packetKey)
	, _maxPacketSize(maxPacketSize)
	, _bUsePacketEncryption(bUsePacketEncryption)
{
}


CLANClientMonitoring::~CLANClientMonitoring()
{
	Shutdown();
}




bool CLANClientMonitoring::StartUp()
{
	return CLANClient::StartUp(_sTargetIP.c_str(), _targetPort, _bUseNagle, _packetCode, _packetKey, _maxPacketSize, _bUsePacketEncryption);
}

void CLANClientMonitoring::ConnectToServer()
{
	lanlib::CPacket& packet = AllocPacket();
	packet << (WORD)en_PACKET_SS_MONITOR_LOGIN << _serverNo;
	SendPacket(packet);
	packet.SubUseCount();
}

void CLANClientMonitoring::Shutdown()
{
	CLANClient::Shutdown();
}



/* 라이브러리 callback 함수 */
bool CLANClientMonitoring::OnRecv(CPacket& packet)
{
	return true;
}

void CLANClientMonitoring::OnError(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_ERROR, szError, vaList);
	va_end(vaList);
}

void CLANClientMonitoring::OnOutputDebug(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_DEBUG, szError, vaList);
	va_end(vaList);
}

void CLANClientMonitoring::OnOutputSystem(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_INFO, szError, vaList);
	va_end(vaList);
}




