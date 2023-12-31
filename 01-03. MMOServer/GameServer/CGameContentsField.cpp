#include "stdafx.h"

#include "CGameServer.h"
#include "CGameContents.h"
#include "../common/CommonProtocol.h"

#include "CObject.h"
#include "CPlayer.h"

#include "../utils/CJsonParser.h"
#include "../utils/CDBAsyncWriter.h"
#include "../utils/logger.h"
//#define ENABLE_PROFILER
#include "../utils/profiler.h"

#include "CGameContentsField.h"
#include <psapi.h>

using namespace gameserver;

#define FIELD_RANDOM_TILE_X(center_x, range_x)  min(max(center_x - range_x + (rand() % (range_x * 2)), 0), dfFIELD_TILE_MAX_X - 1)
#define FIELD_RANDOM_TILE_Y(center_y, range_y)  min(max(center_y - range_y + (rand() % (range_y * 2)), 0), dfFIELD_TILE_MAX_Y - 1)
#define FIELD_PLAYER_RESPAWN_TILE_X             (85 + (rand() % 21))
#define FIELD_PLAYER_RESPAWN_TILE_Y             (93 + (rand() % 31))
#define FIELD_RANDOM_POS_X                      (_maxPosX / 32767.f * (float)rand())
#define FIELD_RANDOM_POS_Y                      (_maxPosY / 32767.f * (float)rand())

#define M_PI  3.1415926535f


CGameContentsField::CGameContentsField(EContentsThread myContentsType, std::unique_ptr<CGameServerAccessor> pAccessor, int FPS)
	//: CGameContents(myContentsType, pGameServer, FPS, CONTENTS_MODE_SEND_PACKET_IMMEDIATELY)
	: CGameContents(myContentsType, std::move(pAccessor), FPS)
	, _sessionTransferLimit(1000000)
	, _poolMonster(0, true, false)
	, _poolCrystal(0, true, false)
	, _tile(dfFIELD_TILE_MAX_X, dfFIELD_TILE_MAX_Y)
	, _sector(dfFIELD_TILE_MAX_X / dfFIELD_NUM_TILE_X_PER_SECTOR, dfFIELD_TILE_MAX_Y / dfFIELD_NUM_TILE_X_PER_SECTOR, dfFIELD_SECTOR_VIEW_RANGE)
{
}

CGameContentsField::~CGameContentsField()
{
	_pDBConn->Close();
}


CGameContentsField::Config::Config()
	: numArea(0)
	, numInitialCrystal(0)
	, numRandomCrystalGeneration(0)
	, maxRandomCrystalGeneration(0)
	, tilePickRange(1)
	, crystalLossWhenDie(0)
{
}



CGameContentsField::MathHelper::MathHelper()
{
	for (int i = 0; i < 360; i++)
	{
		cosine[i] = cos((float)i * M_PI / 180.f);
		sine[i] = sin((float)i * M_PI / 180.f);
	}
}

float CGameContentsField::MathHelper::GetCosine(unsigned short degree)
{
	return cosine[degree % 360];
}
float CGameContentsField::MathHelper::GetSine(unsigned short degree)
{
	return sine[degree % 360];
}





/* Init */
void CGameContentsField::Init()
{
	// config 파일 json parser 얻기
	const CJsonParser& jsonParser = GetConfig().GetJsonParser();

	// config 파일 읽기
	int FieldSessionPacketProcessLimit = jsonParser[L"ContentsField"][L"SessionPacketProcessLimit"].Int();
	int FieldHeartBeatTimeout = jsonParser[L"ContentsField"][L"HeartBeatTimeout"].Int();
	SetSessionPacketProcessLimit(FieldSessionPacketProcessLimit);
	EnableHeartBeatTimeout(FieldHeartBeatTimeout);
	_config.numInitialCrystal = jsonParser[L"ContentsField"][L"InitialCrystalNumber"].Int();      // 맵 상의 초기 크리스탈 수

	// _config.vecAreaInfo 에 area 정보를 입력한다.
	_config.numArea = jsonParser[L"ContentsField"][L"NumOfArea"].Int();
	for (int i = 0; i < _config.numArea; i++)
	{
		ReadAreaInfoFromConfigJson(jsonParser, i);
	}


	// 게임 멤버 설정
	_config.numRandomCrystalGeneration = 1;        // 필드에 초당 랜덤생성되는 크리스탈 수
	_config.maxRandomCrystalGeneration = 1000;     // 필드에 랜덤생성되는 크리스탈의 최대 수
	_config.tilePickRange = 2;                     // 줍기 타일 범위
	_config.crystalLossWhenDie = 1000;             // 죽었을때 크리스탈 잃는 양


	_maxPosX = dfFIELD_POS_MAX_X;
	_maxPosY = dfFIELD_POS_MAX_Y;
	_numTileXPerSector = dfFIELD_NUM_TILE_X_PER_SECTOR;
	_sectorViewRange = dfFIELD_SECTOR_VIEW_RANGE;
	_tileViewRange = _sectorViewRange * _numTileXPerSector + _numTileXPerSector / 2;
	


	// DB 연결
	_pDBConn = std::make_unique<CDBAsyncWriter>();
	_pDBConn->ConnectAndRunThread("127.0.0.1", "root", "vmfhzkepal!", "gamedb", 3306);


	// 몬스터를 생성하고 벡터와 타일에 입력한다
	for (int area = 0; area < _config.numArea; area++)
	{
		Config::AreaInfo& areaInfo = _config.vecAreaInfo[area];
		// 일반 몬스터 생성
		for (int i = 0; i < areaInfo.normalMonsterNumber; i++)
		{
			CMonster_t pMonster = AllocMonster();
			pMonster->Init(EMonsterType::NORMAL, area
				, FIELD_RANDOM_TILE_X(areaInfo.x, areaInfo.range)
				, FIELD_RANDOM_TILE_Y(areaInfo.y, areaInfo.range)
				, areaInfo.normalMonsterHP
				, areaInfo.normalMonsterDamage
			);

			_vecAllMonster.push_back(pMonster);
			auto& pos = pMonster->GetPosition();
			_tile.AddObject(pos.tileX, pos.tileY, ETileObjectType::MONSTER, pMonster);
			_sector.AddObject(pos.sectorX, pos.sectorY, ESectorObjectType::MONSTER, pMonster);
		}
		// 보스 몬스터 생성
		for (int i = 0; i < areaInfo.bossMonsterNumber; i++)
		{
			CMonster_t pMonster = AllocMonster();
			pMonster->Init(EMonsterType::BOSS, area
				, FIELD_RANDOM_TILE_X(areaInfo.x, areaInfo.range / 2)
				, FIELD_RANDOM_TILE_Y(areaInfo.y, areaInfo.range / 2)
				, areaInfo.bossMonsterHP
				, areaInfo.bossMonsterDamage
			);


			_vecAllMonster.push_back(pMonster);
			auto& pos = pMonster->GetPosition();
			_tile.AddObject(pos.tileX, pos.tileY, ETileObjectType::MONSTER, pMonster);
			_sector.AddObject(pos.sectorX, pos.sectorY, ESectorObjectType::MONSTER, pMonster);
		}
	}

	// 초기 크리스탈 생성
	for(int i = 0; i < _config.numInitialCrystal; i++)
	{
		CCrystal_t pCrystal = AllocCrystal();
		pCrystal->Init(FIELD_RANDOM_POS_X, FIELD_RANDOM_POS_Y, rand() % 3 + 1);
		auto& pos = pCrystal->GetPosition();
		_tile.AddObject(pos.tileX, pos.tileY, ETileObjectType::CRYSTAL, pCrystal);
		_sector.AddObject(pos.sectorX, pos.sectorY, ESectorObjectType::CRYSTAL, pCrystal);
	}

}


/* Get */
int CGameContentsField::GetQueryRequestPoolSize() const { return _pDBConn->GetQueryRequestPoolSize(); }
int CGameContentsField::GetQueryRequestAllocCount() const { return _pDBConn->GetQueryRequestAllocCount(); }
int CGameContentsField::GetQueryRequestFreeCount() const { return _pDBConn->GetQueryRequestFreeCount(); }

/* virtual Get */
// @Override
__int64 CGameContentsField::GetQueryRunCount() const { return _pDBConn->GetQueryRunCount(); }
float CGameContentsField::GetMaxQueryRunTime() const { return _pDBConn->GetMaxQueryRunTime(); }
float CGameContentsField::GetMinQueryRunTime() const { return _pDBConn->GetMinQueryRunTime(); }
float CGameContentsField::GetAvgQueryRunTime() const { return _pDBConn->GetAvgQueryRunTime(); }
int CGameContentsField::GetRemainQueryCount() const { return _pDBConn->GetUnprocessedQueryCount(); }


/* object pool */
CMonster_t CGameContentsField::AllocMonster()
{
	auto Deleter = [this](CMonster* pMonster) {
		this->_poolMonster.Free(pMonster);
	};
	std::shared_ptr<CMonster> pMonster(_poolMonster.Alloc(), Deleter);
	return pMonster;
}

CCrystal_t CGameContentsField::AllocCrystal()
{
	auto Deleter = [this](CCrystal* pCrystal) {
		this->_poolCrystal.Free(pCrystal);
	};
	std::shared_ptr<CCrystal> pCrystal(_poolCrystal.Alloc(), Deleter);
	return pCrystal;
}



/* 컨텐츠 클래스 callback 함수 구현 */

 // 컨텐츠 업데이트
void CGameContentsField::OnUpdate()
{
	// 모든 오브젝트 업데이트
	ULONGLONG currentTime = GetTickCount64();
	int sectorMaxX = _sector.GetMaxX();
	int sectorMaxY = _sector.GetMaxY();
	for (int y = 0; y < sectorMaxY; y++)
	{
		for (int x = 0; x < sectorMaxX; x++)
		{
			// 크리스탈 업데이트
			auto& vecCrystal = _sector.GetObjectVector(x, y, ESectorObjectType::CRYSTAL);
			for (int iCnt = 0; iCnt < vecCrystal.size();)
			{
				CCrystal_t pCrystal = std::static_pointer_cast<CCrystal>(vecCrystal[iCnt]);
				pCrystal->Update();

				// 삭제된 크리스탈을 타일과 섹터에서 제거
				if (pCrystal->IsAlive() == false)
				{
					auto& pos = pCrystal->GetPosition();
					_tile.RemoveObject(pos.tileX, pos.tileY, ETileObjectType::CRYSTAL, pCrystal);
					_sector.RemoveObject(x, y, ESectorObjectType::CRYSTAL, pCrystal);
					continue;
				}

				iCnt++;
			}

			// 섹터내의 몬스터 업데이트
			auto& vecMonster = _sector.GetObjectVector(x, y, ESectorObjectType::MONSTER);
			for (int iCnt = 0; iCnt < vecMonster.size();)
			{
				CMonster_t pMonster = std::static_pointer_cast<CMonster>(vecMonster[iCnt]);
				auto& posMon = pMonster->GetPosition();
				auto& statMon = pMonster->GetStatus();
				pMonster->Update();

				// 몬스터가 살아있을 경우
				if (pMonster->IsAlive() == true)
				{
					// 타겟이 없을 경우 주변에서 찾음
					if (pMonster->IsHasTarget() == false)
					{
						MonsterFindTarget(pMonster);
					}

					// 몬스터가 공격했을 경우
					if (pMonster->IsAttack() == true)
					{
						// 몬스터 공격판정 처리
						MonsterAttack(pMonster);
					}
				}
				// 몬스터가 사망했을경우
				else if (pMonster->IsAlive() == false)
				{
					// 몬스터 사망 패킷 전송
					CPacket& packetMonsterDie = AllocPacket();
					Msg_MonsterDie(packetMonsterDie, pMonster);
					SendAroundSector(posMon.sectorX, posMon.sectorY, packetMonsterDie, nullptr);
					packetMonsterDie.SubUseCount();
					
					// 크리스탈 생성
					CCrystal_t pCrystal = AllocCrystal();
					pCrystal->Init(posMon.x, posMon.y, rand() % 3);
					auto& posCrystal = pCrystal->GetPosition();
					_tile.AddObject(posCrystal.tileX, posCrystal.tileY, ETileObjectType::CRYSTAL, pCrystal);
					_sector.AddObject(x, y, ESectorObjectType::CRYSTAL, pCrystal);

					// 크리스탈 생성 패킷 전송
					CPacket& packetCreateCrystal = AllocPacket();
					Msg_CreateCrystal(packetCreateCrystal, pCrystal);
					SendAroundSector(posCrystal.sectorX, posCrystal.sectorY, packetCreateCrystal, nullptr);
					packetCreateCrystal.SubUseCount();

					// 몬스터를 타일과 섹터에서 제거
					_tile.RemoveObject(posMon.tileX, posMon.tileY, ETileObjectType::MONSTER, pMonster);
					_sector.RemoveObject(x, y, ESectorObjectType::MONSTER, pMonster);

					LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnUpdate. monster dead. clientId:%lld, tile:(%d, %d), hp:%d\n", pMonster->GetClientId(), posMon.tileX, posMon.tileY, statMon.hp);
					continue;
				}

				iCnt++;
			}

			// 플레이어 업데이트
			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int iCnt = 0; iCnt < vecPlayer.size(); iCnt++)
			{
				CPlayer_t pPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[iCnt]);
				auto& character = pPlayer->GetCharacter();
				auto& pos = pPlayer->GetPosition();

				pPlayer->Update();
				
				if (pPlayer->IsAlive())
				{
					// 플레이어가 방금 일어났을 경우 HP 패킷 전송
					if (pPlayer->IsJustStandUp() == true)
					{
						CPacket& packetPlayerHp = AllocPacket();
						Msg_PlayerHP(packetPlayerHp, pPlayer);
						SendUnicast(pPlayer, packetPlayerHp);
						packetPlayerHp.SubUseCount();

						DB_PlayerRenewHP(pPlayer);
					}
				}

				// 방금 죽었을경우
				if (pPlayer->IsJustDied() == true)
				{
					pPlayer->AddCrystal(-_config.crystalLossWhenDie);

					// 플레이어 사망패킷 전송
					CPacket& packetPlayerDie = AllocPacket();
					Msg_PlayerDie(packetPlayerDie, pPlayer, _config.crystalLossWhenDie);
					SendAroundSector(pos.sectorX, pos.sectorY, packetPlayerDie, nullptr);
					packetPlayerDie.SubUseCount();

					// DB에 저장
					DB_PlayerDie(pPlayer);

					LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnUpdate. player die. sessionId:%lld, accountNo:%lld, hp:%d\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo(), character.hp);
				}
			}
		}
	}



	// 타일에 없는 몬스터를 포함하여 전체 몬스터 체크 (몬스터의 타일이동이 필요한 로직을 여기서 처리)
	for (int iCnt = 0; iCnt < _vecAllMonster.size(); iCnt++)
	{
		CMonster_t pMonster = _vecAllMonster[iCnt];
		auto& statMon = pMonster->GetStatus();
		auto& posMon = pMonster->GetPosition();

		// 몬스터가 살아있음
		if (pMonster->IsAlive() == true)
		{
			// 몬스터가 이동했을 경우
			if (pMonster->IsMove() == true)
			{
				// 타일 및 섹터 이동
				MoveMonsterTileAndSector(pMonster);

				// 몬스터 이동 패킷 전송
				CPacket& packetMoveMonster = AllocPacket();
				Msg_MoveMonster(packetMoveMonster, pMonster);
				SendAroundSector(posMon.sectorX, posMon.sectorY, packetMoveMonster, nullptr);
				packetMoveMonster.SubUseCount();
			}
		}
		// 몬스터가 죽어있음
		else if (pMonster->IsAlive() == false)
		{
			// 부활 시간만큼 대기했으면 부활
			if (statMon.deadTime + statMon.respawnWaitTime < currentTime)
			{
				// 부활 및 타일에 입력
				Config::AreaInfo& area = _config.vecAreaInfo[statMon.area];
				if (statMon.eMonsterType == EMonsterType::NORMAL)
				{
					pMonster->SetRespawn(FIELD_RANDOM_TILE_X(area.x, area.range)
						, FIELD_RANDOM_TILE_Y(area.y, area.range));
				}
				else
				{
					pMonster->SetRespawn(FIELD_RANDOM_TILE_X(area.x, area.range / 2)
						, FIELD_RANDOM_TILE_Y(area.y, area.range / 2));
				}

				_tile.AddObject(posMon.tileX, posMon.tileY, ETileObjectType::MONSTER, pMonster);
				_sector.AddObject(posMon.sectorX, posMon.sectorY, ESectorObjectType::MONSTER, pMonster);

				// 몬스터 리스폰 패킷 전송
				CPacket& packetRespawnMonster = AllocPacket();
				Msg_CreateMonsterCharacter(packetRespawnMonster, pMonster, 1);
				SendAroundSector(posMon.sectorX, posMon.sectorY, packetRespawnMonster, nullptr);
				packetRespawnMonster.SubUseCount();

				LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnUpdate. monster respawn. clientId:%lld, tile:(%d, %d)\n", pMonster->GetClientId(), posMon.tileX, posMon.tileY);
			}
		}
	}

}


// 컨텐츠가 관리하는 세션의 메시지큐에 메시지가 들어왔을 때 호출됨
void CGameContentsField::OnRecv(__int64 sessionId, CPacket& packet)
{
	// 세션ID로 플레이어 객체를 얻는다.
	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"CGameContentsField::OnRecv. No player in the map. sessionId:%lld\n", sessionId);
		return;
	}

	// 메시지 타입 확인
	WORD msgType;
	packet >> msgType;

	// 메시지 타입에 따른 처리
	if (msgType < 2000)
	{
		switch (msgType)
		{
			// 캐릭터 이동 요청
		case en_PACKET_CS_GAME_REQ_MOVE_CHARACTER:
		{
			PacketProc_MoveCharacter(packet, pPlayer);
			break;
		}

		// 캐릭터 정지 요청
		case en_PACKET_CS_GAME_REQ_STOP_CHARACTER:
		{
			PacketProc_StopCharacter(packet, pPlayer);
			break;
		}

		// 캐릭터 공격 요청 처리1
		case en_PACKET_CS_GAME_REQ_ATTACK1:
		{
			PacketProc_Attack1(packet, pPlayer);
			break;
		}

		// 캐릭터 공격 요청 처리2
		case en_PACKET_CS_GAME_REQ_ATTACK2:
		{
			PacketProc_Attack2(packet, pPlayer);
			break;
		}

		// 줍기 요청
		case en_PACKET_CS_GAME_REQ_PICK:
		{
			PacketProc_Pick(packet, pPlayer);
			break;
		}

		// 바닥에 앉기 요청
		case en_PACKET_CS_GAME_REQ_SIT:
		{
			PacketProc_Sit(packet, pPlayer);
			break;
		}

		case en_PACKET_CS_GAME_REQ_PLAYER_RESTART:
		{
			PacketProc_PlayerRestart(packet, pPlayer);
			break;
		}
		// 잘못된 메시지를 받은 경우
		default:
		{
			// 세션의 연결을 끊는다. 연결을 끊으면 더이상 세션이 detach되어 더이상 메시지가 처리되지 않음
			DisconnectSession(pPlayer->GetSessionId());
			_disconnByInvalidMessageType++; // 모니터링

			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnRecv::DEFAULT. sessionId:%lld, accountNo:%lld, msg type:%d\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo(), msgType);
			break;
		}
		}
	}
	else
	{
		switch (msgType)
		{
		// 테스트 에코 요청
		case en_PACKET_CS_GAME_REQ_ECHO:
		{
			PacketProc_Echo(packet, pPlayer);
			break;
		}
		// 하트비트
		case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		{
			pPlayer->SetHeartBeatTime();
			break;
		}
		// 잘못된 메시지를 받은 경우
		default:
		{
			// 세션의 연결을 끊는다. 연결을 끊으면 더이상 세션이 detach되어 더이상 메시지가 처리되지 않음
			DisconnectSession(pPlayer->GetSessionId());
			_disconnByInvalidMessageType++; // 모니터링

			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnRecv::DEFAULT. sessionId:%lld, accountNo:%lld, msg type:%d\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo(), msgType);
			break;
		}
		}
	}

}

// 스레드에 최초접속 세션이 들어왔을 때 호출됨
void CGameContentsField::OnInitialSessionJoin(__int64 sessionId)
{
	LOGGING(LOGGING_LEVEL_ERROR, L"CGameContentsField::OnInitialSessionJoin. Invalid function call. sessionId:%lld\n", sessionId);
}

// 스레드에 플레이어가 들어왔을 때 호출됨(다른 컨텐츠로부터 이동해옴)
void CGameContentsField::OnPlayerJoin(__int64 sessionId, CPlayer_t pPlayer)
{
	PROFILE_BEGIN("CGameContentsField::OnPlayerJoin");
	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnSessionJoin::MoveJoin. sessionId:%lld, accountNo:%lld\n", pPlayer->GetSessionId(), pPlayer->GetAccountNo());

	// 플레이어를 map에 등록한다.
	InsertPlayerToMap(sessionId, pPlayer);

	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();

	// 만약 게임에 처음 접속한 유저일 경우 플레이어 데이터를 초기화하고 DB에 입력한다.
	if (character.state.bNewPlayer == true)
	{
		int tileX = FIELD_PLAYER_RESPAWN_TILE_X;
		int tileY = FIELD_PLAYER_RESPAWN_TILE_Y;
		pPlayer->LoadCharacterInfo(character.characterType, dfFIELD_TILE_TO_POS(tileX), dfFIELD_TILE_TO_POS(tileY), tileX, tileY, 0, 0, 5000, 0, 1, 0);

		// 플레이어 정보 DB에 저장
		DB_InsertPlayerInfo(pPlayer);
	}

	// DB에 로그인 로그 저장
	DB_Login(pPlayer);

	// 플레이어가 죽어있을 경우 위치를 리스폰위치로 지정한다.
	if (character.die == 1)
	{
		int tileX = FIELD_PLAYER_RESPAWN_TILE_X;
		int tileY = FIELD_PLAYER_RESPAWN_TILE_Y;
		pPlayer->SetRespawn(tileX, tileY);
		DB_PlayerRespawn(pPlayer);
	}

	// 플레이어를 섹터와 타일에 등록하고, 캐릭터를 생성하고, 주변 오브젝트들을 로드한다.
	InitializePlayerEnvironment(pPlayer);

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnSessionJoin. player join. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);
}



// 스레드 내에서 세션의 연결이 끊겼을때 호출됨
void CGameContentsField::OnSessionDisconnected(__int64 sessionId)
{
	PROFILE_BEGIN("CGameContentsField::OnSessionDisconnected");

	// 플레이어 객체 얻기
	CPlayer_t pPlayer = GetPlayerBySessionId(sessionId);
	if (pPlayer == nullptr)
	{
		LOGGING(LOGGING_LEVEL_ERROR, L"CGameContentsField::OnSessionDisconnected. No player in the map. sessionId:%lld\n", sessionId);
		return;
	}
		
	auto& pos = pPlayer->GetPosition();
	pPlayer->SetDead();

	// 플레이어를 섹터와 타일에서 제거하고 주변에 캐릭터 삭제 패킷을 보냄
	_tile.RemoveObject(pos.tileX, pos.tileY, ETileObjectType::PLAYER, pPlayer);
	_sector.RemoveObject(pos.sectorX, pos.sectorY, ESectorObjectType::PLAYER, pPlayer);

	CPacket& packetDeleteCharacter = AllocPacket();
	Msg_RemoveObject(packetDeleteCharacter, pPlayer);
	SendAroundSector(pos.sectorX, pos.sectorY, packetDeleteCharacter, pPlayer);
	packetDeleteCharacter.SubUseCount();


	// 현재 플레이어 정보를 DB에 저장
	DB_Logout(pPlayer);


	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnSessionDisconnected. player leave. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);

	// 플레이어를 map에서 삭제하고 free
	ErasePlayerFromMap(sessionId);
}

void CGameContentsField::OnError(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_ERROR, szError, vaList);
	va_end(vaList);
}



/* 플레이어 */

// 플레이어를 타일과 섹터에 등록하고, 캐릭터를 생성하고, 주변 오브젝트들을 로드한다. 플레이어 접속, 리스폰 시에 호출한다.
void CGameContentsField::InitializePlayerEnvironment(CPlayer_t& pPlayer)
{

	// 플레이어를 타일과 섹터에 등록한다.
	auto& posPlayer = pPlayer->GetPosition();
	_tile.AddObject(posPlayer.tileX, posPlayer.tileY, ETileObjectType::PLAYER, pPlayer);
	_sector.AddObject(posPlayer.sectorX, posPlayer.sectorY, ESectorObjectType::PLAYER, pPlayer);
	

	// 내 캐릭터 생성 패킷 전송
	CPacket& packetCreateMyChr = AllocPacket();
	Msg_CreateMyCharacter(packetCreateMyChr, pPlayer);
	SendUnicast(pPlayer, packetCreateMyChr);
	packetCreateMyChr.SubUseCount();

	// 내 주변의 오브젝트 생성 패킷 전송
	int minY = max(posPlayer.sectorY - _sectorViewRange, 0);
	int maxY = min(posPlayer.sectorY + _sectorViewRange, dfFIELD_SECTOR_MAX_Y - 1);
	int minX = max(posPlayer.sectorX - _sectorViewRange, 0);
	int maxX = min(posPlayer.sectorX + _sectorViewRange, dfFIELD_SECTOR_MAX_X - 1);
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayerOther = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
				if (pPlayerOther == pPlayer)
					continue;

				// 주변의 다른 캐릭터 생성 패킷 전송
				CPacket& packetCreateOtherChr = AllocPacket();
				Msg_CreateOtherCharacter(packetCreateOtherChr, pPlayerOther);
				SendUnicast(pPlayer, packetCreateOtherChr);
				packetCreateOtherChr.SubUseCount();

				// 다른 캐릭터가 이동중일경우 이동 패킷 전송
				if (pPlayerOther->IsMove() == true)
				{
					CPacket& packetMoveOtherChr = AllocPacket();
					Msg_MoveCharacter(packetMoveOtherChr, pPlayerOther);
					SendUnicast(pPlayer, packetMoveOtherChr);
					packetMoveOtherChr.SubUseCount();
				}
			}

			auto& vecMonster = _sector.GetObjectVector(x, y, ESectorObjectType::MONSTER);
			for (int i = 0; i < vecMonster.size(); i++)
			{
				CMonster_t pMonster = std::static_pointer_cast<CMonster>(vecMonster[i]);
				if (pMonster->IsAlive() == false)
					continue;

				// 나에게 몬스터 생성 패킷 전송
				CPacket& packetCreateMonster = AllocPacket();
				Msg_CreateMonsterCharacter(packetCreateMonster, pMonster, 0);
				SendUnicast(pPlayer, packetCreateMonster);
				packetCreateMonster.SubUseCount();

				// 몬스터가 움직이고 있을 경우 나에게 몬스터 이동 패킷 전송
				if (pMonster->IsMove())
				{
					CPacket& packetMoveMonster = AllocPacket();
					Msg_MoveMonster(packetMoveMonster, pMonster);
					SendUnicast(pPlayer, packetMoveMonster);
					packetMoveMonster.SubUseCount();
				}
			}

			auto& vecCrystal = _sector.GetObjectVector(x, y, ESectorObjectType::CRYSTAL);
			for (int i = 0; i < vecCrystal.size(); i++)
			{
				CCrystal_t pCrystal = std::static_pointer_cast<CCrystal>(vecCrystal[i]);

				// 나에게 크리스탈 생성 패킷 전송
				CPacket& packetCreateCrystal = AllocPacket();
				Msg_CreateCrystal(packetCreateCrystal, pCrystal);
				SendUnicast(pPlayer, packetCreateCrystal);
				packetCreateCrystal.SubUseCount();
			}
		}
	}

	// 내 캐릭터를 주변 플레이어들에게 전송
	CPacket& packetCreateMyChrToOther = AllocPacket();
	Msg_CreateOtherCharacter(packetCreateMyChrToOther, pPlayer);
	SendAroundSector(posPlayer.sectorX, posPlayer.sectorY, packetCreateMyChrToOther, pPlayer);
	packetCreateMyChrToOther.SubUseCount();
}




// 플레이어를 이전 타일, 섹터에서 현재 타일, 섹터로 옮긴다.
void CGameContentsField::MovePlayerTileAndSector(CPlayer_t& pPlayer)
{
	// 타일이 달라졌으면 타일 이동
	auto& pos = pPlayer->GetPosition();
	if (pos.tileY != pos.tileY_old || pos.tileX != pos.tileX_old)
	{
		_tile.RemoveObject(pos.tileX_old, pos.tileY_old, ETileObjectType::PLAYER, pPlayer);
		_tile.AddObject(pos.tileX, pos.tileY, ETileObjectType::PLAYER, pPlayer);
	}

	// 섹터가 같다면 더이상 할일은 없다
	if (pos.sectorY == pos.sectorY_old && pos.sectorX == pos.sectorX_old)
	{
		return;
	}

	PROFILE_BEGIN("CGameContentsField::MovePlayerTileAndSector");

	// 섹터 이동
	_sector.RemoveObject(pos.sectorX_old, pos.sectorY_old, ESectorObjectType::PLAYER, pPlayer);
	_sector.AddObject(pos.sectorX, pos.sectorY, ESectorObjectType::PLAYER, pPlayer);

	// 이전 섹터 기준의 X범위, Y범위 구하기
	int oldXLeft = max(pos.sectorX_old - _sectorViewRange, 0);
	int oldXRight = min(pos.sectorX_old + _sectorViewRange, _sector.GetMaxX() - 1);
	int oldYDown = max(pos.sectorY_old - _sectorViewRange, 0);
	int oldYUp = min(pos.sectorY_old + _sectorViewRange, _sector.GetMaxY() - 1);

	// 현재 섹터 기준의 X범위, Y범위 구하기
	int newXLeft = max(pos.sectorX - _sectorViewRange, 0);
	int newXRight = min(pos.sectorX + _sectorViewRange, _sector.GetMaxX() - 1);
	int newYDown = max(pos.sectorY - _sectorViewRange, 0);
	int newYUp = min(pos.sectorY + _sectorViewRange, _sector.GetMaxY() - 1);

	// 범위에 새로 포함된 섹터에 대한 처리
	CPacket& packetCreateMyChr = AllocPacket();
	Msg_CreateOtherCharacter(packetCreateMyChr, pPlayer);

	for (int y = newYDown; y <= newYUp; y++)
	{
		for (int x = newXLeft; x <= newXRight; x++)
		{
			if (y <= oldYUp && y >= oldYDown && x <= oldXRight && x >= oldXLeft)
				continue;

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayerOther = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
				if (pPlayerOther == pPlayer)
					continue;

				// 내 캐릭터를 상대에게 전송
				SendUnicast(pPlayerOther, packetCreateMyChr);

				// 상대 캐릭터를 나에게 전송
				CPacket& packetCreateOtherChr = AllocPacket();
				Msg_CreateOtherCharacter(packetCreateOtherChr, pPlayerOther);
				SendUnicast(pPlayer, packetCreateOtherChr);
				packetCreateOtherChr.SubUseCount();

				// 상대 캐릭터가 움직이고 있을 경우 나에게 캐릭터 이동 패킷 전송
				if (pPlayerOther->IsMove())
				{
					CPacket& packetMoveOtherChr = AllocPacket();
					Msg_MoveCharacter(packetMoveOtherChr, pPlayerOther);
					SendUnicast(pPlayer, packetMoveOtherChr);
					packetMoveOtherChr.SubUseCount();
				}
			}

			auto& vecMonster = _sector.GetObjectVector(x, y, ESectorObjectType::MONSTER);
			for (int i = 0; i < vecMonster.size(); i++)
			{
				CMonster_t pMonster = std::static_pointer_cast<CMonster>(vecMonster[i]);
				if (pMonster->IsAlive() == false)
					continue;

				// 나에게 몬스터 생성 패킷 전송
				CPacket& packetCreateMonster = AllocPacket();
				Msg_CreateMonsterCharacter(packetCreateMonster, pMonster, 0);
				SendUnicast(pPlayer, packetCreateMonster);
				packetCreateMonster.SubUseCount();

				// 몬스터가 움직이고 있을 경우 나에게 몬스터 이동 패킷 전송
				if (pMonster->IsMove())
				{
					CPacket& packetMoveMonster = AllocPacket();
					Msg_MoveMonster(packetMoveMonster, pMonster);
					SendUnicast(pPlayer, packetMoveMonster);
					packetMoveMonster.SubUseCount();
				}
			}

			auto& vecCrystal = _sector.GetObjectVector(x, y, ESectorObjectType::CRYSTAL);
			for (int i = 0; i < vecCrystal.size(); i++)
			{
				CCrystal_t pCrystal = std::static_pointer_cast<CCrystal>(vecCrystal[i]);

				// 나에게 크리스탈 생성 패킷 전송
				CPacket& packetCreateCrystal = AllocPacket();
				Msg_CreateCrystal(packetCreateCrystal, pCrystal);
				SendUnicast(pPlayer, packetCreateCrystal);
				packetCreateCrystal.SubUseCount();
			}

		}
	}
	packetCreateMyChr.SubUseCount();





	// 범위에서 제외된 타일에 대한 처리
	CPacket& packetDeleteMyChr = AllocPacket();
	Msg_RemoveObject(packetDeleteMyChr, pPlayer);
	for (int y = oldYDown; y <= oldYUp; y++)
	{
		for (int x = oldXLeft; x <= oldXRight; x++)
		{
			if (y <= newYUp && y >= newYDown && x <= newXRight && x >= newXLeft)
				continue;

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayerOther = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
				
				// 내 캐릭터 삭제패킷을 상대에게 전송
				SendUnicast(pPlayerOther, packetDeleteMyChr);

				// 상대 캐릭터 삭제패킷을 나에게 전송
				CPacket& packetDeleteOtherChr = AllocPacket();
				Msg_RemoveObject(packetDeleteOtherChr, pPlayerOther);
				SendUnicast(pPlayer, packetDeleteOtherChr);
				packetDeleteOtherChr.SubUseCount();
			}

			auto& vecMonster = _sector.GetObjectVector(x, y, ESectorObjectType::MONSTER);
			for (int i = 0; i < vecMonster.size(); i++)
			{
				CMonster_t pMonster = std::static_pointer_cast<CMonster>(vecMonster[i]);
				if (pMonster->IsAlive() == false)
					continue;

				// 나에게 몬스터 삭제 패킷 전송
				CPacket& packetDeleteMonster = AllocPacket();
				Msg_RemoveObject(packetDeleteMonster, pMonster);
				SendUnicast(pPlayer, packetDeleteMonster);
				packetDeleteMonster.SubUseCount();
			}

			auto& vecCrystal = _sector.GetObjectVector(x, y, ESectorObjectType::CRYSTAL);
			for (int i = 0; i < vecCrystal.size(); i++)
			{
				CCrystal_t pCrystal = std::static_pointer_cast<CCrystal>(vecCrystal[i]);

				// 나에게 크리스탈 삭제 패킷 전송
				CPacket& packetDeleteCrystal = AllocPacket();
				Msg_RemoveObject(packetDeleteCrystal, pCrystal);
				SendUnicast(pPlayer, packetDeleteCrystal);
				packetDeleteCrystal.SubUseCount();
			}
		}
	}
	packetDeleteMyChr.SubUseCount();
}








/* 몬스터 */

// 몬스터를 이전 타일, 섹터에서 현재 타일, 섹터로 옮긴다.
void CGameContentsField::MoveMonsterTileAndSector(CMonster_t& pMonster)
{
	// 타일이 다르다면 타일이동
	auto& pos = pMonster->GetPosition();
	if (pos.tileX_old != pos.tileX || pos.tileY_old != pos.tileY)
	{
		_tile.RemoveObject(pos.tileX_old, pos.tileY_old, ETileObjectType::MONSTER, pMonster);
		_tile.AddObject(pos.tileX, pos.tileY, ETileObjectType::MONSTER, pMonster);
	}

	// 섹터가 같다면 더이상 할일은 없음
	if (pos.sectorX_old == pos.sectorX && pos.sectorY_old == pos.sectorY)
	{
		return;
	}

	PROFILE_BEGIN("CGameContentsField::MoveMonsterTileAndSector");

	// 섹터 이동
	_sector.RemoveObject(pos.sectorX_old, pos.sectorY_old, ESectorObjectType::MONSTER, pMonster);
	_sector.AddObject(pos.sectorX, pos.sectorY, ESectorObjectType::MONSTER, pMonster);

	// 이전 섹터 기준의 X범위, Y범위 구하기
	int oldXLeft = max(pos.sectorX_old - _sectorViewRange, 0);
	int oldXRight = min(pos.sectorX_old + _sectorViewRange, _sector.GetMaxX() - 1);
	int oldYDown = max(pos.sectorY_old - _sectorViewRange, 0);
	int oldYUp = min(pos.sectorY_old + _sectorViewRange, _sector.GetMaxY() - 1);

	// 현재 섹터 기준의 X범위, Y범위 구하기
	int newXLeft = max(pos.sectorX - _sectorViewRange, 0);
	int newXRight = min(pos.sectorX + _sectorViewRange, _sector.GetMaxX() - 1);
	int newYDown = max(pos.sectorY - _sectorViewRange, 0);
	int newYUp = min(pos.sectorY + _sectorViewRange, _sector.GetMaxY() - 1);


	// 범위에 새로 포함된 섹터에 대한 처리
	CPacket& packetCreateMonster = AllocPacket();
	Msg_CreateMonsterCharacter(packetCreateMonster, pMonster, 0);
	for (int y = newYDown; y <= newYUp; y++)
	{
		for (int x = newXLeft; x <= newXRight; x++)
		{
			if (y <= oldYUp && y >= oldYDown && x <= oldXRight && x >= oldXLeft)
				continue;

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayerOther = std::static_pointer_cast<CPlayer>(vecPlayer[i]);

				// 플레이어에게 몬스터 생성 패킷 전송
				SendUnicast(pPlayerOther, packetCreateMonster);
			}
		}
	}
	packetCreateMonster.SubUseCount();


	// 범위에서 제외된 타일에 대한 처리
	CPacket& packetDeleteMonster = AllocPacket();
	Msg_RemoveObject(packetDeleteMonster, pMonster);
	for (int y = oldYDown; y <= oldYUp; y++)
	{
		for (int x = oldXLeft; x <= oldXRight; x++)
		{
			if (y <= newYUp && y >= newYDown && x <= newXRight && x >= newXLeft)
				continue;

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayerOther = std::static_pointer_cast<CPlayer>(vecPlayer[i]);

				// 몬스터 삭제 패킷을 플레이어에게 전송
				SendUnicast(pPlayerOther, packetDeleteMonster);
			}
		}
	}
	packetDeleteMonster.SubUseCount();
}


// 몬스터가 공격함
void CGameContentsField::MonsterAttack(CMonster_t& pMonster)
{
	if (pMonster->IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::MonsterAttack");

	
	// 주변에 몬스터 이동 패킷을 전송하여 방향을 바꾼다. 그리고 주변에 공격 패킷 전송
	auto& posMon = pMonster->GetPosition();
	auto& statMon = pMonster->GetStatus();
	CPacket& packetMoveMonster = AllocPacket();    // 이동 패킷
	Msg_MoveMonster(packetMoveMonster, pMonster);
	SendAroundSector(posMon.sectorX, posMon.sectorY, packetMoveMonster, nullptr);
	packetMoveMonster.SubUseCount();

	CPacket& packetMonsterAttack = AllocPacket();  // 공격 패킷
	Msg_MonsterAttack(packetMonsterAttack, pMonster);
	SendAroundSector(posMon.sectorX, posMon.sectorY, packetMonsterAttack, nullptr);
	packetMonsterAttack.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::MonsterAttack. monster attack. id:%lld, tile:(%d, %d), pos:(%.3f, %.3f), rotation:%d\n"
		, pMonster->GetClientId(), posMon.tileX, posMon.tileY, posMon.x, posMon.y, statMon.rotation);

	float monsterPosX = posMon.x;
	float monsterPosY = posMon.y;
	int degree = (360 - statMon.rotation + 90) % 360;  // rotation을 360도 각도로 변환

	// 점을 0도 방향으로 회전시키기 위한 cos, sin값
	float R[2] = { _math.GetCosine(360 - degree), _math.GetSine(360 - degree) };


	// 공격 범위에 해당하는 타일 조사하기
	// 공격범위 타일: 몬스터 타일 기준 상하좌우 +2칸
	// 공격범위 좌표: 몬스터가 바라보는 방향으로 지름 _attackRange 인 반원
	int minY = max(posMon.tileY - 2, 0);
	int maxY = min(posMon.tileY + 2, dfFIELD_TILE_MAX_Y - 1);
	int minX = max(posMon.tileX - 2, 0);
	int maxX = min(posMon.tileX + 2, dfFIELD_TILE_MAX_X - 1);
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			auto& vecPlayer = _tile.GetObjectVector(x, y, ETileObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayerOther = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
				if (pPlayerOther->IsAlive() == false)
					continue;

				// 몬스터 위치를 원점으로 했을 때, 플레이어의 좌표를 원점으로 옮기고 0도 방향으로 회전시킴
				auto& posPlayer = pPlayerOther->GetPosition();
				float playerPosX = (posPlayer.x - monsterPosX) * R[0] - (posPlayer.y - monsterPosY) * R[1];
				float playerPosY = (posPlayer.x - monsterPosX) * R[1] + (posPlayer.y - monsterPosY) * R[0];

				// 플레이어의 좌표가 가로 pMonster->_attackRange 세로 pMonster->_attackRange * 2 사각형 안이면 hit
				if (playerPosX > 0 && playerPosX < statMon.attackRange && playerPosY > -statMon.attackRange && playerPosY < statMon.attackRange)
				{
					pPlayerOther->Hit(statMon.damage);

					// 대미지 패킷 전송
					CPacket& packetDamage = AllocPacket();
					Msg_Damage(packetDamage, pMonster, pPlayerOther, statMon.damage);
					SendAroundSector(posPlayer.sectorX, posPlayer.sectorY, packetDamage, nullptr);
					packetDamage.SubUseCount();

					LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::MonsterAttack. hit player. sessionId:%lld, accountNo:%lld, target tile:(%d, %d), target pos:(%.3f, %.3f)\n"
						, pPlayerOther->GetSessionId(), pPlayerOther->GetAccountNo(), posPlayer.tileX, posPlayer.tileY, posPlayer.x, posPlayer.y);
				}
			}

		}
	}
}

// 몬스터의 타겟을 찾음
void CGameContentsField::MonsterFindTarget(CMonster_t& pMonster)
{
	PROFILE_BEGIN("CGameContentsField::MonsterFindTarget");

	auto& statMon = pMonster->GetStatus();
	auto& posMon = pMonster->GetPosition();

	int viewSectorRange = dfFIELD_POS_TO_SECTOR(statMon.viewRange) + 1;
	int initialSectorX = dfFIELD_POS_TO_SECTOR(posMon.x_initial);
	int initialSectorY = dfFIELD_POS_TO_SECTOR(posMon.y_initial);

	int minX = max(posMon.sectorX - viewSectorRange, 0);
	int maxX = min(posMon.sectorX + viewSectorRange, _sector.GetMaxX() - 1);
	int minY = max(posMon.sectorY - viewSectorRange, 0);
	int maxY = min(posMon.sectorY + viewSectorRange, _sector.GetMaxY() - 1);

	// 섹터는 시야 범위의 랜덤한 위치부터 시작하여 찾도록 한다.
	int midX = minX + rand() % (maxX - minX + 1);
	int midY = minY + rand() % (maxY - minY + 1);
	
	for (int y = midY; y <= maxY; y++)
	{
		for (int x = midX; x <= maxX; x++)
		{
			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
				if (pPlayer->IsAlive() == false)
					continue;

				// 주변에 플레이어가 있다면 타겟으로 지정하도록 한다.
				pMonster->SetTargetPlayer(pPlayer);
			}
		}
		for (int x = minX; x < midX; x++)
		{
			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
				if (pPlayer->IsAlive() == false)
					continue;

				// 주변에 플레이어가 있다면 타겟으로 지정하도록 한다.
				pMonster->SetTargetPlayer(pPlayer);
			}
		}
	}
	for (int y = minY; y < midY; y++)
	{
		for (int x = midX; x <= maxX; x++)
		{
			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
				if (pPlayer->IsAlive() == false)
					continue;

				// 주변에 플레이어가 있다면 타겟으로 지정하도록 한다.
				pMonster->SetTargetPlayer(pPlayer);
			}
		}
		for (int x = minX; x < midX; x++)
		{
			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
				if (pPlayer->IsAlive() == false)
					continue;

				// 주변에 플레이어가 있다면 타겟으로 지정하도록 한다.
				pMonster->SetTargetPlayer(pPlayer);
			}
		}
	}
}





/* 플레이어에게 패킷 전송 */

// 한명에게 보내기
int CGameContentsField::SendUnicast(CPlayer_t& pPlayer, CPacket& packet)
{

	//packet.AddUseCount();
	//pPlayer->_vecPacketBuffer.push_back(packet);
	SendPacket(pPlayer->GetSessionId(), packet);  // 임시

	return 1;
}






// 주변 섹터 전체에 보내기
int CGameContentsField::SendAroundSector(int sectorX, int sectorY, CPacket& packet, const CPlayer_t& except)
{
	PROFILE_BEGIN("CGameContentsField::SendAroundSector");

	auto& vecAroundSector = _sector.GetAroundSector(sectorX, sectorY);

	int sendCount = 0;
	for (int i = 0; i < vecAroundSector.size(); i++)
	{
		auto& vecPlayer = vecAroundSector[i]->GetObjectVector(ESectorObjectType::PLAYER);
		for (int i = 0; i < vecPlayer.size(); i++)
		{
			CPlayer_t pPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
			if (pPlayer == except)
				continue;

			//packet.AddUseCount();
			//pPlayer->_vecPacketBuffer.push_back(packet);
			SendPacket(pPlayer->GetSessionId(), packet);  // 임시
			sendCount++;
		}
	}

	return sendCount;
}

int CGameContentsField::SendAroundSector(int sectorX, int sectorY, CPacket& packet, const CPlayer_t&& except)
{
	PROFILE_BEGIN("CGameContentsField::SendAroundSector");

	auto& vecAroundSector = _sector.GetAroundSector(sectorX, sectorY);

	int sendCount = 0;
	for (int i = 0; i < vecAroundSector.size(); i++)
	{
		auto& vecPlayer = vecAroundSector[i]->GetObjectVector(ESectorObjectType::PLAYER);
		for (int i = 0; i < vecPlayer.size(); i++)
		{
			CPlayer_t pPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
			if (pPlayer == except)
				continue;

			//packet.AddUseCount();
			//pPlayer->_vecPacketBuffer.push_back(packet);
			SendPacket(pPlayer->GetSessionId(), packet);  // 임시
			sendCount++;
		}
	}

	return sendCount;
}



// 2개 섹터의 주변 섹터 전체에 보내기
int CGameContentsField::SendTwoAroundSector(int sectorX1, int sectorY1, int sectorX2, int sectorY2, CPacket& packet, const CPlayer_t& except)
{
	PROFILE_BEGIN("CGameContentsField::SendTwoAroundSector");

	if (sectorX1 == sectorX2 && sectorY1 == sectorY2)
	{
		return SendAroundSector(sectorX1, sectorY1, packet, except);
	}

	int leftX1 = max(sectorX1 - _sectorViewRange, 0);
	int rightX1 = min(sectorX1 + _sectorViewRange, _sector.GetMaxX() - 1);
	int downY1 = max(sectorY1 - _sectorViewRange, 0);
	int upY1 = min(sectorY1 + _sectorViewRange, _sector.GetMaxY() - 1);
	int leftX2 = max(sectorX2 - _sectorViewRange, 0);
	int rightX2 = min(sectorX2 + _sectorViewRange, _sector.GetMaxX() - 1);
	int downY2 = max(sectorY2 - _sectorViewRange, 0);
	int upY2 = min(sectorY2 + _sectorViewRange, _sector.GetMaxY() - 1);

	int minX = min(leftX1, rightX1);
	int maxX = max(rightX1, rightX2);
	int minY = min(downY1, downY2);
	int maxY = max(upY1, upY2);

	int sendCount = 0;
	// y 축은 2개 타일 범위 중 작은 값부터 큰 값까지, x 축도 2개 타일 범위 중 작은 값부터 큰 값까지 조사한다.
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			// 만약 (x,y)가 sector1 범위에도 속하지 않고 sector2 범위에도 속하지 않는다면 넘어감
			if (!(y >= downY1 && y <= upY1 && x >= leftX1 && x <= rightX1)
				&& !(y >= downY2 && y <= upY2 && x >= leftX2 && x <= rightX2))
			{
				continue;
			}

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
				if (pPlayer == except)
					continue;

				//packet.AddUseCount();
				//pPlayer->_vecPacketBuffer.push_back(packet);
				SendPacket(pPlayer->GetSessionId(), packet); // 임시
				sendCount++;
			}
		}
	}

	return sendCount;
}


int CGameContentsField::SendTwoAroundSector(int sectorX1, int sectorY1, int sectorX2, int sectorY2, CPacket& packet, const CPlayer_t&& except)
{
	PROFILE_BEGIN("CGameContentsField::SendTwoAroundSector");

	if (sectorX1 == sectorX2 && sectorY1 == sectorY2)
	{
		return SendAroundSector(sectorX1, sectorY1, packet, except);
	}

	int leftX1 = max(sectorX1 - _sectorViewRange, 0);
	int rightX1 = min(sectorX1 + _sectorViewRange, _sector.GetMaxX() - 1);
	int downY1 = max(sectorY1 - _sectorViewRange, 0);
	int upY1 = min(sectorY1 + _sectorViewRange, _sector.GetMaxY() - 1);
	int leftX2 = max(sectorX2 - _sectorViewRange, 0);
	int rightX2 = min(sectorX2 + _sectorViewRange, _sector.GetMaxX() - 1);
	int downY2 = max(sectorY2 - _sectorViewRange, 0);
	int upY2 = min(sectorY2 + _sectorViewRange, _sector.GetMaxY() - 1);

	int minX = min(leftX1, rightX1);
	int maxX = max(rightX1, rightX2);
	int minY = min(downY1, downY2);
	int maxY = max(upY1, upY2);

	int sendCount = 0;
	// y 축은 2개 타일 범위 중 작은 값부터 큰 값까지, x 축도 2개 타일 범위 중 작은 값부터 큰 값까지 조사한다.
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			// 만약 (x,y)가 sector1 범위에도 속하지 않고 sector2 범위에도 속하지 않는다면 넘어감
			if (!(y >= downY1 && y <= upY1 && x >= leftX1 && x <= rightX1)
				&& !(y >= downY2 && y <= upY2 && x >= leftX2 && x <= rightX2))
			{
				continue;
			}

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer_t pPlayer = std::static_pointer_cast<CPlayer>(vecPlayer[i]);
				if (pPlayer == except)
					continue;

				//packet.AddUseCount();
				//pPlayer->_vecPacketBuffer.push_back(packet);
				SendPacket(pPlayer->GetSessionId(), packet); // 임시
				sendCount++;
			}
		}
	}

	return sendCount;
}




/* 패킷 처리 */

// 캐릭터 이동 요청 처리
void CGameContentsField::PacketProc_MoveCharacter(CPacket& packet, CPlayer_t& pPlayer)
{
	if (pPlayer->IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_MoveCharacter");

	INT64 clientId;
	float posX;
	float posY;
	USHORT rotation;
	BYTE VKey;
	BYTE HKey;
	packet >> clientId >> posX >> posY >> rotation >> VKey >> HKey;
	
	pPlayer->MovePosition(posX, posY, rotation, VKey, HKey);

	// 앉았다 일어났을 때 플레이어 HP 보정
	if (pPlayer->IsSit())
	{
		pPlayer->SetSit(false);
		CPacket& packetPlayerHp = AllocPacket();
		Msg_PlayerHP(packetPlayerHp, pPlayer);
		SendUnicast(pPlayer, packetPlayerHp);
		packetPlayerHp.SubUseCount();

		// DB에 저장 요청
		DB_PlayerRenewHP(pPlayer);
	}


	// 타일 및 섹터 이동
	auto& pos = pPlayer->GetPosition();
	auto& character = pPlayer->GetCharacter();
	if (pos.tileY != pos.tileY_old || pos.tileX != pos.tileX_old)
	{
		MovePlayerTileAndSector(pPlayer);
	}


	// 내 주변에 캐릭터 이동 패킷 전송
	CPacket& packetMoveChr = AllocPacket();
	Msg_MoveCharacter(packetMoveChr, pPlayer);
	SendAroundSector(pos.sectorX, pos.sectorY, packetMoveChr, pPlayer);
	packetMoveChr.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_MoveCharacter. move player. sessionId:%lld, accountNo:%lld, tile (%d, %d) to (%d, %d), pos (%.3f, %.3f) to (%.3f, %.3f), rotation:%d\n"
		, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), pos.tileX_old, pos.tileY_old, pos.tileX, pos.tileY, pos.x_old, pos.y_old, pos.x, pos.y, character.rotation);
}




// 캐릭터 정지 요청 처리
void CGameContentsField::PacketProc_StopCharacter(CPacket& packet, CPlayer_t& pPlayer)
{
	if (pPlayer->IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_StopCharacter");

	INT64 clientId;
	float x;
	float y;
	USHORT rotation;
	packet >> clientId >> x >> y >> rotation;

	pPlayer->MovePosition(x, y, rotation, 0, 0);

	// 앉았다 일어났을 때 플레이어 HP 보정
	if (pPlayer->IsSit())
	{
		pPlayer->SetSit(false);
		CPacket& packetPlayerHp = AllocPacket();
		Msg_PlayerHP(packetPlayerHp, pPlayer);
		SendUnicast(pPlayer, packetPlayerHp);
		packetPlayerHp.SubUseCount();

		// DB에 저장 요청
		DB_PlayerRenewHP(pPlayer);
	}


	// 타일 좌표가 달라지면 타일 및 섹터 이동
	auto& pos = pPlayer->GetPosition();
	auto& character = pPlayer->GetCharacter();
	if (pos.tileY != pos.tileY_old || pos.tileX != pos.tileX_old)
	{
		MovePlayerTileAndSector(pPlayer);
	}


	// 내 주변에 캐릭터 정지 패킷 전송
	CPacket& packetStopChr = AllocPacket();
	Msg_StopCharacter(packetStopChr, pPlayer);
	SendAroundSector(pos.sectorX, pos.sectorY, packetStopChr, pPlayer);
	packetStopChr.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_StopCharacter. stop player. sessionId:%lld, accountNo:%lld, tile (%d, %d) to (%d, %d), pos (%.3f, %.3f) to (%.3f, %.3f), rotation:%d\n"
		, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), pos.tileX_old, pos.tileY_old, pos.tileX, pos.tileY, pos.x_old, pos.y_old, pos.x, pos.y, character.rotation);
}



// 줍기 요청
void CGameContentsField::PacketProc_Pick(CPacket& packet, CPlayer_t& pPlayer)
{
	if (pPlayer->IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_Pick");

	INT64 clientId;
	packet >> clientId;

	auto& pos = pPlayer->GetPosition();
	auto& character = pPlayer->GetCharacter();

	// 주변 섹터에 먹기 액션 전송
	CPacket& packetPickAction = AllocPacket();
	Msg_Pick(packetPickAction, pPlayer);
	SendAroundSector(pos.sectorX, pos.sectorY, packetPickAction, nullptr);
	packetPickAction.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Pick. pick. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);

	// 주변 타일에 있는 모든 크리스탈 획득
	int minY = max(pos.tileY - _config.tilePickRange, 0);
	int maxY = min(pos.tileY + _config.tilePickRange, dfFIELD_TILE_MAX_Y - 1);
	int minX = max(pos.tileX - _config.tilePickRange, 0);
	int maxX = min(pos.tileX + _config.tilePickRange, dfFIELD_TILE_MAX_X - 1);
	int pickAmount = 0;
	CCrystal_t pFirstCrystal = nullptr;
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			auto& vecCrystal = _tile.GetObjectVector(x, y, ETileObjectType::CRYSTAL);
			for (int i = 0; i < vecCrystal.size(); i++)
			{
				CCrystal_t pCrystal = std::static_pointer_cast<CCrystal>(vecCrystal[i]);
				auto& posCrystal = pCrystal->GetPosition();
				auto& statCrystal = pCrystal->GetStatus();

				if (pCrystal->IsAlive() == false)
					continue;

				if (pFirstCrystal == nullptr)
					pFirstCrystal = pCrystal;

				pPlayer->AddCrystal(statCrystal.amount);
				pickAmount += statCrystal.amount;

				// 주변에 크리스탈 삭제 패킷 전송
				CPacket& packetDeleteCrystal = AllocPacket();
				Msg_RemoveObject(packetDeleteCrystal, pCrystal);
				SendAroundSector(posCrystal.sectorX, posCrystal.sectorY, packetDeleteCrystal, nullptr);
				packetDeleteCrystal.SubUseCount();

				// 크리스탈 삭제처리
				pCrystal->SetDead();

				LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Pick. get crystal. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f), crystal:%d (+%d)\n"
					, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y, character.crystal, statCrystal.amount);

			}
		}
	}

	if (pickAmount > 0)
	{
		// 주변에 크리스탈 획득 패킷 전송
		CPacket& packetPickCrystal = AllocPacket();
		Msg_PickCrystal(packetPickCrystal, pPlayer, pFirstCrystal);
		SendAroundSector(pos.sectorX, pos.sectorY, packetPickCrystal, nullptr);
		packetPickCrystal.SubUseCount();

		// DB에 저장
		DB_PlayerGetCrystal(pPlayer);
	}

}

// 캐릭터 공격 요청 처리1
void CGameContentsField::PacketProc_Attack1(CPacket& packet, CPlayer_t& pPlayer)
{
	if (pPlayer->IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_Attack1");

	INT64 clientId;
	packet >> clientId;

	auto& pos = pPlayer->GetPosition();
	auto& character = pPlayer->GetCharacter();

	// 주변에 공격 패킷 전송
	CPacket& packetAttack = AllocPacket();
	Msg_Attack1(packetAttack, pPlayer);
	SendAroundSector(pos.sectorX, pos.sectorY, packetAttack, pPlayer);
	packetAttack.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Attack1. attack1. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);


	float playerPosX = pos.x;
	float playerPosY = pos.y;
	int degree = (360 - character.rotation + 90) % 360;  // rotation을 360도 각도로 변환

	// 점을 0도 방향으로 회전시키기 위한 cos, sin값
	float R[2] = { _math.GetCosine(360 - degree), _math.GetSine(360 - degree) };


	// 공격범위를 조사할 타일: 내 위치 기준 상하좌우 2개 타일
	// 공격범위: 내가 바라보는 방향으로 너비 0.5뒤~1.0앞, 높이 1.5의 사각형
	int minTileX = max(pos.tileX - 2, 0);
	int maxTileX = min(pos.tileX + 2, dfFIELD_TILE_MAX_X - 1);
	int minTileY = max(pos.tileY - 2, 0);
	int maxTileY = min(pos.tileY + 2, dfFIELD_TILE_MAX_Y - 1);
	for (int y = minTileY; y <= maxTileY; y++)
	{
		for (int x = minTileX; x <= maxTileX; x++)
		{
			auto& vecMonster = _tile.GetObjectVector(x, y, ETileObjectType::MONSTER);
			for (int i = 0; i < vecMonster.size(); i++)
			{
				CMonster_t pMonster = std::static_pointer_cast<CMonster>(vecMonster[i]);
				if (pMonster->IsAlive() == false)
					continue;

				// 플레이어 위치를 원점으로 했을 때, 몬스터의 좌표를 원점으로 옮기고 0도 방향으로 회전시킴
				auto& posMon = pMonster->GetPosition();
				float monsterPosX = (posMon.x - playerPosX) * R[0] - (posMon.y - playerPosY) * R[1];
				float monsterPosY = (posMon.x - playerPosX) * R[1] + (posMon.y - playerPosY) * R[0];

				// 몬스터의 좌표가 가로 1.5 세로 1.5 사각형 안이면 hit
				if (monsterPosX > -0.5 && monsterPosX < 1.0 && monsterPosY > -0.75 && monsterPosY < 0.75)
				{
					pMonster->Hit(character.damageAttack1);

					// 대미지 패킷 전송
					CPacket& packetDamage = AllocPacket();
					Msg_Damage(packetDamage, pPlayer, pMonster, character.damageAttack1);
					SendTwoAroundSector(pos.sectorX, pos.sectorY, posMon.sectorX, posMon.sectorY, packetDamage, nullptr);
					packetDamage.SubUseCount();

					LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Attack1. hit monster. sessionId:%lld, accountNo:%lld, target tile:(%d, %d), target pos:(%.3f, %.3f)\n"
						, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), posMon.tileX, posMon.tileY, posMon.x, posMon.y);
				}
			}

		}
	}
}




// 캐릭터 공격 요청 처리2
void CGameContentsField::PacketProc_Attack2(CPacket& packet, CPlayer_t& pPlayer)
{
	if (pPlayer->IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_Attack2");

	INT64 clientId;
	packet >> clientId;

	auto& pos = pPlayer->GetPosition();
	auto& character = pPlayer->GetCharacter();

	// 주변에 공격 패킷 전송
	CPacket& packetAttack = AllocPacket();
	Msg_Attack2(packetAttack, pPlayer);
	SendAroundSector(pos.sectorX, pos.sectorY, packetAttack, pPlayer);
	packetAttack.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Attack2. attack2. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);


	// 내가 바라보는 방향으로 0.5뒤, 1.5앞의 좌표를 찾음
	float playerPosX = pos.x;
	float playerPosY = pos.y;
	int degree = (360 - character.rotation + 90) % 360;  // rotation을 360도 각도로 변환
	float tailPosX = _math.GetCosine(degree + 180) * 0.5f + playerPosX;  // 1.5 위치뒤 x
	float tailPosY = _math.GetSine(degree + 180) * 0.5f + playerPosY;    // 1.5 위치뒤 y
	float headPosX = _math.GetCosine(degree) * 1.5f + playerPosX;        // 2.5 위치앞 x
	float headPosY = _math.GetSine(degree) * 1.5f + playerPosY;          // 2.5 위치앞 y

	// 점을 0도 방향으로 회전시키기 위한 cos, sin값
	float R[2] = { _math.GetCosine(360 - degree), _math.GetSine(360 - degree) };

	// 공격 범위에 해당하는 타일 찾기
	// 공격범위를 조사할 타일: 내가 바라보는 방향으로 0.5뒤 ~ 1.5앞 위치를 포함하는 타일
	// 공격범위: 내가 바라보는 방향으로 너비 0.5뒤~1.5앞, 높이 1.0의 사각형
	int minTileX = max(dfFIELD_POS_TO_TILE(min(tailPosX, headPosX)) - 1, 0);
	int maxTileX = min(dfFIELD_POS_TO_TILE(max(tailPosX, headPosX)) + 2, dfFIELD_TILE_MAX_X - 1);
	int minTileY = max(dfFIELD_POS_TO_TILE(min(tailPosY, headPosY)) - 1, 0);
	int maxTileY = min(dfFIELD_POS_TO_TILE(max(tailPosY, headPosY)) + 2, dfFIELD_TILE_MAX_Y - 1);

	for (int y = minTileY; y <= maxTileY; y++)
	{
		for (int x = minTileX; x <= maxTileX; x++)
		{
			auto& vecMonster = _tile.GetObjectVector(x, y, ETileObjectType::MONSTER);
			for (int i = 0; i < vecMonster.size(); i++)
			{
				CMonster_t pMonster = std::static_pointer_cast<CMonster>(vecMonster[i]);
				if (pMonster->IsAlive() == false)
					continue;

				// 플레이어 위치를 원점으로 했을 때, 몬스터의 좌표를 원점으로 옮기고 0도 방향으로 회전시킴
				auto& posMon = pMonster->GetPosition();
				float monsterPosX = (posMon.x - playerPosX) * R[0] - (posMon.y - playerPosY) * R[1];
				float monsterPosY = (posMon.x - playerPosX) * R[1] + (posMon.y - playerPosY) * R[0];

				// 몬스터의 좌표가 가로 2.0 세로 1.0 사각형 안이면 hit
				if (monsterPosX > -0.5 && monsterPosX < 1.5 && monsterPosY > -0.6 && monsterPosY < 0.6)
				{
					pMonster->Hit(character.damageAttack2);

					// 대미지 패킷 전송
					CPacket& packetDamage = AllocPacket();
					Msg_Damage(packetDamage, pPlayer, pMonster, character.damageAttack2);
					SendTwoAroundSector(pos.sectorX, pos.sectorY, posMon.sectorX, posMon.sectorY, packetDamage, nullptr);
					packetDamage.SubUseCount();

					LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Attack2. hit monster. sessionId:%lld, accountNo:%lld, target tile:(%d, %d), target pos:(%.3f, %.3f)\n"
						, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), posMon.tileX, posMon.tileY, posMon.x, posMon.y);
				}
			}

		}
	}
}




// 바닥에 앉기 요청 처리
void CGameContentsField::PacketProc_Sit(CPacket& packet, CPlayer_t& pPlayer)
{
	if (pPlayer->IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_Sit");

	INT64 clientId;
	packet >> clientId;

	// 플레이어 앉기 처리
	pPlayer->SetSit(true);

	// 주변 타일에 앉기 전송
	auto& pos = pPlayer->GetPosition();
	CPacket& packetSit = AllocPacket();
	Msg_Sit(packetSit, pPlayer);
	SendAroundSector(pos.sectorX, pos.sectorY, packetSit, pPlayer);
	packetSit.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Sit. sit. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);
}


// 플레이어 죽은 후 다시하기 요청
void CGameContentsField::PacketProc_PlayerRestart(CPacket& packet, CPlayer_t& pPlayer)
{
	if (pPlayer->IsAlive() == true)  // 사망하지 않았다면 무시
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_PlayerRestart");

	// 플레이어를 타일과 섹터에서 제거하고 주변에 캐릭터 삭제 패킷을 보낸다.
	auto& pos = pPlayer->GetPosition();
	_tile.RemoveObject(pos.tileX, pos.tileY, ETileObjectType::PLAYER, pPlayer);
	_sector.RemoveObject(pos.sectorX, pos.sectorY, ESectorObjectType::PLAYER, pPlayer);
	CPacket& packetDeleteCharacter = AllocPacket();
	Msg_RemoveObject(packetDeleteCharacter, pPlayer);
	SendAroundSector(pos.sectorX, pos.sectorY, packetDeleteCharacter, nullptr);
	packetDeleteCharacter.SubUseCount();

	// 리스폰 세팅
	pPlayer->SetRespawn(FIELD_PLAYER_RESPAWN_TILE_X, FIELD_PLAYER_RESPAWN_TILE_Y);
	DB_PlayerRespawn(pPlayer);

	// 다시하기 패킷 전송
	CPacket& packetPlayerRestart = AllocPacket();
	Msg_PlayerRestart(packetPlayerRestart);
	SendUnicast(pPlayer, packetPlayerRestart);
	packetPlayerRestart.SubUseCount();

	// 플레이어를 타일과 섹터에 등록하고, 캐릭터를 생성하고, 주변 오브젝트들을 로드한다.
	InitializePlayerEnvironment(pPlayer);

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_PlayerRestart. respawn player. sessionId:%lld, accountNo:%lld, tile:(%d, %d) to (%d, %d)\n"
		, pPlayer->GetSessionId(), pPlayer->GetAccountNo(), pos.tileX_old, pos.tileY_old, pos.tileX, pos.tileY);

}


// 테스트 에코 요청 처리
void CGameContentsField::PacketProc_Echo(CPacket& packet, CPlayer_t& pPlayer)
{
	INT64 accountNo;
	LONGLONG sendTick;
	packet >> accountNo >> sendTick;

	CPacket& packetEcho = AllocPacket();
	Msg_Echo(packetEcho, pPlayer, sendTick);
	SendUnicast(pPlayer, packetEcho);
	packetEcho.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Echo. echo. sessionId:%lld, accountNo:%lld\n"
		, pPlayer->GetSessionId(), pPlayer->GetAccountNo());
}




/* 패킷에 데이터 입력 */

// 내 캐릭터 생성 패킷
void CGameContentsField::Msg_CreateMyCharacter(CPacket& packet, const CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	auto& account = pPlayer->GetAccountInfo();
	packet << (WORD)en_PACKET_CS_GAME_RES_CREATE_MY_CHARACTER << pPlayer->GetClientId() << character.characterType;
	packet.PutData((const char*)account.szNick, sizeof(account.szNick));
	packet << pos.x << pos.y << character.rotation << character.crystal << character.hp << character.exp << character.level;
}

// 다른 캐릭터 생성 패킷
void CGameContentsField::Msg_CreateOtherCharacter(CPacket& packet, const CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	auto& account = pPlayer->GetAccountInfo();
	packet << (WORD)en_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER << pPlayer->GetClientId() << character.characterType;
	packet.PutData((const char*)account.szNick, sizeof(account.szNick));
	packet << pos.x << pos.y << character.rotation << character.level << character.respawn << character.die << character.sit;
}

// 몬스터 생성 패킷
void CGameContentsField::Msg_CreateMonsterCharacter(CPacket& packet, const CMonster_t& pMonster, BYTE respawn)
{
	auto& status = pMonster->GetStatus();
	auto& pos = pMonster->GetPosition();
	packet << (WORD)en_PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER << pMonster->GetClientId() << pos.x << pos.y << status.rotation << respawn;
}

// 캐릭터, 오브젝트 삭제 패킷
void CGameContentsField::Msg_RemoveObject(CPacket& packet, const CObject_t& pObject)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_REMOVE_OBJECT << pObject->GetClientId();
}

// 캐릭터 이동 패킷
void CGameContentsField::Msg_MoveCharacter(CPacket& packet, const CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	packet << (WORD)en_PACKET_CS_GAME_RES_MOVE_CHARACTER << pPlayer->GetClientId() << pos.x << pos.y << character.rotation << character.VKey << character.HKey;
}

// 캐릭터 정지 패킷
void CGameContentsField::Msg_StopCharacter(CPacket& packet, const CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	packet << (WORD)en_PACKET_CS_GAME_RES_STOP_CHARACTER << pPlayer->GetClientId() << pos.x << pos.y << character.rotation;
}

// 몬스터 이동 패킷
void CGameContentsField::Msg_MoveMonster(CPacket& packet, const CMonster_t& pMonster)
{
	auto& stat = pMonster->GetStatus();
	auto& pos = pMonster->GetPosition();
	packet << (WORD)en_PACKET_CS_GAME_RES_MOVE_MONSTER << pMonster->GetClientId() << pos.x << pos.y << stat.rotation;
}

// 캐릭터 공격1 패킷
void CGameContentsField::Msg_Attack1(CPacket& packet, const CPlayer_t& pPlayer)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_ATTACK1 << pPlayer->GetClientId();
}

// 캐릭터 공격1 패킷
void CGameContentsField::Msg_Attack2(CPacket& packet, const CPlayer_t& pPlayer)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_ATTACK2 << pPlayer->GetClientId();
}

// 몬스터 공격 패킷
void CGameContentsField::Msg_MonsterAttack(CPacket& packet, const CMonster_t& pMonster)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_MONSTER_ATTACK << pMonster->GetClientId();
}

// 공격대상에게 대미지를 먹임 패킷
void CGameContentsField::Msg_Damage(CPacket& packet, const CObject_t& pAttackObj, const CObject_t& pTargetObj, int damage)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_DAMAGE << pAttackObj->GetClientId() << pTargetObj->GetClientId() << damage;
}

// 몬스터 사망 패킷
void CGameContentsField::Msg_MonsterDie(CPacket& packet, const CMonster_t& pMonster)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_MONSTER_DIE << pMonster->GetClientId();
}

// 크리스탈 생성 패킷
void CGameContentsField::Msg_CreateCrystal(CPacket& packet, const CCrystal_t& pCrystal)
{
	auto& stat = pCrystal->GetStatus();
	auto& pos = pCrystal->GetPosition();
	packet << (WORD)en_PACKET_CS_GAME_RES_CREATE_CRISTAL << pCrystal->GetClientId() << stat.crystalType << pos.posX << pos.posY;
}

// 크리스탈 먹기 요청
void CGameContentsField::Msg_Pick(CPacket& packet, const CPlayer_t& pPlayer)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_PICK << pPlayer->GetClientId();
}

// 바닥에 앉기 패킷
void CGameContentsField::Msg_Sit(CPacket& packet, const CPlayer_t& pPlayer)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_SIT << pPlayer->GetClientId();
}

// 크리스탈 획득 패킷
void CGameContentsField::Msg_PickCrystal(CPacket& packet, const CPlayer_t& pPlayer, const CCrystal_t& pCrystal)
{
	auto& character = pPlayer->GetCharacter();
	packet << (WORD)en_PACKET_CS_GAME_RES_PICK_CRISTAL << pPlayer->GetClientId() << pCrystal->GetClientId() << character.crystal;
}

// 플레이어 HP 보정 패킷
void CGameContentsField::Msg_PlayerHP(CPacket& packet, const CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	packet << (WORD)en_PACKET_CS_GAME_RES_PLAYER_HP << character.hp;
}

// 플레이어 죽음 패킷
void CGameContentsField::Msg_PlayerDie(CPacket& packet, const CPlayer_t& pPlayer, int minusCrystal)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_PLAYER_DIE << pPlayer->GetClientId() << minusCrystal;
}

// 플레이어 죽은 후 다시하기 패킷
void CGameContentsField::Msg_PlayerRestart(CPacket& packet)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_PLAYER_RESTART;
}

// 테스트 에코 패킷
void CGameContentsField::Msg_Echo(CPacket& packet, const CPlayer_t& pPlayer, LONGLONG sendTick)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_ECHO << pPlayer->GetAccountNo() << sendTick;
}


/* DB에 데이터 저장 */
void CGameContentsField::DB_Login(CPlayer_t& pPlayer)
{
	auto& account = pPlayer->GetAccountInfo();
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	std::wstring strIP(account.sIP);
	strIP += L":";
	strIP += std::to_wstring(account.port);
	_pDBConn->PostQueryRequest(
		L"INSERT INTO `logdb`.`gamelog` (`type`, `code`, `accountno`, `servername`, `param1`, `param2`, `param3`, `param4`, `message`)"
		" VALUES (%d, %d, %lld, '%s', %d, %d, %d, %d, '%s');"
		, 1, 11, pPlayer->GetAccountNo(), L"Game", pos.tileX, pos.tileY, character.crystal, character.hp, strIP.c_str()
	);
}

void CGameContentsField::DB_Logout(CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	_pDBConn->PostQueryRequest(
		L"START TRANSACTION;"
		" UPDATE `gamedb`.`character`"
		" SET `posx`=%.3f, `posy`=%.3f, `tilex`=%d, `tiley`=%d, `rotation`=%d, `crystal`=%d, `hp`=%d, `exp`=%d, `level`=%d, `die`=%d"
		" WHERE `accountno`=%lld;"
		" "
		" INSERT INTO `logdb`.`gamelog` (`type`, `code`, `accountno`, `servername`, `param1`, `param2`, `param3`, `param4`)"
		" VALUES (%d, %d, %lld, '%s', %d, %d, %d, %d);"
		" COMMIT;"
		, pos.x, pos.y, pos.tileX, pos.tileY, character.rotation, character.crystal, character.hp, character.exp, character.level, character.die
		, pPlayer->GetAccountNo()
		, 1, 12, pPlayer->GetAccountNo(), L"Game", pos.tileX, pos.tileY, character.crystal, character.hp
	);
	pPlayer->SetDBUpdateTime();
}

void CGameContentsField::DB_PlayerDie(CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	_pDBConn->PostQueryRequest(
		L"START TRANSACTION;"
		" UPDATE `gamedb`.`character`"
		" SET `posx`=%.3f, `posy`=%.3f, `tilex`=%d, `tiley`=%d, `rotation`=%d, `crystal`=%d, `hp`=%d, `exp`=%d, `level`=%d, `die`=%d"
		" WHERE `accountno`=%lld;"
		" "
		" INSERT INTO `logdb`.`gamelog` (`type`, `code`, `accountno`, `servername`, `param1`, `param2`, `param3`)"
		" VALUES (%d, %d, %lld, '%s', %d, %d, %d);"
		" COMMIT;"
		, pos.x, pos.y, pos.tileX, pos.tileY, character.rotation, character.crystal, character.hp, character.exp, character.level, character.die
		, pPlayer->GetAccountNo()
		, 3, 31, pPlayer->GetAccountNo(), L"Game", pos.tileX, pos.tileY, character.crystal
	);
	pPlayer->SetDBUpdateTime();
}

void CGameContentsField::DB_PlayerRespawn(CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	_pDBConn->PostQueryRequest(
		L"START TRANSACTION;"
		" UPDATE `gamedb`.`character`"
		" SET `posx`=%.3f, `posy`=%.3f, `tilex`=%d, `tiley`=%d, `rotation`=%d, `crystal`=%d, `hp`=%d, `exp`=%d, `level`=%d, `die`=%d"
		" WHERE `accountno`=%lld;"
		" "
		" INSERT INTO `logdb`.`gamelog` (`type`, `code`, `accountno`, `servername`, `param1`, `param2`, `param3`)"
		" VALUES (%d, %d, %lld, '%s', %d, %d, %d);"
		" COMMIT;"
		, pos.x, pos.y, pos.tileX, pos.tileY, character.rotation, character.crystal, character.hp, character.exp, character.level, character.die
		, pPlayer->GetAccountNo()
		, 3, 31, pPlayer->GetAccountNo(), L"Game", pos.tileX, pos.tileY, character.crystal
	);
	pPlayer->SetDBUpdateTime();
}

void CGameContentsField::DB_PlayerGetCrystal(CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	_pDBConn->PostQueryRequest(
		L"START TRANSACTION;"
		" UPDATE `gamedb`.`character`"
		" SET `posx`=%.3f, `posy`=%.3f, `tilex`=%d, `tiley`=%d, `rotation`=%d, `crystal`=%d, `hp`=%d, `exp`=%d, `level`=%d"
		" WHERE `accountno`=%lld;"
		" "
		" INSERT INTO `logdb`.`gamelog` (`type`, `code`, `accountno`, `servername`, `param1`, `param2`)"
		" VALUES (%d, %d, %lld, '%s', %d, %d);"
		" COMMIT;"
		, pos.x, pos.y, pos.tileX, pos.tileY, character.rotation, character.crystal, character.hp, character.exp, character.level
		, pPlayer->GetAccountNo()
		, 4, 41, pPlayer->GetAccountNo(), L"Game", character.crystal - character.oldCrystal, character.crystal
	);
	pPlayer->SetDBUpdateTime();
}

void CGameContentsField::DB_PlayerRenewHP(CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	_pDBConn->PostQueryRequest(
		L"START TRANSACTION;"
		" UPDATE `gamedb`.`character`"
		" SET `posx`=%.3f, `posy`=%.3f, `tilex`=%d, `tiley`=%d, `rotation`=%d, `crystal`=%d, `hp`=%d, `exp`=%d, `level`=%d"
		" WHERE `accountno`=%lld;"
		" "
		" INSERT INTO `logdb`.`gamelog` (`type`, `code`, `accountno`, `servername`, `param1`, `param2`, `param3`)"
		" VALUES (%d, %d, %lld, '%s', %d, %d, %d);"
		" COMMIT;"
		, pos.x, pos.y, pos.tileX, pos.tileY, character.rotation, character.crystal, character.hp, character.exp, character.level
		, pPlayer->GetAccountNo()
		, 5, 51, pPlayer->GetAccountNo(), L"Game", character.sitStartHp, character.hp, (int)((GetTickCount64() - character.sitStartTime) / 1000)
	);
	pPlayer->SetDBUpdateTime();
}



void CGameContentsField::DB_InsertPlayerInfo(CPlayer_t& pPlayer)
{
	auto& character = pPlayer->GetCharacter();
	auto& pos = pPlayer->GetPosition();
	_pDBConn->PostQueryRequest(
		L"START TRANSACTION;"
		" INSERT INTO `gamedb`.`character` (`accountno`, `charactertype`, `posx`, `posy`, `tilex`, `tiley`, `rotation`, `crystal`, `hp`, `exp`, `level`, `die`)"
		" VALUES(%lld, %d, %.3f, %.3f, %d, %d, %d, %d, %d, %lld, %d, %d);"
		" "
		" INSERT INTO `logdb`.`gamelog` (`type`, `code`, `accountno`, `servername`, `param1`)"
		" VALUES (%d, %d, %lld, '%s', %d);"
		" COMMIT;"
		, pPlayer->GetAccountNo(), character.characterType, pos.x, pos.y, pos.tileX, pos.tileY, character.rotation, character.crystal, character.hp, character.exp, character.level, character.die
		, 3, 32, pPlayer->GetAccountNo(), L"Game", character.characterType
	);
	pPlayer->SetDBUpdateTime();
}





/* utils */
void CGameContentsField::ReadAreaInfoFromConfigJson(const CJsonParser& configJson, int areaNumber)
{
	// json 파일의 "Area#" 요소에서 데이터를 읽어서 _vecAreaInfo 에 입력한다.
	std::wstring strArea = L"Area";
	strArea += std::to_wstring(areaNumber + 1);

	CJsonObject& configArea = configJson[L"ContentsField"][strArea.c_str()];
	Config::AreaInfo areaInfo;
	areaInfo.x = configArea[L"X"].Int();
	areaInfo.y = configArea[L"Y"].Int();
	areaInfo.range = configArea[L"Range"].Int();
	areaInfo.normalMonsterNumber = configArea[L"NormalMonster"][L"Number"].Int();
	areaInfo.normalMonsterHP = configArea[L"NormalMonster"][L"HP"].Int();
	areaInfo.normalMonsterDamage = configArea[L"NormalMonster"][L"Damage"].Int();
	areaInfo.bossMonsterNumber = configArea[L"BossMonster"][L"Number"].Int();
	areaInfo.bossMonsterHP = configArea[L"BossMonster"][L"HP"].Int();
	areaInfo.bossMonsterDamage = configArea[L"BossMonster"][L"Damage"].Int();

	_config.vecAreaInfo.push_back(areaInfo);
}










