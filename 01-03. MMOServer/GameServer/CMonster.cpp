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
	// 현재 타겟이 존재한다면 다른플레이어를 타겟으로 지정하지 않음
	if (_action.bHasTarget == true)
		return;

	// 플레이어와 몬스터 사이의 거리 계산
	float distX = _pos.x - pPlayer->GetPosX();
	float distY = _pos.y - pPlayer->GetPosY();

	// 거리가 시야범위 내라면 플레이어를 타겟으로 지정
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

	// 로밍중일 경우
	switch (_action.eActionType)
	{
	case EMonsterActionType::ROAMING:
	{
		// 마지막으로 이동한 시간에서 _nextRoamingWaitTime 밀리세컨드가 지났다면 움직임
		if (_action.lastMoveTime + _action.nextRoamingWaitTime < currentTime)
		{
			_status.bMove = true;  // 이동 플래그 set
			_action.lastMoveTime = currentTime;
			_action.nextRoamingWaitTime = rand() % 2001 + 4000;

			// 현재 위치와 초기 위치사이의 거리 계산
			float distX = _pos.x - _pos.x_initial;
			float distY = _pos.y - _pos.y_initial;
			if (distX * distX + distY * distY > _status.roamingRange * _status.roamingRange)
			{
				// 만약 현재 위치가 로밍 범위를 벗어났다면 초기위치로 움직임
				MoveToInitialPosition();
			}
			else
			{
				// 로밍 범위를 벗어나지 않았다면 랜덤 위치로 움직인다.
				MoveRandom();
			}
		}
		break;
	}
	// 플레이어를 쫓는 중일 경우
	case EMonsterActionType::CHASING:
	{
		// 플레이어 객체가 없으면 타겟을 해제한다.
		CPlayer_t spTarget = _action.wpTargetPlayer.lock();
		if (spTarget == nullptr)
		{
			_action.bHasTarget = false;
			_action.wpTargetPlayer.reset();
			_action.eActionType = EMonsterActionType::RETURN;
			_action.lastMoveTime = currentTime + 2000;
		}
		// 플레이어가 죽었다면 타겟을 해제하고 액션 타입을 리턴으로 바꿈
		else if (spTarget->IsAlive() == false)
		{
			_action.bHasTarget = false;
			_action.wpTargetPlayer.reset();
			_action.eActionType = EMonsterActionType::RETURN;
			_action.lastMoveTime = currentTime + 2000;
		}
		// 마지막으로 이동한 시간에서 1000 밀리세컨드가 지났다면 움직임
		else if (_action.lastMoveTime + 1000 < currentTime)
		{
			_status.bMove = true;  // 이동 플래그 set
			_action.lastMoveTime = currentTime;

			// 현재 위치와 초기 위치 사이의 거리 계산
			float distX = _pos.x - _pos.x_initial;
			float distY = _pos.y - _pos.y_initial;
			// 최대 이동거리보다 멀리왔다면 타겟을 해제하고 액션 타입을 리턴으로 변경
			if (distX * distX + distY * distY > _status.maxMoveDistance * _status.maxMoveDistance)
			{
				_action.bHasTarget = false;
				_action.wpTargetPlayer.reset();
				_action.eActionType = EMonsterActionType::RETURN;
				MoveToInitialPosition();
			}
			// 그렇지 않다면 플레이어를 쫓음
			else
			{
				MoveChasePlayer(spTarget);

				// 플레이어와의 거리가 _attackRange 이하라면 액션 타입을 공격으로 변경
				if (dfGET_SQUARE_DISTANCE(_pos.x, _pos.y, spTarget->GetPosX(), spTarget->GetPosY()) < _status.attackRange * _status.attackRange)
				{
					_action.eActionType = EMonsterActionType::ATTACK;
				}
			}
		}
		break;
	}
	// 처음위치로 돌아가는 중일 경우
	case EMonsterActionType::RETURN:
	{
		// 마지막으로 이동한 시간에서 1000 밀리세컨드가 지났다면 움직임
		if (_action.lastMoveTime + 1000 < currentTime)
		{
			_status.bMove = true;  // 이동 플래그 set
			_action.lastMoveTime = currentTime;

			// 초기 위치로 이동
			MoveToInitialPosition();

			// 현재 위치와 초기 위치 사이의 거리가 1.0 이하라면 액션타입을 로밍으로 변경
			float distX = _pos.x - _pos.x_initial;
			float distY = _pos.y - _pos.y_initial;
			if (distX * distX + distY * distY < 1.f)
			{
				_action.eActionType = EMonsterActionType::ROAMING;
			}
		}
		break;
	}
	// 공격 하려는 경우
	case EMonsterActionType::ATTACK:
	{
		// 플레이어 객체가 없으면 타겟을 해제한다.
		CPlayer_t spTarget = _action.wpTargetPlayer.lock();
		if (spTarget == nullptr)
		{
			_action.bHasTarget = false;
			_action.wpTargetPlayer.reset();
			_action.eActionType = EMonsterActionType::RETURN;
		}
		// 플레이어가 죽었다면 타겟을 해제하고 액션 타입을 리턴으로 바꿈
		else if (spTarget->IsAlive() == false)
		{
			_action.bHasTarget = false;
			_action.wpTargetPlayer.reset();
			_action.eActionType = EMonsterActionType::RETURN;
		}
		// 마지막으로 이동한 시간에서 1000 밀리세컨드가 지났고, 마지막으로 공격한 시간에서 _attackDelay 밀리세컨드가 지났다면 공격을 시도함
		else if (_action.lastMoveTime + 1000 < currentTime && _action.lastAttackTime + _status.attackDelay < currentTime)
		{
			// 플레이어와의 거리가 _attackRange 이상이라면 액션 타입을 쫓기로 변경
			if (dfGET_SQUARE_DISTANCE(_pos.x, _pos.y, spTarget->GetPosX(), spTarget->GetPosY()) > _status.attackRange * _status.attackRange)
			{
				_action.eActionType = EMonsterActionType::CHASING;
			}
			// 공격범위 내라면 공격
			else
			{
				_status.bAttack = true;  // 공격 플래그 set
				_action.lastAttackTime = currentTime;

				// 바라보는 방향을 플레이어쪽으로 설정
				float radian_direction = atan2(spTarget->GetPosY() - _pos.y, spTarget->GetPosX() - _pos.x);   // 몬스터 위치를 원점으로 했을 때 플레이어 위치의 각도 계산
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




// 현재 위치에서 랜덤 거리만큼 움직인다
void CMonster::MoveRandom()
{
	int direction = rand() % 360;
	float radian_direction = (float)direction * M_PI / 180.f;

	// 랜덤 위치 계산
	float x_new = _pos.x + cos(radian_direction) * _status.roamingMoveDistance;
	float y_new = _pos.y + sin(radian_direction) * _status.roamingMoveDistance;
	x_new = min(max(0.f, x_new), dfFIELD_POS_MAX_X - 0.5f);
	y_new = min(max(0.f, y_new), dfFIELD_POS_MAX_Y - 0.5f);

	// 좌표 업데이트
	_pos.MovePosition(x_new, y_new);
	_status.rotation = (360 - direction + 90) % 360;
}


// 현재 위치에서 초기 위치로 움직인다.
void CMonster::MoveToInitialPosition()
{
	float distX = _pos.x - _pos.x_initial;
	float distY = _pos.y - _pos.y_initial;

	// 초기 위치를 원점으로 했을 때, 현재 위치와 초기 위치 사이의 라디안 계산
	float radian_direction = atan2(distY, distX);
	if (radian_direction < 0)
		radian_direction = 2.f * M_PI + radian_direction;

	// 현재 위치에서 초기 위치쪽으로 이동(라디안의 반대방향으로 이동)
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

	// 몬스터 위치를 원점으로 했을 때, 몬스터 위치와 플레이어 위치 사이의 라디안 계산
	float distX = pTarget->GetPosX() - _pos.x;
	float distY = pTarget->GetPosY() - _pos.y;
	float radian_direction = atan2(distY, distX);
	if (radian_direction < 0)
		radian_direction = 2.f * M_PI + radian_direction;

	// 현재 위치에서 플레이어 위치쪽으로 이동(라디안 방향으로 이동)
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