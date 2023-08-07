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
	virtual void VirtualDummy() override { __sVirtualCount += 2; } // �����
public:
	virtual void Update() override;

	/* init */
	void Init(EMonsterType type, int area, int tileX, int tileY, int hp, int damage);
	void SetRespawn(int tileX, int tileY);

	/* set */
	void Hit(int damage);                      // �ǰݵ�
	void SetTargetPlayer(const CPlayer* pPlayer);  // �÷��̾ Ÿ������ ������

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

		ULONGLONG deadTime;   // ��� �ð�
		int respawnWaitTime;  // ������ ��� �ð�

		float viewRange;            // �þ� ����
		float roamingRange;         // �ι� ����
		float roamingMoveDistance;  // �ι� �ѹ��� �����̴� �Ÿ�
		float chasingMoveDistance;  // �÷��̾� �ѱ� �ѹ��� �����̴� �Ÿ�
		float maxMoveDistance;      // �̵������� �ִ� �Ÿ�

		float attackRange;          // ���� ����
		DWORD attackDelay;          // ���� ������
		

	private:
		void Initialization(EMonsterType type, int area, int hp, int damage);
		void SetRespawn();
		void Hit(int damage);
	};
	Status _status;

	struct Action
	{
		EMonsterActionType eActionType;          // ���� �׼� Ÿ��
		ULONGLONG          lastMoveTime;         // ���������� �̵��� �ð�
		DWORD              nextRoamingWaitTime;  // ���� �ιֱ��� ��ٸ��� �ð�
		const CPlayer*     pTargetPlayer;        // ��ǥ �÷��̾�
		ULONGLONG          lastAttackTime;       // ���������� ������ �ð�
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