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

	EMonsterType _eMonsterType;
	int _area;
	float _posX;
	float _posY;
	float _oldPosX;
	float _oldPosY;
	float _initialPosX;
	float _initialPosY;
	int _tileX;
	int _tileY;
	int _oldTileX;
	int _oldTileY;
	int _sectorX;
	int _sectorY;
	int _oldSectorX;
	int _oldSectorY;
	unsigned short _rotation;
	int _maxHp;
	int _hp;
	int _damage;
	BYTE _die;
	BYTE _respawn;

	bool _bAlive;
	bool _bMove;
	bool _bAttack;
	bool _bHasTarget;
	ULONGLONG _deadTime;   // ��� �ð�
	int _respawnWaitTime;  // ������ ��� �ð�

	EMonsterActionType _eActionType; // ���� �׼� Ÿ��
	float _viewRange;            // �þ� ����
	float _roamingRange;         // �ι� ����
	float _roamingMoveDistance;  // �ι� �ѹ��� �����̴� �Ÿ�
	float _chasingMoveDistance;  // �÷��̾� �ѱ� �ѹ��� �����̴� �Ÿ�
	float _maxMoveDistance;      // �̵������� �ִ� �Ÿ�
	ULONGLONG _lastMoveTime;     // ���������� �̵��� �ð�
	DWORD _nextRoamingWaitTime;  // ���� �ιֱ��� ��ٸ��� �ð�
	CPlayer* _pTargetPlayer;     // ��ǥ �÷��̾�

	float _attackRange;          // ���� ����
	DWORD _attackDelay;          // ���� ������
	ULONGLONG _lastAttackTime;   // ���������� ������ �ð�



public:
	CMonster();
	virtual ~CMonster();

public:
	void Init(EMonsterType type, int area, int tileX, int tileY, int hp, int damage);
	void SetRespawn(int tileX, int tileY);

	void Hit(int damage);                      // �ǰݵ�
	void SetTargetPlayer(CPlayer* pPlayer);  // �÷��̾ Ÿ������ ������

	// @Override
	virtual void Update();

private:
	void MoveRandom();
	void MoveToInitialPosition();
	void MoveChasePlayer();
};