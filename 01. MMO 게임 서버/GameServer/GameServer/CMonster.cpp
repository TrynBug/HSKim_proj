#include <Windows.h>
#include <iostream>
#include <math.h>
#include <vector>
#include "defineGameServer.h"
#include "CPlayer.h"
#include "CMonster.h"

#define M_PI  3.1415926535f
#define dfGET_SQUARE_DISTANCE(posX1, posY1, posX2, posY2)     (((posX1) - (posX2)) * ((posX1) - (posX2)) + ((posY1) - (posY2)) * ((posY1) - (posY2)))
#define dfGET_DISTANCE(posX1, posY1, posX2, posY2)        sqrt(((posX1) - (posX2)) * ((posX1) - (posX2)) + ((posY1) - (posY2)) * ((posY1) - (posY2)))

CMonster::CMonster()
{

}

CMonster::~CMonster()
{
}

void CMonster::Init(EMonsterType type, int area, int tileX, int tileY, int hp, int damage)
{
	CObject::Init();

	_eMonsterType = type;
	_area = area;
	_posX = dfFIELD_TILE_TO_POS(tileX);
	_posY = dfFIELD_TILE_TO_POS(tileY);
	_oldPosX = _posX;
	_oldPosY = _posY;
	_initialPosX = _posX;
	_initialPosY = _posY;
	_tileX = tileX;
	_tileY = tileY;
	_oldTileX = _tileX;
	_oldTileY = _tileY;
	_sectorX = dfFIELD_TILE_TO_SECTOR(tileX);
	_sectorY = dfFIELD_TILE_TO_SECTOR(tileY);
	_oldSectorX = _sectorX;
	_oldSectorY = _sectorY;
	_rotation = 0;
	_maxHp = hp;
	_hp = hp;
	_damage = damage;
	_die = 0;
	_respawn = 0;

	_bAlive = true;
	_bMove = false;
	_bAttack = false;
	_bHasTarget = false;
	_deadTime = 0;
	_respawnWaitTime = 5000;

	if (type == EMonsterType::NORMAL)
	{
		_viewRange = 10.0f;
		_roamingRange = 4.0f;
		_roamingMoveDistance = 1.0f;
		_chasingMoveDistance = 0.5f;
		_maxMoveDistance = 10.0f;
	}
	if (type == EMonsterType::BOSS)
	{
		_viewRange = 10.0f;
		_roamingRange = 6.0f;
		_roamingMoveDistance = 2.0f;
		_chasingMoveDistance = 1.0f;
		_maxMoveDistance = 15.0f;
	}

	_eActionType = EMonsterActionType::ROAMING;
	_nextRoamingWaitTime = 5000;
	_lastMoveTime = GetTickCount64();
	_pTargetPlayer = nullptr;

	_attackRange = 0.6f;
	_attackDelay = 3000;
	_lastAttackTime = 0;

}


void CMonster::SetRespawn(int tileX, int tileY)
{
	Init(_eMonsterType, _area, tileX, tileY, _maxHp, _damage);
	_respawn = 1;
}


void CMonster::Hit(int damage)
{
	_hp -= damage;
	if (_hp <= 0)
	{
		_bAlive = false;
		_die = 1;
		_deadTime = GetTickCount64();
	}
}


void CMonster::SetTargetPlayer(CPlayer* pPlayer)
{
	if (_pTargetPlayer != nullptr)
		return;

	// �÷��̾�� ���� ������ �Ÿ� ���
	float distX = _posX - pPlayer->GetPosX();
	float distY = _posY - pPlayer->GetPosY();

	// �Ÿ��� �þ߹��� ����� �÷��̾ Ÿ������ ����
	if (distX * distX + distY * distY < _viewRange)
	{
		_pTargetPlayer = pPlayer;
		_bHasTarget = true;
		_eActionType = EMonsterActionType::CHASING;
	}
}


// @Override
void CMonster::Update()
{
	if (_bAlive == false)
		return;

	ULONGLONG currentTime = GetTickCount64();
	_bMove = false;
	_bAttack = false;

	// �ι����� ���
	if (_eActionType == EMonsterActionType::ROAMING)
	{
		// ���������� �̵��� �ð����� _nextRoamingWaitTime �и������尡 �����ٸ� ������
		if (_lastMoveTime + _nextRoamingWaitTime < currentTime)
		{
			_bMove = true;  // �̵� �÷��� set
			_lastMoveTime = currentTime;
			_nextRoamingWaitTime = rand() % 2001 + 4000;

			// ���� ��ġ�� �ʱ� ��ġ������ �Ÿ� ���
			float distX = _posX - _initialPosX;
			float distY = _posY - _initialPosY;
			if (distX * distX + distY * distY > _roamingRange * _roamingRange)
			{
				// ���� ���� ��ġ�� �ι� ������ ����ٸ� �ʱ���ġ�� ������
				MoveToInitialPosition();
			}
			else
			{
				// �ι� ������ ����� �ʾҴٸ� ���� ��ġ�� �����δ�.
				MoveRandom();
			}
		}
	}
	// �÷��̾ �Ѵ� ���� ���
	else if (_eActionType == EMonsterActionType::CHASING)
	{
		// �÷��̾ �׾��ٸ� Ÿ���� �����ϰ� �׼� Ÿ���� �������� �ٲ�
		if (_pTargetPlayer->IsAlive() == false)
		{
			_bHasTarget = false;
			_pTargetPlayer = nullptr;
			_eActionType = EMonsterActionType::RETURN;
			_lastMoveTime = currentTime + 2000;
		}
		// ���������� �̵��� �ð����� 1000 �и������尡 �����ٸ� ������
		else if (_lastMoveTime + 1000 < currentTime)
		{
			_bMove = true;  // �̵� �÷��� set
			_lastMoveTime = currentTime;

			// ���� ��ġ�� �ʱ� ��ġ ������ �Ÿ� ���
			float distX = _posX - _initialPosX;
			float distY = _posY - _initialPosY;
			// �ִ� �̵��Ÿ����� �ָ��Դٸ� Ÿ���� �����ϰ� �׼� Ÿ���� �������� ����
			if (distX * distX + distY * distY > _maxMoveDistance * _maxMoveDistance)
			{
				_bHasTarget = false;
				_pTargetPlayer = nullptr;
				_eActionType = EMonsterActionType::RETURN;
				MoveToInitialPosition();
			}
			// �׷��� �ʴٸ� �÷��̾ ����
			else
			{
				MoveChasePlayer();
				
				// �÷��̾���� �Ÿ��� _attackRange ���϶�� �׼� Ÿ���� �������� ����
				if (dfGET_SQUARE_DISTANCE(_posX, _posY, _pTargetPlayer->GetPosX(), _pTargetPlayer->GetPosY()) < _attackRange * _attackRange)
				{
					_eActionType = EMonsterActionType::ATTACK;
				}
			}
		}
	}
	// ó����ġ�� ���ư��� ���� ���
	else if (_eActionType == EMonsterActionType::RETURN)
	{
		// ���������� �̵��� �ð����� 1000 �и������尡 �����ٸ� ������
		if (_lastMoveTime + 1000 < currentTime)
		{
			_bMove = true;  // �̵� �÷��� set
			_lastMoveTime = currentTime;

			// �ʱ� ��ġ�� �̵�
			MoveToInitialPosition();

			// ���� ��ġ�� �ʱ� ��ġ ������ �Ÿ��� 1.0 ���϶�� �׼�Ÿ���� �ι����� ����
			float distX = _posX - _initialPosX;
			float distY = _posY - _initialPosY;
			if (distX * distX + distY * distY < 1.f)
			{
				_eActionType = EMonsterActionType::ROAMING;
			}
		}
	}
	// ���� �Ϸ��� ���
	else if (_eActionType == EMonsterActionType::ATTACK)
	{
		// �÷��̾ �׾��ٸ� Ÿ���� �����ϰ� �׼� Ÿ���� �������� �ٲ�
		if (_pTargetPlayer->IsAlive() == false)
		{
			_bHasTarget = false;
			_pTargetPlayer = nullptr;
			_eActionType = EMonsterActionType::RETURN;
		}
		// ���������� �̵��� �ð����� 1000 �и������尡 ������, ���������� ������ �ð����� _attackDelay �и������尡 �����ٸ� ������ �õ���
		else if (_lastMoveTime + 1000 < currentTime && _lastAttackTime + _attackDelay < currentTime)
		{
			// �÷��̾���� �Ÿ��� _attackRange �̻��̶�� �׼� Ÿ���� �ѱ�� ����
			if (dfGET_SQUARE_DISTANCE(_posX, _posY, _pTargetPlayer->GetPosX(), _pTargetPlayer->GetPosY()) > _attackRange * _attackRange)
			{
				_eActionType = EMonsterActionType::CHASING;
			}
			// ���ݹ��� ����� ����
			else
			{
				_bAttack = true;  // ���� �÷��� set
				_lastAttackTime = currentTime;

				// �ٶ󺸴� ������ �÷��̾������� ����
				float radian_direction = atan2(_pTargetPlayer->GetPosY() - _posY, _pTargetPlayer->GetPosX() - _posX);   // ���� ��ġ�� �������� ���� �� �÷��̾� ��ġ�� ���� ���
				if (radian_direction < 0)
					radian_direction = 2.f * M_PI + radian_direction;
				int direction = (int)(radian_direction * 180.f / M_PI);
				_rotation = (360 - direction + 90) % 360;
			}
		}
	}
}




// ���� ��ġ���� ���� �Ÿ���ŭ �����δ�
void CMonster::MoveRandom()
{
	int direction = rand() % 360;
	float radian_direction = (float)direction * M_PI / 180.f;
	_oldPosX = _posX;
	_oldPosY = _posY;
	_posX = _posX + cos(radian_direction) * _roamingMoveDistance;
	_posY = _posY + sin(radian_direction) * _roamingMoveDistance;
	_rotation = (360 - direction + 90) % 360;

	//printf("MoveRandom, dist:%.3f, init:(%.3f, %.3f), pos:(%.3f, %.3f) to (%.3f, %.3f), direction:%d, rotation:%d\n"
	//	, sqrt((_posX - _initialPosX) * (_posX - _initialPosX) + (_posY - _initialPosY) * (_posY - _initialPosY))
	//	, _initialPosX, _initialPosY, _oldPosX, _oldPosY, _posX, _posY, direction, _rotation);

	// Ÿ�� ��ǥ ������Ʈ
	_posX = min(max(0.f, _posX), dfFIELD_POS_MAX_X - 0.5f);
	_posY = min(max(0.f, _posY), dfFIELD_POS_MAX_Y - 0.5f);
	_oldTileX = _tileX;
	_oldTileY = _tileY;
	_tileX = dfFIELD_POS_TO_TILE(_posX);
	_tileY = dfFIELD_POS_TO_TILE(_posY);
	_oldSectorX = _sectorX;
	_oldSectorY = _sectorY;
	_sectorX = dfFIELD_TILE_TO_SECTOR(_tileX);
	_sectorY = dfFIELD_TILE_TO_SECTOR(_tileY);
}


// ���� ��ġ���� �ʱ� ��ġ�� �����δ�.
void CMonster::MoveToInitialPosition()
{
	float distX = _posX - _initialPosX;
	float distY = _posY - _initialPosY;

	// �ʱ� ��ġ�� �������� ���� ��, ���� ��ġ�� �ʱ� ��ġ ������ ���� ���
	float radian_direction = atan2(distY, distX);
	if (radian_direction < 0)
		radian_direction = 2.f * M_PI + radian_direction;

	// ���� ��ġ���� �ʱ� ��ġ������ �̵�(������ �ݴ�������� �̵�)
	_oldPosX = _posX;
	_oldPosY = _posY;
	_posX = _posX - cos(radian_direction) * _roamingMoveDistance;
	_posY = _posY - sin(radian_direction) * _roamingMoveDistance;
	int direction = ((int)(radian_direction * 180.f / M_PI) + 180) % 360;
	_rotation = (360 - direction + 90) % 360;

	//printf("MoveToInitialPosition, dist:%.3f, init:(%.3f, %.3f), pos:(%.3f, %.3f) to (%.3f, %.3f), direction:%d, rotation:%d\n"
	//	, sqrt((distX* distX) + (distY * distY)), _initialPosX, _initialPosY, _oldPosX, _oldPosY, _posX, _posY, direction, _rotation);


	// Ÿ�� ��ǥ ������Ʈ
	_posX = min(max(0.f, _posX), dfFIELD_POS_MAX_X - 0.5f);
	_posY = min(max(0.f, _posY), dfFIELD_POS_MAX_Y - 0.5f);
	_oldTileX = _tileX;
	_oldTileY = _tileY;
	_tileX = dfFIELD_POS_TO_TILE(_posX);
	_tileY = dfFIELD_POS_TO_TILE(_posY);
	_oldSectorX = _sectorX;
	_oldSectorY = _sectorY;
	_sectorX = dfFIELD_TILE_TO_SECTOR(_tileX);
	_sectorY = dfFIELD_TILE_TO_SECTOR(_tileY);
}



void CMonster::MoveChasePlayer()
{
	// ���� ��ġ�� �������� ���� ��, ���� ��ġ�� �÷��̾� ��ġ ������ ���� ���
	float distX = _pTargetPlayer->GetPosX() - _posX;
	float distY = _pTargetPlayer->GetPosY() - _posY;
	float radian_direction = atan2(distY, distX);
	if (radian_direction < 0)
		radian_direction = 2.f * M_PI + radian_direction;

	// ���� ��ġ���� �÷��̾� ��ġ������ �̵�(���� �������� �̵�)
	_oldPosX = _posX;
	_oldPosY = _posY;
	_posX = _posX + cos(radian_direction) * _chasingMoveDistance;
	_posY = _posY + sin(radian_direction) * _chasingMoveDistance;
	int direction = (int)(radian_direction * 180.f / M_PI);
	_rotation = (360 - direction + 90) % 360;

	//printf("MoveChasePlayer, dist:%.3f, init:(%.3f, %.3f), pos:(%.3f, %.3f) to (%.3f, %.3f), direction:%d, rotation:%d\n"
	//	, distX * distX + distY * distY, _initialPosX, _initialPosY, _oldPosX, _oldPosY, _posX, _posY, direction, _rotation);


	// Ÿ�� ��ǥ ������Ʈ
	_posX = min(max(0.f, _posX), dfFIELD_POS_MAX_X - 0.5f);
	_posY = min(max(0.f, _posY), dfFIELD_POS_MAX_Y - 0.5f);
	_oldTileX = _tileX;
	_oldTileY = _tileY;
	_tileX = dfFIELD_POS_TO_TILE(_posX);
	_tileY = dfFIELD_POS_TO_TILE(_posY);
	_oldSectorX = _sectorX;
	_oldSectorY = _sectorY;
	_sectorX = dfFIELD_TILE_TO_SECTOR(_tileX);
	_sectorY = dfFIELD_TILE_TO_SECTOR(_tileY);
}