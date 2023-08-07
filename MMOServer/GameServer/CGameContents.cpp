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
// ������ destination �������� �ű��. ���ǿ� ���� �޽����� ���� �����忡���� ���̻� ó������ �ʴ´�.
void CGameContents::TransferSession(__int64 sessionId, EContentsThread destination, PVOID data)
{
	CContents::TransferSession(sessionId, _pServerAccessor->GetContents(destination), data);
}



/* �÷��̾� */
CPlayer& CGameContents::AllocPlayer() 
{ 
	return _pServerAccessor->AllocPlayer(); 
}

void CGameContents::FreePlayer(CPlayer& player)
{
	_pServerAccessor->FreePlayer(player);
}