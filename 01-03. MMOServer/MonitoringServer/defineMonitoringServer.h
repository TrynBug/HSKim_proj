#pragma once

namespace netlib
{
	class CPacket;
}

namespace monitorserver
{
	class CUser;

	using CPacket = netlib::CPacket;
	using CUser_t = std::shared_ptr<CUser>;
	

	enum class EMonitoringServerJob
	{
		SHUTDOWN,
		CHECK_LOGIN_TIMEOUT
	};

}