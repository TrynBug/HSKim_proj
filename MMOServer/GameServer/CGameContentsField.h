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

	/* Get DB 모니터링 */
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
	/* 컨텐츠 클래스 callback 함수 구현 */
	virtual void OnUpdate() override;                                     // 주기적으로 호출됨
	virtual void OnRecv(__int64 sessionId, CPacket& packet) override;     // 컨텐츠가 관리하는 세션의 메시지큐에 메시지가 들어왔을 때 호출됨
	virtual void OnSessionJoin(__int64 sessionId, PVOID data) override;   // 스레드에 세션이 들어왔을 때 호출됨
	virtual void OnSessionDisconnected(__int64 sessionId) override;       // 스레드 내에서 세션의 연결이 끊겼을때 호출됨
	virtual void OnError(const wchar_t* szError, ...) override;           // 오류 발생

	
private:
	/* 플레이어 */
	void InitializePlayerEnvironment(CPlayer& player);  // 플레이어를 타일에 등록하고, 캐릭터를 생성하고, 주변 오브젝트들을 로드한다. 플레이어 접속, 리스폰 시에 호출한다.
	void MovePlayerTileAndSector(CPlayer& player);  // 플레이어를 이전 타일, 섹터에서 현재 타일, 섹터로 옮긴다.

	/* 몬스터 */
	void MoveMonsterTileAndSector(CMonster& monster); // 몬스터를 이전 타일, 섹터에서 현재 타일, 섹터로 옮긴다.
	void MonsterAttack(CMonster& monster);   // 몬스터가 공격함
	void MonsterFindTarget(CMonster& monster);  // 몬스터의 타겟을 찾음

private:
	/* 플레이어에게 패킷 전송 */
	int SendUnicast(CPlayer& player, CPacket& packet);                         // 한명에게 보내기
	int SendAroundSector(int sectorX, int sectorY, CPacket& packet, const CPlayer* except);  // 주변 섹터 전체에 보내기
	int SendTwoAroundSector(int sectorX1, int sectorY1, int sectorX2, int sectorY2, CPacket& packet, const CPlayer* except);  // 2개 섹터의 주변 섹터 전체에 보내기

private:
	/* utils */
	void ReadAreaInfoFromConfigJson(const CJsonParser& configJson, int areaNumber);

	
private:
	/* packet processing :: 패킷 처리 */
	void PacketProc_MoveCharacter(CPacket& packet, CPlayer& player);   // 캐릭터 이동 요청 처리
	void PacketProc_StopCharacter(CPacket& packet, CPlayer& player);   // 캐릭터 정지 요청 처리
	void PacketProc_Attack1(CPacket& packet, CPlayer& player);         // 캐릭터 공격 요청 처리1
	void PacketProc_Attack2(CPacket& packet, CPlayer& player);         // 캐릭터 공격 요청 처리2
	void PacketProc_Sit(CPacket& packet, CPlayer& player);             // 바닥에 앉기 요청 처리
	void PacketProc_Pick(CPacket& packet, CPlayer& player);            // 줍기 요청 처리
	void PacketProc_PlayerRestart(CPacket& packet, CPlayer& player);   // 플레이어 죽은 후 다시하기 요청
	void PacketProc_Echo(CPacket& packet, CPlayer& player);            // 테스트 에코 요청 처리

	/* packet processing :: 패킷에 데이터 입력 */
	void Msg_CreateMyCharacter(CPacket& packet, const CPlayer& player);        // 내 캐릭터 생성 패킷
	void Msg_CreateOtherCharacter(CPacket& packet, const CPlayer& player);     // 다른 캐릭터 생성 패킷
	void Msg_CreateMonsterCharacter(CPacket& packet, const CMonster& monster, BYTE respawn); // 몬스터 생성 패킷
	void Msg_RemoveObject(CPacket& packet, const CObject& object);             // 캐릭터, 오브젝트 삭제 패킷
	void Msg_MoveCharacter(CPacket& packet, const CPlayer& player);            // 캐릭터 이동 패킷
	void Msg_StopCharacter(CPacket& packet, const CPlayer& player);            // 캐릭터 정지 패킷
	void Msg_MoveMonster(CPacket& packet, const CMonster& monster);            // 몬스터 이동 패킷
	void Msg_Attack1(CPacket& packet, const CPlayer& player);                  // 캐릭터 공격1 패킷
	void Msg_Attack2(CPacket& packet, const CPlayer& player);                  // 캐릭터 공격1 패킷
	void Msg_MonsterAttack(CPacket& packet, const CMonster& monster);          // 몬스터 공격 패킷
	void Msg_Damage(CPacket& packet, const CObject& attackObject, const CObject& targetObject, int damage);    // 공격대상에게 대미지를 먹임 패킷
	void Msg_MonsterDie(CPacket& packet, const CMonster& monster);             // 몬스터 사망 패킷
	void Msg_CreateCrystal(CPacket& packet, const CCrystal& crystal);          // 크리스탈 생성 패킷
	void Msg_Pick(CPacket& packet, const CPlayer& player);                     // 먹기 동작 패킷
	void Msg_Sit(CPacket& packet, const CPlayer& player);                      // 바닥에 앉기 패킷
	void Msg_PickCrystal(CPacket& packet, const CPlayer& player, const CCrystal& crystal);   // 크리스탈 획득 패킷
	void Msg_PlayerHP(CPacket& packet, const CPlayer& player);                 // 플레이어 HP 보정 패킷
	void Msg_PlayerDie(CPacket& packet, const CPlayer& player, int minusCrystal);  // 플레이어 죽음 패킷
	void Msg_PlayerRestart(CPacket& packet);                              // 플레이어 죽은 후 다시하기 패킷
	void Msg_Echo(CPacket& packet, const CPlayer& player, LONGLONG sendTick);  // 테스트 에코 패킷

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

	/* 타일 */
	CTileGrid _tile;
	int _tileViewRange;  // 플레이어가 보는 타일 범위(5일 경우 플레이어 위치로부터 앞,뒤,좌,우로 5 타일씩, 즉 11*11=121개의 타일을 봄)

	/* 섹터 */
	CSectorGrid _sector;
	int _numTileXPerSector;  // 섹터 하나에 타일이 가로로 몇 개 있는지 수. 값이 4라면 섹터 하나에 타일이 4*4=16개 있는 것이다.
	int _sectorViewRange;

	/* config */
	struct Config
	{
		int numArea;                        // area 수
		int numInitialCrystal;              // 맵 상의 초기 크리스탈 수
		int numRandomCrystalGeneration;     // 필드에 초당 랜덤생성되는 크리스탈 수
		int maxRandomCrystalGeneration;     // 필드에 랜덤생성되는 크리스탈의 최대 수
		int tilePickRange;                  // 줍기 타일 범위
		int crystalLossWhenDie;             // 죽었을 때 크리스탈 잃는 양

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
		std::vector<AreaInfo> vecAreaInfo;   // area 정보

		Config();
	};
	Config _config;



	/* monster */
	std::vector<CMonster*> _vecAllMonster;     // 전체 몬스터 배열

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



