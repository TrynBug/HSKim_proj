#pragma once



#define FIELD_NUMBER_OF_AREA  7

struct StAreaInfo
{
	int x;
	int y;
	int range;
	int normalMonsterNumber;
	int normalMonsterHP;
	int normalMonsterDamage;
	int bossMonsterNumber;
	int bossMonsterHP;
	int bossMonsterDamage;
};


class CDBAsyncWriter;
class CTile;
class CSector;
class CMonster;
class CCrystal;

class CGameContentsField : public CGameContents
{
private:
	int _sessionTransferLimit;

	/* position */
	float _maxPosX;
	float _maxPosY;

	/* Ÿ�� */
	CTile** _tile;
	int _tileMaxX;
	int _tileMaxY;
	int _tileViewRange;  // �÷��̾ ���� Ÿ�� ����(5�� ��� �÷��̾� ��ġ�κ��� ��,��,��,��� 5 Ÿ�Ͼ�, �� 11*11=121���� Ÿ���� ��)

	/* ���� */
	CSector** _sector;
	int _numTileXPerSector;  // ���� �ϳ��� Ÿ���� ���η� �� �� �ִ��� ��. ���� 4��� ���� �ϳ��� Ÿ���� 4*4=16�� �ִ� ���̴�.
	int _sectorMaxX;
	int _sectorMaxY;
	int _sectorViewRange;
	


	/* config */
	StAreaInfo _arrAreaInfo[FIELD_NUMBER_OF_AREA];   // area ����
	int _numInitialCrystal;                       // �� ���� �ʱ� ũ����Ż ��

	int _numRandomCrystalGeneration;     // �ʵ忡 �ʴ� ���������Ǵ� ũ����Ż ��
	int _maxRandomCrystalGeneration;     // �ʵ忡 ���������Ǵ� ũ����Ż�� �ִ� ��
	int _tilePickRange;                  // �ݱ� Ÿ�� ����
	int _crystalLossWhenDie;             // �׾��� �� ũ����Ż �Ҵ� ��
	
	/* monster */
	int _numMonster;           // ��ü ���� ��
	CMonster** _arrPtrMonster;     // ��ü ���� �迭

	/* pool */
	CMemoryPool<CCrystal>* _poolCrystal;

	/* DB */
	CDBAsyncWriter* _pDBConn;

	/* utils */
	float _cosine[360];
	float _sine[360];

public:
	CGameContentsField(EContentsThread myContentsType, CGameServer* pGameServer, int FPS);
	virtual ~CGameContentsField();

public:
	/* Init */
	void Init();

	/* Get */
	int GetQueryRequestPoolSize() const;
	int GetQueryRequestAllocCount() const;
	int GetQueryRequestFreeCount() const;

	/* Get (�����Լ�) */
	// @Override
	virtual __int64 GetQueryRunCount() const;
	virtual float GetMaxQueryRunTime() const;
	virtual float GetMinQueryRunTime() const;
	virtual float GetAvgQueryRunTime() const;
	virtual int GetRemainQueryCount() const;

	/* Set */
	void SetSessionTransferLimit(int limit) { _sessionTransferLimit = limit; }

private:
	/* ������ Ŭ���� callback �Լ� ���� */
	virtual void OnUpdate();                                     // �ֱ������� ȣ���
	virtual void OnRecv(__int64 sessionId, game_netserver::CPacket* pPacket);    // �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
	virtual void OnSessionJoin(__int64 sessionId, PVOID data);   // �����忡 ������ ������ �� ȣ���
	virtual void OnSessionDisconnected(__int64 sessionId);       // ������ ������ ������ ������ �������� ȣ���

	
private:
	/* �÷��̾� */
	void InitializePlayerEnvironment(CPlayer* pPlayer);  // �÷��̾ Ÿ�Ͽ� ����ϰ�, ĳ���͸� �����ϰ�, �ֺ� ������Ʈ���� �ε��Ѵ�. �÷��̾� ����, ������ �ÿ� ȣ���Ѵ�.
	void MovePlayerTileAndSector(CPlayer* pPlayer);  // �÷��̾ ���� Ÿ��, ���Ϳ��� ���� Ÿ��, ���ͷ� �ű��.

	/* ���� */
	void MoveMonsterTileAndSector(CMonster* pMonster); // ���͸� ���� Ÿ��, ���Ϳ��� ���� Ÿ��, ���ͷ� �ű��.
	void MonsterAttack(CMonster* pMonster);   // ���Ͱ� ������
	void MonsterFindTarget(CMonster* pMonster);  // ������ Ÿ���� ã��

private:
	/* �÷��̾�� ��Ŷ ���� */
	int SendUnicast(CPlayer* pPlayer, game_netserver::CPacket* pPacket);                         // �Ѹ��� ������
	int SendAroundSector(int sectorX, int sectorY, game_netserver::CPacket* pPacket, CPlayer* except);  // �ֺ� ���� ��ü�� ������
	int SendTwoAroundSector(int sectorX1, int sectorY1, int sectorX2, int sectorY2, game_netserver::CPacket* pPacket, CPlayer* except);  // 2�� ������ �ֺ� ���� ��ü�� ������

	/* ��Ŷ ó�� */
	void PacketProc_MoveCharacter(game_netserver::CPacket* pPacket, CPlayer* pPlayer);   // ĳ���� �̵� ��û ó��
	void PacketProc_StopCharacter(game_netserver::CPacket* pPacket, CPlayer* pPlayer);   // ĳ���� ���� ��û ó��
	void PacketProc_Attack1(game_netserver::CPacket* pPacket, CPlayer* pPlayer);         // ĳ���� ���� ��û ó��1
	void PacketProc_Attack2(game_netserver::CPacket* pPacket, CPlayer* pPlayer);         // ĳ���� ���� ��û ó��2
	void PacketProc_Sit(game_netserver::CPacket* pPacket, CPlayer* pPlayer);             // �ٴڿ� �ɱ� ��û ó��
	void PacketProc_Pick(game_netserver::CPacket* pPacket, CPlayer* pPlayer);            // �ݱ� ��û ó��
	void PacketProc_PlayerRestart(game_netserver::CPacket* pPacket, CPlayer* pPlayer);   // �÷��̾� ���� �� �ٽ��ϱ� ��û
	void PacketProc_Echo(game_netserver::CPacket* pPacket, CPlayer* pPlayer);            // �׽�Ʈ ���� ��û ó��

	/* ��Ŷ�� ������ �Է� */
	void Msg_CreateMyCharacter(game_netserver::CPacket* pPacket, CPlayer* pPlayer);        // �� ĳ���� ���� ��Ŷ
	void Msg_CreateOtherCharacter(game_netserver::CPacket* pPacket, CPlayer* pPlayer);     // �ٸ� ĳ���� ���� ��Ŷ
	void Msg_CreateMonsterCharacter(game_netserver::CPacket* pPacket, CMonster* pMonster, BYTE respawn); // ���� ���� ��Ŷ
	void Msg_RemoveObject(game_netserver::CPacket* pPacket, CObject* pObject);             // ĳ����, ������Ʈ ���� ��Ŷ
	void Msg_MoveCharacter(game_netserver::CPacket* pPacket, CPlayer* pPlayer);            // ĳ���� �̵� ��Ŷ
	void Msg_StopCharacter(game_netserver::CPacket* pPacket, CPlayer* pPlayer);            // ĳ���� ���� ��Ŷ
	void Msg_MoveMonster(game_netserver::CPacket* pPacket, CMonster* pMonster);            // ���� �̵� ��Ŷ
	void Msg_Attack1(game_netserver::CPacket* pPacket, CPlayer* pPlayer);                  // ĳ���� ����1 ��Ŷ
	void Msg_Attack2(game_netserver::CPacket* pPacket, CPlayer* pPlayer);                  // ĳ���� ����1 ��Ŷ
	void Msg_MonsterAttack(game_netserver::CPacket* pPacket, CMonster* pMonster);          // ���� ���� ��Ŷ
	void Msg_Damage(game_netserver::CPacket* pPacket, CObject* pAttackObject, CObject* pTargetObject, int damage);    // ���ݴ�󿡰� ������� ���� ��Ŷ
	void Msg_MonsterDie(game_netserver::CPacket* pPacket, CMonster* pMonster);             // ���� ��� ��Ŷ
	void Msg_CreateCrystal(game_netserver::CPacket* pPacket, CCrystal* pCrystal);          // ũ����Ż ���� ��Ŷ
	void Msg_Pick(game_netserver::CPacket* pPacket, CPlayer* pPlayer);                     // �Ա� ���� ��Ŷ
	void Msg_Sit(game_netserver::CPacket* pPacket, CPlayer* pPlayer);                      // �ٴڿ� �ɱ� ��Ŷ
	void Msg_PickCrystal(game_netserver::CPacket* pPacket, CPlayer* pPlayer, CCrystal* pCrystal);   // ũ����Ż ȹ�� ��Ŷ
	void Msg_PlayerHP(game_netserver::CPacket* pPacket, CPlayer* pPlayer);                 // �÷��̾� HP ���� ��Ŷ
	void Msg_PlayerDie(game_netserver::CPacket* pPacket, CPlayer* pPlayer, int minusCrystal);  // �÷��̾� ���� ��Ŷ
	void Msg_PlayerRestart(game_netserver::CPacket* pPacket);                              // �÷��̾� ���� �� �ٽ��ϱ� ��Ŷ
	void Msg_Echo(game_netserver::CPacket* pPacket, CPlayer* pPlayer, LONGLONG sendTick);  // �׽�Ʈ ���� ��Ŷ

	/* DB�� ������ ���� */
	void DB_Login(CPlayer* pPlayer);
	void DB_Logout(CPlayer* pPlayer);
	void DB_PlayerDie(CPlayer* pPlayer);
	void DB_PlayerRespawn(CPlayer* pPlayer);
	void DB_PlayerGetCrystal(CPlayer* pPlayer);
	void DB_PlayerRenewHP(CPlayer* pPlayer);
	void DB_InsertPlayerInfo(CPlayer* pPlayer);

private:
	/* utils */
	void ReadAreaInfoFromConfigJson(CJsonParser& configJson, int areaNumber);
	float GetCosine(unsigned short degree);
	float GetSine(unsigned short degree);
};
