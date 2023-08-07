#include <Windows.h>
#include <vector>
#include <locale>
#include <codecvt>
#include <string>
#include "defineGameServer.h"
#include "CPlayer.h"


CPlayer::CPlayer() 
	: _sessionId(0)
	, _accountNo(0)
	, _sessionKey{ 0, }
	, _bLogin(false)
	, _bDisconnect(false)
	, _lastHeartBeatTime(0)
	, _lastDBUpdateTime(0)
{
}

CPlayer::~CPlayer() 
{
}

void CPlayer::Init(__int64 sessionId)
{
	CObject::Init();

	_sessionId = sessionId;
	_accountNo = -1;
	_bLogin = false;
	_bDisconnect = false;
	_lastHeartBeatTime = GetTickCount64();

	_character.state.bNewPlayer = true;
}


void CPlayer::SetAccountInfo(const char* userId, const char* userNick, const wchar_t* ip, unsigned short port)
{
	MultiByteToWideChar(CP_UTF8, 0, userId, -1, _account.szId, sizeof(_account.szId) / sizeof(WCHAR));
	MultiByteToWideChar(CP_UTF8, 0, userNick, -1, _account.szNick, sizeof(_account.szNick) / sizeof(WCHAR));
	_account.sIP = ip;
	_account.port = port;
}


void CPlayer::SetCharacterInfo(BYTE characterType)
{
	_character.characterType = characterType;
}


void CPlayer::LoadCharacterInfo(BYTE characterType, float posX, float posY, int tileX, int tileY, USHORT rotation, int crystal, int hp, INT64 exp, USHORT level, BYTE die)
{
	_character.LoadCharacterInfo(characterType, rotation, crystal, hp, exp, level, die);
	_pos.ResetPosition(posX, posY);
}


void CPlayer::SetLogin(INT64 accountNo, char sessionKey[64])
{
	_accountNo = accountNo;
	memcpy(_sessionKey, sessionKey, sizeof(_sessionKey));
	_bLogin = true;
	_lastHeartBeatTime = GetTickCount64();
}

void CPlayer::SetSit(bool bSit)
{
	_character.SetSit(bSit);
}

void CPlayer::SetRespawn(int tileX, int tileY)
{
	_character.SetRespawn();
	_pos.ResetTile(tileX, tileY);
}


void CPlayer::MovePosition(float posX, float posY, unsigned short rotation, BYTE VKey, BYTE HKey)
{
	_pos.MovePosition(posX, posY);
	_character.rotation = rotation;
	_character.VKey = VKey;
	_character.HKey = HKey;
	_character.state.bMove = true;
}

void CPlayer::AddCrystal(int amount)
{
	_character.AddCrystal(amount);
}

void CPlayer::Hit(int damage)
{
	_character.Hit(damage);
}


// @Override
void CPlayer::Update()
{
	_character.Update();
}


void CPlayer::AddPacket(netlib_game::CPacket& packet)
{
	_vecPacketBuffer.push_back(&packet);
}



void CPlayer::Character::LoadCharacterInfo(BYTE in_characterType, USHORT in_rotation, int in_crystal, int in_hp, INT64 in_exp, USHORT in_level, BYTE in_die)
{
	state.bNewPlayer = false;
	characterType = in_characterType;

	rotation = in_rotation;
	crystal = in_crystal;
	oldCrystal = crystal;
	maxHp = 5000;
	hp = in_hp;
	exp = in_exp;
	level = in_level;
	die = in_die;
	sit = 0;
	respawn = 0;
	VKey = 0;
	HKey = 0;

	if (die == 0)
	{
		state.bAlive = true;
	}
	else
	{
		state.bAlive = false;
	}
	state.bMove = false;
	state.bAttack = false;
	state.bHit = false;
	damageAttack1 = 300;
	damageAttack2 = 500;

	amountSitHPRecovery = 1000;
	sitStartTime = 0;
	lastSitHPRecoveryTime = 0;

	bJustDied = false;
	bJustStandUp = false;
	deadTime = 0;
}

void CPlayer::Character::SetRespawn()
{
	hp = maxHp;
	die = 0;
	state.bAlive = true;
	state.bMove = false;
	state.bAttack = false;
	state.bHit = false;

	bJustDied = false;
	bJustStandUp = false;
	deadTime = 0;
}

void CPlayer::Character::AddCrystal(int amount)
{
	oldCrystal = crystal;
	crystal += amount;
	if (crystal < 0)
		crystal = 0;
}

void CPlayer::Character::Hit(int damage)
{
	hp -= damage;
	hp = max(0, hp);
	state.bHit = true;
}

void CPlayer::Character::SetSit(bool bSit)
{
	if (bSit)
	{
		sit = 1;
		sitStartHp = hp;
		sitStartTime = GetTickCount64();
		lastSitHPRecoveryTime = GetTickCount64();
	}
	else
	{
		sit = 0;
	}
}

void CPlayer::Character::Update()
{
	ULONGLONG currentTime = GetTickCount64();
	bJustDied = false;
	bJustStandUp = false;

	// 사망 처리
	if (state.bAlive == true && hp <= 0)
	{
		state.bAlive = false;
		die = 1;

		bJustDied = true;
		deadTime = currentTime;
	}

	if (state.bAlive == true)
	{
		// 플레이어가 앉아있는데 맞았을 경우
		if (sit == 1 && state.bHit == true)
		{
			sit = 0;
			bJustStandUp = true;
		}
		// 플레이어가 앉기 상태일 경우 1초마다 체력회복
		else if (sit == 1)
		{
			while (currentTime > lastSitHPRecoveryTime + 1000)
			{
				hp += amountSitHPRecovery;
				lastSitHPRecoveryTime += 1000;
			}

			if (hp > maxHp)
			{
				hp = maxHp;
			}
		}
	}

	state.bHit = false;
}



void CPlayer::Position::MovePosition(float posX, float posY)
{
	x_old = x;
	y_old = y;
	x = posX;
	y = posY;
	tileX_old = tileX;
	tileY_old = tileY;
	tileX = dfFIELD_POS_TO_TILE(posX);
	tileY = dfFIELD_POS_TO_TILE(posY);
	sectorX_old = sectorX;
	sectorY_old = sectorY;
	sectorX = dfFIELD_TILE_TO_SECTOR(tileX);
	sectorY = dfFIELD_TILE_TO_SECTOR(tileY);

}

void CPlayer::Position::MoveTile(int in_tileX, int in_tileY)
{
	x_old = x;
	y_old = y;
	x = dfFIELD_TILE_TO_POS(in_tileX);
	y = dfFIELD_TILE_TO_POS(in_tileY);
	tileX_old = tileX;
	tileY_old = tileY;
	tileX = in_tileX;
	tileY = in_tileY;
	sectorX_old = sectorX;
	sectorY_old = sectorY;
	sectorX = dfFIELD_TILE_TO_SECTOR(in_tileX);
	sectorY = dfFIELD_TILE_TO_SECTOR(in_tileY);
}


void CPlayer::Position::ResetPosition(float posX, float posY)
{
	x = posX;
	y = posY;
	x_old = posX;
	y_old = posY;
	tileX = dfFIELD_POS_TO_TILE(posX);
	tileY = dfFIELD_POS_TO_TILE(posY);
	tileX_old = dfFIELD_POS_TO_TILE(posX);
	tileY_old = dfFIELD_POS_TO_TILE(posY);
	sectorX = dfFIELD_TILE_TO_SECTOR(tileX);
	sectorY = dfFIELD_TILE_TO_SECTOR(tileY);
	sectorX_old = dfFIELD_TILE_TO_SECTOR(tileX);
	sectorY_old = dfFIELD_TILE_TO_SECTOR(tileY);
}

void CPlayer::Position::ResetTile(int in_tileX, int in_tileY)
{
	x = dfFIELD_TILE_TO_POS(in_tileX);
	y = dfFIELD_TILE_TO_POS(in_tileY);
	x_old = dfFIELD_TILE_TO_POS(in_tileX);
	y_old = dfFIELD_TILE_TO_POS(in_tileY);
	tileX = in_tileX;
	tileY = in_tileY;
	tileX_old = in_tileX;
	tileY_old = in_tileY;
	sectorX = dfFIELD_TILE_TO_SECTOR(in_tileX);
	sectorY = dfFIELD_TILE_TO_SECTOR(in_tileY);
	sectorX_old = dfFIELD_TILE_TO_SECTOR(in_tileX);
	sectorY_old = dfFIELD_TILE_TO_SECTOR(in_tileY);

}

