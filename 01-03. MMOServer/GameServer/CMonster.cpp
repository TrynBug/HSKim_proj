#include "stdafx.h"
#include <math.h>

#include "CObject.h"
#include "CPlayer.h"
#include "CMonster.h"

using namespace gameserver;

#define M_PI  3.1415926535f
#define dfGET_SQUARE_DISTANCE(posX1, posY1, posX2, posY2)     (((posX1) - (posX2)) * ((posX1) - (posX2)) + ((posY1) - (posY2)) * ((posY1) - (posY2)))
#define dfGET_DISTANCE(posX1, posY1, posX2, posY2)        sqrt(((posX1) - (posX2)) * ((posX1) - (posX2)) + ((posY1) - (posY2)) * ((posY1) - (posY2)))

CMonster::CMonster()
	: _status()
	, _action()
	, _pos()
{
}

CMonster::~CMonster()
{
}




void CMonster::Init(EMonsterType type, int area, int tileX, int tileY, int hp, int damage)
{
	CObject::Init();

	_status.Initialization(type, area, hp, damage);
	_action.Init();
	_pos.ResetTile(tileX, tileY);

}


void CMonster::SetRespawn(int tileX, int tileY)
{
	_status.SetRespawn();
	_action.Init();
	_pos.ResetTile(tileX, tileY);
}


void CMonster::Hit(int damage)
{
	_status.Hit(damage);
}


void CMonster::SetTargetPlayer(CPlayer_t& pPlayer)
{
	// ���� Ÿ���� �����Ѵٸ� �ٸ��÷��̾ Ÿ������ �������� ����
	if (_action.bHasTarget == true)
		return;

	// �÷��̾�� ���� ������ �Ÿ� ���
	float distX = _pos.x - pPlayer->GetPosX();
	float distY = _pos.y - pPlayer->GetPosY();

	// �Ÿ��� �þ߹��� ����� �÷��̾ Ÿ������ ����
	if (distX * distX + distY * distY < _status.viewRange)
	{
		_action.wpTargetPlayer = pPlayer;
		_action.bHasTarget = true;
		_action.eActionType = EMonsterActionType::CHASING;
	}
}



// @Override
void CMonster::Update()
{
	if (_status.bAlive == false)
		return;

	ULONGLONG currentTime = GetTickCount64();
	_status.bMove = false;
	_status.bAttack = false;

	// �ι����� ���
	switch (_action.eActionType)
	{
	case EMonsterActionType::ROAMING:
	{
		// ���������� �̵��� �ð����� _nextRoamingWaitTime �и������尡 �����ٸ� ������
		if (_action.lastMoveTime + _action.nextRoamingWaitTime < currentTime)
		{
			_status.bMove = true;  // �̵� �÷��� set
			_action.lastMoveTime = currentTime;
			_action.nextRoamingWaitTime = rand() % 2001 + 4000;

			// ���� ��ġ�� �ʱ� ��ġ������ �Ÿ� ���
			float distX = _pos.x - _pos.x_initial;
			float distY = _pos.y - _pos.y_initial;
			if (distX * distX + distY * distY > _status.roamingRange * _status.roamingRange)
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
		break;
	}
	// �÷��̾ �Ѵ� ���� ���
	case EMonsterActionType::CHASING:
	{
		// �÷��̾� ��ü�� ������ Ÿ���� �����Ѵ�.
		CPlayer_t spTarget = _action.wpTargetPlayer.lock();
		if (spTarget == nullptr)
		{
			_action.bHasTarget = false;
			_action.wpTargetPlayer.reset();
			_action.eActionType = EMonsterActionType::RETURN;
			_action.lastMoveTime = currentTime + 2000;
		}
		// �÷��̾ �׾��ٸ� Ÿ���� �����ϰ� �׼� Ÿ���� �������� �ٲ�
		else if (spTarget->IsAlive() == false)
		{
			_action.bHasTarget = false;
			_action.wpTargetPlayer.reset();
			_action.eActionType = EMonsterActionType::RETURN;
			_action.lastMoveTime = currentTime + 2000;
		}
		// ���������� �̵��� �ð����� 1000 �и������尡 �����ٸ� ������
		else if (_action.lastMoveTime + 1000 < currentTime)
		{
			_status.bMove = true;  // �̵� �÷��� set
			_action.lastMoveTime = currentTime;

			// ���� ��ġ�� �ʱ� ��ġ ������ �Ÿ� ���
			float distX = _pos.x - _pos.x_initial;
			float distY = _pos.y - _pos.y_initial;
			// �ִ� �̵��Ÿ����� �ָ��Դٸ� Ÿ���� �����ϰ� �׼� Ÿ���� �������� ����
			if (distX * distX + distY * distY > _status.maxMoveDistance * _status.maxMoveDistance)
			{
				_action.bHasTarget = false;
				_action.wpTargetPlayer.reset();
				_action.eActionType = EMonsterActionType::RETURN;
				MoveToInitialPosition();
			}
			// �׷��� �ʴٸ� �÷��̾ ����
			else
			{
				MoveChasePlayer(spTarget);

				// �÷��̾���� �Ÿ��� _attackRange ���϶�� �׼� Ÿ���� �������� ����
				if (dfGET_SQUARE_DISTANCE(_pos.x, _pos.y, spTarget->GetPosX(), spTarget->GetPosY()) < _status.attackRange * _status.attackRange)
				{
					_action.eActionType = EMonsterActionType::ATTACK;
				}
			}
		}
		break;
	}
	// ó����ġ�� ���ư��� ���� ���
	case EMonsterActionType::RETURN:
	{
		// ���������� �̵��� �ð����� 1000 �и������尡 �����ٸ� ������
		if (_action.lastMoveTime + 1000 < currentTime)
		{
			_status.bMove = true;  // �̵� �÷��� set
			_action.lastMoveTime = currentTime;

			// �ʱ� ��ġ�� �̵�
			MoveToInitialPosition();

			// ���� ��ġ�� �ʱ� ��ġ ������ �Ÿ��� 1.0 ���϶�� �׼�Ÿ���� �ι����� ����
			float distX = _pos.x - _pos.x_initial;
			float distY = _pos.y - _pos.y_initial;
			if (distX * distX + distY * distY < 1.f)
			{
				_action.eActionType = EMonsterActionType::ROAMING;
			}
		}
		break;
	}
	// ���� �Ϸ��� ���
	case EMonsterActionType::ATTACK:
	{
		// �÷��̾� ��ü�� ������ Ÿ���� �����Ѵ�.
		CPlayer_t spTarget = _action.wpTargetPlayer.lock();
		if (spTarget == nullptr)
		{
			_action.bHasTarget = false;
			_action.wpTargetPlayer.reset();
			_action.eActionType = EMonsterActionType::RETURN;
		}
		// �÷��̾ �׾��ٸ� Ÿ���� �����ϰ� �׼� Ÿ���� �������� �ٲ�
		else if (spTarget->IsAlive() == false)
		{
			_action.bHasTarget = false;
			_action.wpTargetPlayer.reset();
			_action.eActionType = EMonsterActionType::RETURN;
		}
		// ���������� �̵��� �ð����� 1000 �и������尡 ������, ���������� ������ �ð����� _attackDelay �и������尡 �����ٸ� ������ �õ���
		else if (_action.lastMoveTime + 1000 < currentTime && _action.lastAttackTime + _status.attackDelay < currentTime)
		{
			// �÷��̾���� �Ÿ��� _attackRange �̻��̶�� �׼� Ÿ���� �ѱ�� ����
			if (dfGET_SQUARE_DISTANCE(_pos.x, _pos.y, spTarget->GetPosX(), spTarget->GetPosY()) > _status.attackRange * _status.attackRange)
			{
				_action.eActionType = EMonsterActionType::CHASING;
			}
			// ���ݹ��� ����� ����
			else
			{
				_status.bAttack = true;  // ���� �÷��� set
				_action.lastAttackTime = currentTime;

				// �ٶ󺸴� ������ �÷��̾������� ����
				float radian_direction = atan2(spTarget->GetPosY() - _pos.y, spTarget->GetPosX() - _pos.x);   // ���� ��ġ�� �������� ���� �� �÷��̾� ��ġ�� ���� ���
				if (radian_direction < 0)
					radian_direction = 2.f * M_PI + radian_direction;
				int direction = (int)(radian_direction * 180.f / M_PI);
				_status.rotation = (360 - direction + 90) % 360;
			}
		}
		break;
	}
	};
}




// ���� ��ġ���� ���� �Ÿ���ŭ �����δ�
void CMonster::MoveRandom()
{
	int direction = rand() % 360;
	float radian_direction = (float)direction * M_PI / 180.f;

	// ���� ��ġ ���
	float x_new = _pos.x + cos(radian_direction) * _status.roamingMoveDistance;
	float y_new = _pos.y + sin(radian_direction) * _status.roamingMoveDistance;
	x_new = min(max(0.f, x_new), dfFIELD_POS_MAX_X - 0.5f);
	y_new = min(max(0.f, y_new), dfFIELD_POS_MAX_Y - 0.5f);

	// ��ǥ ������Ʈ
	_pos.MovePosition(x_new, y_new);
	_status.rotation = (360 - direction + 90) % 360;
}


// ���� ��ġ���� �ʱ� ��ġ�� �����δ�.
void CMonster::MoveToInitialPosition()
{
	float distX = _pos.x - _pos.x_initial;
	float distY = _pos.y - _pos.y_initial;

	// �ʱ� ��ġ�� �������� ���� ��, ���� ��ġ�� �ʱ� ��ġ ������ ���� ���
	float radian_direction = atan2(distY, distX);
	if (radian_direction < 0)
		radian_direction = 2.f * M_PI + radian_direction;

	// ���� ��ġ���� �ʱ� ��ġ������ �̵�(������ �ݴ�������� �̵�)
	float x_new = _pos.x - cos(radian_direction) * _status.roamingMoveDistance;
	float y_new = _pos.y - sin(radian_direction) * _status.roamingMoveDistance;
	x_new = min(max(0.f, x_new), dfFIELD_POS_MAX_X - 0.5f);
	y_new = min(max(0.f, y_new), dfFIELD_POS_MAX_Y - 0.5f);

	_pos.MovePosition(x_new, y_new);

	int direction = ((int)(radian_direction * 180.f / M_PI) + 180) % 360;
	_status.rotation = (360 - direction + 90) % 360;
}



void CMonster::MoveChasePlayer(CPlayer_t& pTarget)
{
	if (pTarget == nullptr)
		return;

	// ���� ��ġ�� �������� ���� ��, ���� ��ġ�� �÷��̾� ��ġ ������ ���� ���
	float distX = pTarget->GetPosX() - _pos.x;
	float distY = pTarget->GetPosY() - _pos.y;
	float radian_direction = atan2(distY, distX);
	if (radian_direction < 0)
		radian_direction = 2.f * M_PI + radian_direction;

	// ���� ��ġ���� �÷��̾� ��ġ������ �̵�(���� �������� �̵�)
	float x_new = _pos.x + cos(radian_direction) * _status.chasingMoveDistance;
	float y_new = _pos.y + sin(radian_direction) * _status.chasingMoveDistance;
	x_new = min(max(0.f, x_new), dfFIELD_POS_MAX_X - 0.5f);
	y_new = min(max(0.f, y_new), dfFIELD_POS_MAX_Y - 0.5f);
	_pos.MovePosition(x_new, y_new);
	
	int direction = (int)(radian_direction * 180.f / M_PI);
	_status.rotation = (360 - direction + 90) % 360;
}



void CMonster::Status::Initialization(EMonsterType in_type, int in_area, int in_hp, int in_damage)
{
	eMonsterType = in_type;
	area = in_area;

	rotation = 0;
	maxHp = in_hp;
	hp = in_hp;
	damage = in_damage;
	die = 0;
	respawn = 0;

	bAlive = true;
	bMove = false;
	bAttack = false;
	
	deadTime = 0;
	respawnWaitTime = 5000;

	if (eMonsterType == EMonsterType::NORMAL)
	{
		viewRange = 10.0f;
		roamingRange = 4.0f;
		roamingMoveDistance = 1.0f;
		chasingMoveDistance = 0.5f;
		maxMoveDistance = 10.0f;
	}
	else if (eMonsterType == EMonsterType::BOSS)
	{
		viewRange = 10.0f;
		roamingRange = 6.0f;
		roamingMoveDistance = 2.0f;
		chasingMoveDistance = 1.0f;
		maxMoveDistance = 15.0f;
	}

	attackRange = 0.6f;
	attackDelay = 3000;
}


void CMonster::Status::SetRespawn()
{
	Initialization(eMonsterType, area, maxHp, damage);
	respawn = 1;
}

void CMonster::Status::Hit(int damage)
{
	hp -= damage;
	if (hp <= 0)
	{
		bAlive = false;
		die = 1;
		deadTime = GetTickCount64();
	}
}


void CMonster::Action::Init()
{
	bHasTarget = false;
	eActionType = EMonsterActionType::ROAMING;
	nextRoamingWaitTime = 5000;
	lastMoveTime = GetTickCount64();
	wpTargetPlayer.reset();
	lastAttackTime = 0;
}

void CMonster::Position::MovePosition(float posX, float posY)
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

void CMonster::Position::MoveTile(int in_tileX, int in_tileY)
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
	sectorX = dfFIELD_TILE_TO_SECTOR(tileX);
	sectorY = dfFIELD_TILE_TO_SECTOR(tileY);
}

void CMonster::Position::ResetTile(int in_tileX, int in_tileY)
{
	x = dfFIELD_TILE_TO_POS(in_tileX);
	y = dfFIELD_TILE_TO_POS(in_tileY);
	x_old = dfFIELD_TILE_TO_POS(in_tileX);
	y_old = dfFIELD_TILE_TO_POS(in_tileY);
	x_initial = dfFIELD_TILE_TO_POS(in_tileX);
	y_initial = dfFIELD_TILE_TO_POS(in_tileY);
	tileX = in_tileX;
	tileY = in_tileY;
	tileX_old = in_tileX;
	tileY_old = in_tileY;
	sectorX = dfFIELD_TILE_TO_SECTOR(tileX);
	sectorY = dfFIELD_TILE_TO_SECTOR(tileY);
	sectorX_old = dfFIELD_TILE_TO_SECTOR(tileX);
	sectorY_old = dfFIELD_TILE_TO_SECTOR(tileY);
	
}