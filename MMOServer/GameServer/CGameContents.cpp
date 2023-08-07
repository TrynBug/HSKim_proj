#include "CGameServer.h"
#include "CGameContents.h"


CGameContents::CGameContents(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS)
	: CContents(FPS)
	, _eMyContentsType(myContentsType)
	, _pServerAccessor(std::move(pAccessor))
	, _FPS(FPS)
{
}

CGameContents::CGameContents(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS, DWORD sendMode)
	: CContents(FPS, sendMode)
	, _eMyContentsType(myContentsType)
	, _pServerAccessor(std::move(pAccessor))
	, _FPS(FPS)
{
}

CGameContents::~CGameContents()
{

}





/* session */
// 세션을 destination 컨텐츠로 옮긴다. 세션에 대한 메시지는 현재 스레드에서는 더이상 처리되지 않는다.
void CGameContents::TransferSession(__int64 sessionId, EContentsThread destination, PVOID data)
{
	CContents::TransferSession(sessionId, _pServerAccessor->GetContents(destination), data);
}



/* 플레이어 */
CPlayer& CGameContents::AllocPlayer() 
{ 
	return _pServerAccessor->AllocPlayer(); 
}

void CGameContents::FreePlayer(CPlayer& player)
{
	_pServerAccessor->FreePlayer(player);
}