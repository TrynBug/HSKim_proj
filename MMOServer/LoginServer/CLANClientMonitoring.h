#pragma once

#include "../LANClient/CLANClient.h"

class CLANClientMonitoring : public lanlib::CLANClient
{
	using CPacket = lanlib::CPacket;

private:
	/* 라이브러리 callback 함수 */
	virtual bool OnRecv(CPacket& packet);
	virtual void OnError(const wchar_t* szError, ...);
	virtual void OnOutputDebug(const wchar_t* szError, ...);
	virtual void OnOutputSystem(const wchar_t* szError, ...);
};

