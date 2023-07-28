#pragma once
#include "CObject.h"

namespace game_netserver
{
	class CPacket;
}

class CPlayer : public CObject
{
private:
	friend class CGameServer;
	friend class CGameContents;
	friend class CGameContentsAuth;
	friend class CGameContentsField;

	__int64 _sessionId;
	INT64	_accountNo;
	char	_sessionKey[64];	// ������ū

	wchar_t _szIP[16];
	unsigned short _port;

	bool _bLogin;                  // �α��ο���
	bool _bDisconnect;             // ������� ����
	ULONGLONG _lastHeartBeatTime;  // ���������� heart beat ���� �ð�

	WCHAR _userId[20];
	WCHAR _userNick[20];

	BYTE _characterType;
	float _posX;
	float _posY;
	float _oldPosX;
	float _oldPosY;
	int _tileX;
	int _tileY;
	int _oldTileX;
	int _oldTileY;
	int _sectorX;
	int _sectorY;
	int _oldSectorX;
	int _oldSectorY;
	unsigned short _rotation;
	int _crystal;
	int _oldCrystal;
	int _maxHp;
	int _hp;
	INT64 _exp;
	unsigned short _level;
	BYTE _die;
	BYTE _sit;
	BYTE _respawn;
	BYTE _VKey;
	BYTE _HKey;

	bool _bNewPlayer;              // ���� ������ ���� ����
	bool _bAlive;
	bool _bMove;
	bool _bAttack;
	bool _bHit; 
	int _damageAttack1;
	int _damageAttack2;

	int _amountSitHPRecovery;     // �ɱ� �����϶� �ʴ� ȸ���Ǵ� HP��
	int _sitStartHp;              // �ɱ� �������� ���� HP
	ULONGLONG _sitStartTime;     // �ɱ� ������ �ð�
	ULONGLONG _lastSitHPRecoveryTime;  // �ɱ�� ���� HP�� ȸ���� ������ �ð�

	bool _bJustDied;      // ��� ����
	bool _bJustStandUp;   // ��� �Ͼ
	ULONGLONG _deadTime;  // ���� �ð�


	ULONGLONG _lastDBUpdateTime;    // ���������� DB ������Ʈ�� �� �ð�

	std::vector<game_netserver::CPacket*> _vecPacketBuffer;

public:
	CPlayer();
	virtual ~CPlayer();

public:
	/* get */
	float GetPosX() { return _posX; }
	float GetPosY() { return _posY; }
	bool IsAlive() { return _bAlive; }

	/* init */
	void Init(__int64 sessionId);
	void SetAccountInfo(const char* userId, const char* userNick, const wchar_t* ip, unsigned short port);
	void SetCharacterInfo(BYTE characterType);
	void LoadCharacterInfo(BYTE characterType, float posX, float posY, int tileX, int tileY, USHORT rotation, int crystal, int hp, INT64 exp, USHORT level, BYTE die);

	/* set */
	void SetLogin(INT64 accountNo, char sessionKey[64]);
	void SetHeartBeatTime() { _lastHeartBeatTime = GetTickCount64(); }
	void SetSit();
	void SetRespawn(int tileX, int tileY);


	void AddCrystal(int amount);
	void Hit(int damage);


	// @Override
	virtual void Update();  
};

