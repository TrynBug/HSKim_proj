#include "CGameServer.h"
#include "CGameContents.h"


CGameContents::CGameContents(EContentsThread myContentsType, CGameServer* pGameServer, int FPS)
	: CContents(FPS)
	, _eMyContentsType(myContentsType), _pGameServer(pGameServer), _FPS(FPS)
{
}

CGameContents::CGameContents(EContentsThread myContentsType, CGameServer* pGameServer, int FPS, DWORD sendMode)
	: CContents(FPS, sendMode)
	, _eMyContentsType(myContentsType), _pGameServer(pGameServer), _FPS(FPS)
{
}

CGameContents::~CGameContents()
{

}





/* session */
// ������ destination �������� �ű��. ���ǿ� ���� �޽����� ���� �����忡���� ���̻� ó������ �ʴ´�.
void CGameContents::TransferSession(__int64 sessionId, EContentsThread destination, PVOID data)
{
	CContents::TransferSession(sessionId, _pGameServer->GetContentsPtr(destination), data);
}



/* �÷��̾� */
void CGameContents::FreePlayer(CPlayer* pPlayer)
{
	for (int i = 0; i < pPlayer->_vecPacketBuffer.size(); i++)
	{
		pPlayer->_vecPacketBuffer[i]->SubUseCount();
	}
	pPlayer->_vecPacketBuffer.clear();
	_pGameServer->_poolPlayer.Free(pPlayer);
}