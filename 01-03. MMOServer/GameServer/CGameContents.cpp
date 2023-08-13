#include "stdafx.h"
#include "CGameServer.h"
#include "CGameContents.h"

#include "CObject.h"
#include "CPlayer.h"

using namespace gameserver;

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
// player�� destination �������� �ű��.�ش� ���ǿ� ���� �޽����� ���� �����忡���� ���̻� ó������ �ʴ´�.
void CGameContents::TransferSession(__int64 sessionId, EContentsThread destination, CPlayer_t pPlayer)
{
	_TransferPlayerData* pData = _pServerAccessor->AllocTransferPlayerData(std::move(pPlayer));
	CContents::TransferSession(sessionId, _pServerAccessor->GetContents(destination), reinterpret_cast<PVOID*>(pData));
}



/* �÷��̾� */
CPlayer_t CGameContents::GetPlayerBySessionId(__int64 sessionId)
{
	const auto& iter = _mapPlayer.find(sessionId);
	if (iter == _mapPlayer.end())
		return nullptr;
	else
		return iter->second;
}

void CGameContents::InsertPlayerToMap(__int64 sessionId, CPlayer_t pPlayer)
{
	const auto& iter = _mapPlayer.find(sessionId);
	if (iter != _mapPlayer.end())
	{
		OnError(L"CGameContents::InsertPlayerToMap player already exists in the map!! current session:%lld, current accountNo:%lld, new session:%lld, new account:%lld"
			, iter->second->GetSessionId(), iter->second->GetAccountNo(), sessionId, pPlayer->GetAccountNo());
	}
	_mapPlayer.insert(std::make_pair(sessionId, std::move(pPlayer)));
}

void CGameContents::ErasePlayerFromMap(__int64 sessionId)
{
	_mapPlayer.erase(sessionId);
}

CPlayer_t CGameContents::AllocPlayer()
{ 
	return _pServerAccessor->AllocPlayer(); 
}




/* ���̺귯�� callback �Լ� */
void CGameContents::OnSessionJoin(__int64 sessionId, PVOID data)
{
	if (data == nullptr)
	{
		OnInitialSessionJoin(sessionId);
	}
	else
	{
		_TransferPlayerData* pData = reinterpret_cast<_TransferPlayerData*>(data);
		OnPlayerJoin(sessionId, std::move(pData->pPlayer));
		_pServerAccessor->FreeTransferPlayerData(pData);
	}
}