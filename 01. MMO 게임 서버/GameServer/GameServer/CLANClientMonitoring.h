#pragma once

#include "CLANClient.h"

class CLANClientMonitoring : public CLANClient
{

private:
	/* ���̺귯�� callback �Լ� */
	virtual bool OnRecv(CPacket& packet);
	virtual void OnError(const wchar_t* szError, ...);
	virtual void OnOutputDebug(const wchar_t* szError, ...);
	virtual void OnOutputSystem(const wchar_t* szError, ...);
};

