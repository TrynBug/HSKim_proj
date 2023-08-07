#pragma once

#include "../utils/CMemoryPool.h"

#include "CTile.h"
#include "CSector.h"
#include "CMonster.h"
#include "CCrystal.h"

class CDBAsyncWriter;
class CMonster;
class CCrystal;

class CGameContentsField : public CGameContents
{
	using CPacket = netlib_game::CPacket;

public:
	CGameContentsField(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS);
	virtual ~CGameContentsField();

public:
	/* Init */
	void Init();

	/* Get DB ����͸� */
	virtual int GetQueryRequestPoolSize() const override;
	virtual int GetQueryRequestAllocCount() const override;
	virtual int GetQueryRequestFreeCount() const override;
	virtual __int64 GetQueryRunCount() const override;
	virtual float GetMaxQueryRunTime() const override;
	virtual float GetMinQueryRunTime() const override;
	virtual float GetAvgQueryRunTime() const override;
	virtual int GetRemainQueryCount() const override;

	/* Set */
	void SetSessionTransferLimit(int limit) { _sessionTransferLimit = limit; }

	/* object pool */
	CMonster& AllocMonster() { return *_poolMonster.Alloc(); }
	void FreeMonster(CMonster& monster) { return _poolMonster.Free(&monster); }
	CCrystal& AllocCrystal() { return *_poolCrystal.Alloc(); }
	void FreeCrystal(CCrystal& crystal) { return _poolCrystal.Free(&crystal); }

private:
	/* ������ Ŭ���� callback �Լ� ���� */
	virtual void OnUpdate() override;                                     // �ֱ������� ȣ���
	virtual void OnRecv(__int64 sessionId, CPacket& packet) override;     // �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
	virtual void OnSessionJoin(__int64 sessionId, PVOID data) override;   // �����忡 ������ ������ �� ȣ���
	virtual void OnSessionDisconnected(__int64 sessionId) override;       // ������ ������ ������ ������ �������� ȣ���
	virtual void OnError(const wchar_t* szError, ...) override;           // ���� �߻�

	
private:
	/* �÷��̾� */
	void InitializePlayerEnvironment(CPlayer& player);  // �÷��̾ Ÿ�Ͽ� ����ϰ�, ĳ���͸� �����ϰ�, �ֺ� ������Ʈ���� �ε��Ѵ�. �÷��̾� ����, ������ �ÿ� ȣ���Ѵ�.
	void MovePlayerTileAndSector(CPlayer& player);  // �÷��̾ ���� Ÿ��, ���Ϳ��� ���� Ÿ��, ���ͷ� �ű��.

	/* ���� */
	void MoveMonsterTileAndSector(CMonster& monster); // ���͸� ���� Ÿ��, ���Ϳ��� ���� Ÿ��, ���ͷ� �ű��.
	void MonsterAttack(CMonster& monster);   // ���Ͱ� ������
	void MonsterFindTarget(CMonster& monster);  // ������ Ÿ���� ã��

private:
	/* �÷��̾�� ��Ŷ ���� */
	int SendUnicast(CPlayer& player, CPacket& packet);                         // �Ѹ��� ������
	int SendAroundSector(int sectorX, int sectorY, CPacket& packet, const CPlayer* except);  // �ֺ� ���� ��ü�� ������
	int SendTwoAroundSector(int sectorX1, int sectorY1, int sectorX2, int sectorY2, CPacket& packet, const CPlayer* except);  // 2�� ������ �ֺ� ���� ��ü�� ������

private:
	/* utils */
	void ReadAreaInfoFromConfigJson(const CJsonParser& configJson, int areaNumber);

	
private:
	/* packet processing :: ��Ŷ ó�� */
	void PacketProc_MoveCharacter(CPacket& packet, CPlayer& player);   // ĳ���� �̵� ��û ó��
	void PacketProc_StopCharacter(CPacket& packet, CPlayer& player);   // ĳ���� ���� ��û ó��
	void PacketProc_Attack1(CPacket& packet, CPlayer& player);         // ĳ���� ���� ��û ó��1
	void PacketProc_Attack2(CPacket& packet, CPlayer& player);         // ĳ���� ���� ��û ó��2
	void PacketProc_Sit(CPacket& packet, CPlayer& player);             // �ٴڿ� �ɱ� ��û ó��
	void PacketProc_Pick(CPacket& packet, CPlayer& player);            // �ݱ� ��û ó��
	void PacketProc_PlayerRestart(CPacket& packet, CPlayer& player);   // �÷��̾� ���� �� �ٽ��ϱ� ��û
	void PacketProc_Echo(CPacket& packet, CPlayer& player);            // �׽�Ʈ ���� ��û ó��

	/* packet processing :: ��Ŷ�� ������ �Է� */
	void Msg_CreateMyCharacter(CPacket& packet, const CPlayer& player);        // �� ĳ���� ���� ��Ŷ
	void Msg_CreateOtherCharacter(CPacket& packet, const CPlayer& player);     // �ٸ� ĳ���� ���� ��Ŷ
	void Msg_CreateMonsterCharacter(CPacket& packet, const CMonster& monster, BYTE respawn); // ���� ���� ��Ŷ
	void Msg_RemoveObject(CPacket& packet, const CObject& object);             // ĳ����, ������Ʈ ���� ��Ŷ
	void Msg_MoveCharacter(CPacket& packet, const CPlayer& player);            // ĳ���� �̵� ��Ŷ
	void Msg_StopCharacter(CPacket& packet, const CPlayer& player);            // ĳ���� ���� ��Ŷ
	void Msg_MoveMonster(CPacket& packet, const CMonster& monster);            // ���� �̵� ��Ŷ
	void Msg_Attack1(CPacket& packet, const CPlayer& player);                  // ĳ���� ����1 ��Ŷ
	void Msg_Attack2(CPacket& packet, const CPlayer& player);                  // ĳ���� ����1 ��Ŷ
	void Msg_MonsterAttack(CPacket& packet, const CMonster& monster);          // ���� ���� ��Ŷ
	void Msg_Damage(CPacket& packet, const CObject& attackObject, const CObject& targetObject, int damage);    // ���ݴ�󿡰� ������� ���� ��Ŷ
	void Msg_MonsterDie(CPacket& packet, const CMonster& monster);             // ���� ��� ��Ŷ
	void Msg_CreateCrystal(CPacket& packet, const CCrystal& crystal);          // ũ����Ż ���� ��Ŷ
	void Msg_Pick(CPacket& packet, const CPlayer& player);                     // �Ա� ���� ��Ŷ
	void Msg_Sit(CPacket& packet, const CPlayer& player);                      // �ٴڿ� �ɱ� ��Ŷ
	void Msg_PickCrystal(CPacket& packet, const CPlayer& player, const CCrystal& crystal);   // ũ����Ż ȹ�� ��Ŷ
	void Msg_PlayerHP(CPacket& packet, const CPlayer& player);                 // �÷��̾� HP ���� ��Ŷ
	void Msg_PlayerDie(CPacket& packet, const CPlayer& player, int minusCrystal);  // �÷��̾� ���� ��Ŷ
	void Msg_PlayerRestart(CPacket& packet);                              // �÷��̾� ���� �� �ٽ��ϱ� ��Ŷ
	void Msg_Echo(CPacket& packet, const CPlayer& player, LONGLONG sendTick);  // �׽�Ʈ ���� ��Ŷ

private:
	/* database processing */
	void DB_Login(CPlayer& player);
	void DB_Logout(CPlayer& player);
	void DB_PlayerDie(CPlayer& player);
	void DB_PlayerRespawn(CPlayer& player);
	void DB_PlayerGetCrystal(CPlayer& player);
	void DB_PlayerRenewHP(CPlayer& player);
	void DB_InsertPlayerInfo(CPlayer& player);



private:
	int _sessionTransferLimit;

	/* position */
	float _maxPosX;
	float _maxPosY;

	/* Ÿ�� */
	CTileGrid _tile;
	int _tileViewRange;  // �÷��̾ ���� Ÿ�� ����(5�� ��� �÷��̾� ��ġ�κ��� ��,��,��,��� 5 Ÿ�Ͼ�, �� 11*11=121���� Ÿ���� ��)

	/* ���� */
	CSectorGrid _sector;
	int _numTileXPerSector;  // ���� �ϳ��� Ÿ���� ���η� �� �� �ִ��� ��. ���� 4��� ���� �ϳ��� Ÿ���� 4*4=16�� �ִ� ���̴�.
	int _sectorViewRange;

	/* config */
	struct Config
	{
		int numArea;                        // area ��
		int numInitialCrystal;              // �� ���� �ʱ� ũ����Ż ��
		int numRandomCrystalGeneration;     // �ʵ忡 �ʴ� ���������Ǵ� ũ����Ż ��
		int maxRandomCrystalGeneration;     // �ʵ忡 ���������Ǵ� ũ����Ż�� �ִ� ��
		int tilePickRange;                  // �ݱ� Ÿ�� ����
		int crystalLossWhenDie;             // �׾��� �� ũ����Ż �Ҵ� ��

		struct AreaInfo
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
		std::vector<AreaInfo> vecAreaInfo;   // area ����

		Config();
	};
	Config _config;



	/* monster */
	std::vector<CMonster*> _vecAllMonster;     // ��ü ���� �迭

	/* pool */
	CMemoryPool<CMonster> _poolMonster;
	CMemoryPool<CCrystal> _poolCrystal;

	/* DB */
	std::unique_ptr<CDBAsyncWriter> _pDBConn;

	/* utils */
	struct MathHelper
	{
	public:
		MathHelper();
		float GetCosine(unsigned short degree);
		float GetSine(unsigned short degree);
	private:
		float cosine[360];
		float sine[360];
	};
	MathHelper _math;
};



