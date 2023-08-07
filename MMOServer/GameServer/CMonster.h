#pragma once
#include "CObject.h"

enum class EMonsterType
{
	NORMAL,
	BOSS,
	END
};

enum class EMonsterActionType
{
	ROAMING,
	RETURN,
	CHASING,
	ATTACK,
};

class CPlayer;

class CMonster : public CObject
{
private:
	friend class CGameContentsField;
	struct Status;
	struct Position;

public:
	CMonster();
	virtual ~CMonster();
	virtual void VirtualDummy() override { __sVirtualCount += 2; } // 디버그
public:
	virtual void Update() override;

	/* init */
	void Init(EMonsterType type, int area, int tileX, int tileY, int hp, int damage);
	void SetRespawn(int tileX, int tileY);

	/* set */
	void Hit(int damage);                      // 피격됨
	void SetTargetPlayer(const CPlayer* pPlayer);  // 플레이어를 타겟으로 지정함

	/* get */
	bool IsAlive() const { return _status.bAlive; }
	bool IsMove() const { return _status.bMove; }
	bool IsAttack() const { return _status.bAttack; }
	bool IsHasTarget() const { return _action.bHasTarget; }
	const Status& GetStatus() const { return _status; }
	const Position& GetPosition() const { return _pos; }
	

private:
	void MoveRandom();
	void MoveToInitialPosition();
	void MoveChasePlayer();


private:

	struct Status
	{
		friend class CMonster;
	public:
		bool bAlive;
		bool bMove;
		bool bAttack;

		EMonsterType eMonsterType;
		int area;

		unsigned short rotation;
		int maxHp;
		int hp;
		int damage;
		BYTE die;
		BYTE respawn;

		ULONGLONG deadTime;   // 사망 시간
		int respawnWaitTime;  // 리스폰 대기 시간

		float viewRange;            // 시야 범위
		float roamingRange;         // 로밍 범위
		float roamingMoveDistance;  // 로밍 한번에 움직이는 거리
		float chasingMoveDistance;  // 플레이어 쫓기 한번에 움직이는 거리
		float maxMoveDistance;      // 이동가능한 최대 거리

		float attackRange;          // 공격 범위
		DWORD attackDelay;          // 공격 딜레이
		

	private:
		void Initialization(EMonsterType type, int area, int hp, int damage);
		void SetRespawn();
		void Hit(int damage);
	};
	Status _status;

	struct Action
	{
		EMonsterActionType eActionType;          // 현재 액션 타입
		ULONGLONG          lastMoveTime;         // 마지막으로 이동한 시간
		DWORD              nextRoamingWaitTime;  // 다음 로밍까지 기다리는 시간
		const CPlayer*     pTargetPlayer;        // 목표 플레이어
		ULONGLONG          lastAttackTime;       // 마지막으로 공격한 시간
		bool bHasTarget;

		void Init();
	};
	Action _action;

	struct Position
	{
		friend class CMonster;
	public:
		float x;
		float y;
		int tileX;
		int tileY;
		int sectorX;
		int sectorY;

		float x_initial;
		float y_initial;

		float x_old;
		float y_old;
		int tileX_old;
		int tileY_old;
		int sectorX_old;
		int sectorY_old;

	private:
		void MovePosition(float posX, float posY);
		void MoveTile(int tileX, int tileY);
		void ResetTile(int tileX, int tileY);
	};
	Position _pos;


};