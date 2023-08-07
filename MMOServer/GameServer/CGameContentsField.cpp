#include "CGameServer.h"
#include "CGameContents.h"
#include "CommonProtocol.h"

#include "../utils/CJsonParser.h"
#include "../utils/CDBAsyncWriter.h"
#include "../utils/logger.h"
//#define ENABLE_PROFILER
#include "../utils/profiler.h"

#include "CGameContentsField.h"
#include <psapi.h>


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
	// config ���� json parser ���
	const CJsonParser& jsonParser = GetConfig().GetJsonParser();

	// _config.vecAreaInfo �� area ������ �Է��Ѵ�.
	_config.numArea = jsonParser[L"ContentsField"][L"NumOfArea"].Int();
	for (int i = 0; i < _config.numArea; i++)
	{
		ReadAreaInfoFromConfigJson(jsonParser, i);
	}

	// config ���� �б�
	_config.numInitialCrystal = jsonParser[L"ContentsField"][L"InitialCrystalNumber"].Int();      // �� ���� �ʱ� ũ����Ż ��

	// ���� ��� ����
	_config.numRandomCrystalGeneration = 1;        // �ʵ忡 �ʴ� ���������Ǵ� ũ����Ż ��
	_config.maxRandomCrystalGeneration = 1000;     // �ʵ忡 ���������Ǵ� ũ����Ż�� �ִ� ��
	_config.tilePickRange = 2;                     // �ݱ� Ÿ�� ����
	_config.crystalLossWhenDie = 1000;             // �׾����� ũ����Ż �Ҵ� ��


	_maxPosX = dfFIELD_POS_MAX_X;
	_maxPosY = dfFIELD_POS_MAX_Y;
	_numTileXPerSector = dfFIELD_NUM_TILE_X_PER_SECTOR;
	_sectorViewRange = dfFIELD_SECTOR_VIEW_RANGE;
	_tileViewRange = _sectorViewRange * _numTileXPerSector + _numTileXPerSector / 2;
	


	// DB ����
	_pDBConn = std::make_unique<CDBAsyncWriter>();
	_pDBConn->ConnectAndRunThread("127.0.0.1", "root", "vmfhzkepal!", "gamedb", 3306);


	// ���͸� �����ϰ� ���Ϳ� Ÿ�Ͽ� �Է��Ѵ�
	for (int area = 0; area < _config.numArea; area++)
	{
		Config::AreaInfo& areaInfo = _config.vecAreaInfo[area];
		// �Ϲ� ���� ����
		for (int i = 0; i < areaInfo.normalMonsterNumber; i++)
		{
			CMonster& monster = AllocMonster();
			monster.Init(EMonsterType::NORMAL, area
				, FIELD_RANDOM_TILE_X(areaInfo.x, areaInfo.range)
				, FIELD_RANDOM_TILE_Y(areaInfo.y, areaInfo.range)
				, areaInfo.normalMonsterHP
				, areaInfo.normalMonsterDamage
			);

			_vecAllMonster.push_back(&monster);
			auto& pos = monster.GetPosition();
			_tile.AddObject(pos.tileX, pos.tileY, ETileObjectType::MONSTER, monster);
			_sector.AddObject(pos.sectorX, pos.sectorY, ESectorObjectType::MONSTER, monster);
		}
		// ���� ���� ����
		for (int i = 0; i < areaInfo.bossMonsterNumber; i++)
		{
			CMonster& monster = AllocMonster();
			monster.Init(EMonsterType::BOSS, area
				, FIELD_RANDOM_TILE_X(areaInfo.x, areaInfo.range / 2)
				, FIELD_RANDOM_TILE_Y(areaInfo.y, areaInfo.range / 2)
				, areaInfo.bossMonsterHP
				, areaInfo.bossMonsterDamage
			);

			_vecAllMonster.push_back(&monster);
			auto& pos = monster.GetPosition();
			_tile.AddObject(pos.tileX, pos.tileY, ETileObjectType::MONSTER, monster);
			_sector.AddObject(pos.sectorX, pos.sectorY, ESectorObjectType::MONSTER, monster);
		}
	}

	// �ʱ� ũ����Ż ����
	for(int i = 0; i < _config.numInitialCrystal; i++)
	{
		CCrystal& crystal = AllocCrystal();
		crystal.Init(FIELD_RANDOM_POS_X, FIELD_RANDOM_POS_Y, rand() % 3 + 1);
		auto& pos = crystal.GetPosition();
		_tile.AddObject(pos.tileX, pos.tileY, ETileObjectType::CRYSTAL, crystal);
		_sector.AddObject(pos.sectorX, pos.sectorY, ESectorObjectType::CRYSTAL, crystal);
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






/* ������ Ŭ���� callback �Լ� ���� */

 // ������ ������Ʈ
void CGameContentsField::OnUpdate()
{
	// ��� ������Ʈ ������Ʈ
	ULONGLONG currentTime = GetTickCount64();
	int sectorMaxX = _sector.GetMaxX();
	int sectorMaxY = _sector.GetMaxY();
	for (int y = 0; y < sectorMaxY; y++)
	{
		for (int x = 0; x < sectorMaxX; x++)
		{
			// ũ����Ż ������Ʈ
			auto& vecCrystal = _sector.GetObjectVector(x, y, ESectorObjectType::CRYSTAL);
			for (int iCnt = 0; iCnt < vecCrystal.size();)
			{
				CCrystal& crystal = *static_cast<CCrystal*>(vecCrystal[iCnt]);
				crystal.Update();

				// ������ ũ����Ż�� Ÿ�ϰ� ���Ϳ��� ����
				if (crystal.IsAlive() == false)
				{
					auto& pos = crystal.GetPosition();
					_tile.RemoveObject(pos.tileX, pos.tileY, ETileObjectType::CRYSTAL, crystal);
					_sector.RemoveObject(x, y, ESectorObjectType::CRYSTAL, crystal);
					FreeCrystal(crystal);
					continue;
				}

				iCnt++;
			}

			// ���ͳ��� ���� ������Ʈ
			auto& vecMonster = _sector.GetObjectVector(x, y, ESectorObjectType::MONSTER);
			for (int iCnt = 0; iCnt < vecMonster.size();)
			{
				CMonster& monster = *static_cast<CMonster*>(vecMonster[iCnt]);
				auto& posMon = monster.GetPosition();
				auto& statMon = monster.GetStatus();
				monster.Update();

				// ���Ͱ� ������� ���
				if (monster.IsAlive() == true)
				{
					// Ÿ���� ���� ��� �ֺ����� ã��
					if (monster.IsHasTarget() == false)
					{
						MonsterFindTarget(monster);
					}

					// ���Ͱ� �������� ���
					if (monster.IsAttack() == true)
					{
						// ���� �������� ó��
						MonsterAttack(monster);
					}
				}
				// ���Ͱ� ����������
				else if (monster.IsAlive() == false)
				{
					// ���� ��� ��Ŷ ����
					CPacket& packetMonsterDie = AllocPacket();
					Msg_MonsterDie(packetMonsterDie, monster);
					SendAroundSector(posMon.sectorX, posMon.sectorY, packetMonsterDie, nullptr);
					packetMonsterDie.SubUseCount();
					
					// ũ����Ż ����
					CCrystal& crystal = AllocCrystal();
					crystal.Init(posMon.x, posMon.y, rand() % 3);
					auto& posCrystal = crystal.GetPosition();
					_tile.AddObject(posCrystal.tileX, posCrystal.tileY, ETileObjectType::CRYSTAL, crystal);
					_sector.AddObject(x, y, ESectorObjectType::CRYSTAL, crystal);

					// ũ����Ż ���� ��Ŷ ����
					CPacket& packetCreateCrystal = AllocPacket();
					Msg_CreateCrystal(packetCreateCrystal, crystal);
					SendAroundSector(posCrystal.sectorX, posCrystal.sectorY, packetCreateCrystal, nullptr);
					packetCreateCrystal.SubUseCount();

					// ���͸� Ÿ�ϰ� ���Ϳ��� ����
					_tile.RemoveObject(posMon.tileX, posMon.tileY, ETileObjectType::MONSTER, monster);
					_sector.RemoveObject(x, y, ESectorObjectType::MONSTER, monster);

					LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnUpdate. monster dead. clientId:%lld, tile:(%d, %d), hp:%d\n", monster.GetClientId(), posMon.tileX, posMon.tileY, statMon.hp);
					continue;
				}

				iCnt++;
			}

			// �÷��̾� ������Ʈ
			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int iCnt = 0; iCnt < vecPlayer.size(); iCnt++)
			{
				CPlayer& player = *static_cast<CPlayer*>(vecPlayer[iCnt]);
				auto& character = player.GetCharacter();
				auto& pos = player.GetPosition();

				player.Update();
				
				if (player.IsAlive())
				{
					// �÷��̾ ��� �Ͼ�� ��� HP ��Ŷ ����
					if (player.IsJustStandUp() == true)
					{
						CPacket& packetPlayerHp = AllocPacket();
						Msg_PlayerHP(packetPlayerHp, player);
						SendUnicast(player, packetPlayerHp);
						packetPlayerHp.SubUseCount();

						DB_PlayerRenewHP(player);
					}
				}

				// ��� �׾������
				if (player.IsJustDied() == true)
				{
					player.AddCrystal(-_config.crystalLossWhenDie);

					// �÷��̾� �����Ŷ ����
					CPacket& packetPlayerDie = AllocPacket();
					Msg_PlayerDie(packetPlayerDie, player, _config.crystalLossWhenDie);
					SendAroundSector(pos.sectorX, pos.sectorY, packetPlayerDie, nullptr);
					packetPlayerDie.SubUseCount();

					// DB�� ����
					DB_PlayerDie(player);

					LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnUpdate. player die. sessionId:%lld, accountNo:%lld, hp:%d\n", player.GetSessionId(), player.GetAccountNo(), character.hp);
				}
			}
		}
	}



	// Ÿ�Ͽ� ���� ���͸� �����Ͽ� ��ü ���� üũ (������ Ÿ���̵��� �ʿ��� ������ ���⼭ ó��)
	for (int iCnt = 0; iCnt < _vecAllMonster.size(); iCnt++)
	{
		CMonster& monster = *_vecAllMonster[iCnt];
		auto& statMon = monster.GetStatus();
		auto& posMon = monster.GetPosition();

		// ���Ͱ� �������
		if (monster.IsAlive() == true)
		{
			// ���Ͱ� �̵����� ���
			if (monster.IsMove() == true)
			{
				// Ÿ�� �� ���� �̵�
				MoveMonsterTileAndSector(monster);

				// ���� �̵� ��Ŷ ����
				CPacket& packetMoveMonster = AllocPacket();
				Msg_MoveMonster(packetMoveMonster, monster);
				SendAroundSector(posMon.sectorX, posMon.sectorY, packetMoveMonster, nullptr);
				packetMoveMonster.SubUseCount();
			}
		}
		// ���Ͱ� �׾�����
		else if (monster.IsAlive() == false)
		{
			// ��Ȱ �ð���ŭ ��������� ��Ȱ
			if (statMon.deadTime + statMon.respawnWaitTime < currentTime)
			{
				// ��Ȱ �� Ÿ�Ͽ� �Է�
				Config::AreaInfo& area = _config.vecAreaInfo[statMon.area];
				if (statMon.eMonsterType == EMonsterType::NORMAL)
				{
					monster.SetRespawn(FIELD_RANDOM_TILE_X(area.x, area.range)
						, FIELD_RANDOM_TILE_Y(area.y, area.range));
				}
				else
				{
					monster.SetRespawn(FIELD_RANDOM_TILE_X(area.x, area.range / 2)
						, FIELD_RANDOM_TILE_Y(area.y, area.range / 2));
				}

				_tile.AddObject(posMon.tileX, posMon.tileY, ETileObjectType::MONSTER, monster);
				_sector.AddObject(posMon.sectorX, posMon.sectorY, ESectorObjectType::MONSTER, monster);

				// ���� ������ ��Ŷ ����
				CPacket& packetRespawnMonster = AllocPacket();
				Msg_CreateMonsterCharacter(packetRespawnMonster, monster, 1);
				SendAroundSector(posMon.sectorX, posMon.sectorY, packetRespawnMonster, nullptr);
				packetRespawnMonster.SubUseCount();

				LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnUpdate. monster respawn. clientId:%lld, tile:(%d, %d)\n", monster.GetClientId(), posMon.tileX, posMon.tileY);
			}
		}
	}


	// �����
	for (int y = 0; y < _sector.GetMaxY(); y++)
	{
		for (int x = 0; x < _sector.GetMaxX(); x++)
		{
			{
				auto& vec = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
				for (int i = 0; i < vec.size(); i++)
				{
					if (vec[i]->__validation != 0x12345)
					{
						int* p = 0; *p = 0;
					}
				}
			}
			{
				auto& vec = _sector.GetObjectVector(x, y, ESectorObjectType::MONSTER);
				for (int i = 0; i < vec.size(); i++)
				{
					if (vec[i]->__validation != 0x12345)
					{
						int* p = 0; *p = 0;
					}
				}
			}
			{
				auto& vec = _sector.GetObjectVector(x, y, ESectorObjectType::CRYSTAL);
				for (int i = 0; i < vec.size(); i++)
				{
					if (vec[i]->__validation != 0x12345)
					{
						int* p = 0; *p = 0;
					}
				}
			}
		}
	}
	for (int y = 0; y < _tile.GetMaxY(); y++)
	{
		for (int x = 0; x < _tile.GetMaxX(); x++)
		{
			{
				auto& vec = _tile.GetObjectVector(x, y, ETileObjectType::PLAYER);
				for (int i = 0; i < vec.size(); i++)
				{
					if (vec[i]->__validation != 0x12345)
					{
						int* p = 0; *p = 0;
					}
				}
			}
			{
				auto& vec = _tile.GetObjectVector(x, y, ETileObjectType::MONSTER);
				for (int i = 0; i < vec.size(); i++)
				{
					if (vec[i]->__validation != 0x12345)
					{
						int* p = 0; *p = 0;
					}
				}
			}
			{
				auto& vec = _tile.GetObjectVector(x, y, ETileObjectType::CRYSTAL);
				for (int i = 0; i < vec.size(); i++)
				{
					if (vec[i]->__validation != 0x12345)
					{
						int* p = 0; *p = 0;
					}
				}
			}
		}
	}
}


// �������� �����ϴ� ������ �޽���ť�� �޽����� ������ �� ȣ���
void CGameContentsField::OnRecv(__int64 sessionId, CPacket& packet)
{
	// ����ID�� �÷��̾� ��ü�� ��´�.
	auto iter = _mapPlayer.find(sessionId);
	CPlayer& player = *iter->second;

	// �޽��� Ÿ�� Ȯ��
	WORD msgType;
	packet >> msgType;

	// �޽��� Ÿ�Կ� ���� ó��
	if (msgType < 2000)
	{
		switch (msgType)
		{
			// ĳ���� �̵� ��û
		case en_PACKET_CS_GAME_REQ_MOVE_CHARACTER:
		{
			PacketProc_MoveCharacter(packet, player);
			break;
		}

		// ĳ���� ���� ��û
		case en_PACKET_CS_GAME_REQ_STOP_CHARACTER:
		{
			PacketProc_StopCharacter(packet, player);
			break;
		}

		// ĳ���� ���� ��û ó��1
		case en_PACKET_CS_GAME_REQ_ATTACK1:
		{
			PacketProc_Attack1(packet, player);
			break;
		}

		// ĳ���� ���� ��û ó��2
		case en_PACKET_CS_GAME_REQ_ATTACK2:
		{
			PacketProc_Attack2(packet, player);
			break;
		}

		// �ݱ� ��û
		case en_PACKET_CS_GAME_REQ_PICK:
		{
			PacketProc_Pick(packet, player);
			break;
		}

		// �ٴڿ� �ɱ� ��û
		case en_PACKET_CS_GAME_REQ_SIT:
		{
			PacketProc_Sit(packet, player);
			break;
		}

		case en_PACKET_CS_GAME_REQ_PLAYER_RESTART:
		{
			PacketProc_PlayerRestart(packet, player);
			break;
		}
		// �߸��� �޽����� ���� ���
		default:
		{
			// ������ ������ ���´�. ������ ������ ���̻� ������ detach�Ǿ� ���̻� �޽����� ó������ ����
			DisconnectSession(player.GetSessionId());
			_disconnByInvalidMessageType++; // ����͸�

			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnRecv::DEFAULT. sessionId:%lld, accountNo:%lld, msg type:%d\n", player.GetSessionId(), player.GetAccountNo(), msgType);
			break;
		}
		}
	}
	else
	{
		switch (msgType)
		{
		// �׽�Ʈ ���� ��û
		case en_PACKET_CS_GAME_REQ_ECHO:
		{
			PacketProc_Echo(packet, player);
			break;
		}


		// ��Ʈ��Ʈ
		case en_PACKET_CS_GAME_REQ_HEARTBEAT:
		{
			player.SetHeartBeatTime();
			break;
		}
		// �߸��� �޽����� ���� ���
		default:
		{
			// ������ ������ ���´�. ������ ������ ���̻� ������ detach�Ǿ� ���̻� �޽����� ó������ ����
			DisconnectSession(player.GetSessionId());
			_disconnByInvalidMessageType++; // ����͸�

			LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnRecv::DEFAULT. sessionId:%lld, accountNo:%lld, msg type:%d\n", player.GetSessionId(), player.GetAccountNo(), msgType);
			break;
		}
		}
	}

}




// �����忡 ������ ������ �� ȣ���
void CGameContentsField::OnSessionJoin(__int64 sessionId, PVOID data)
{
	PROFILE_BEGIN("CGameContentsField::OnSessionJoin");

	// �Ķ���Ϳ��� �÷��̾� ��ü�� ��´�.
	CPlayer& player = *reinterpret_cast<CPlayer*>(data);
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnSessionJoin::MoveJoin. sessionId:%lld, accountNo:%lld\n", player.GetSessionId(), player.GetAccountNo());

	// �÷��̾ map�� ����Ѵ�.
	_mapPlayer.insert(std::make_pair(sessionId, &player));

	// ���� ���� ������ ������ ��� �÷��̾� �����͸� �ʱ�ȭ�ϰ� DB�� �Է��Ѵ�.
	if (character.state.bNewPlayer == true)
	{
		int tileX = FIELD_PLAYER_RESPAWN_TILE_X;
		int tileY = FIELD_PLAYER_RESPAWN_TILE_Y;
		player.LoadCharacterInfo(character.characterType, dfFIELD_TILE_TO_POS(tileX), dfFIELD_TILE_TO_POS(tileY), tileX, tileY, 0, 0, 5000, 0, 1, 0);

		// �÷��̾� ���� DB�� ����
		DB_InsertPlayerInfo(player);
	}

	// DB�� �α��� �α� ����
	DB_Login(player);

	// �÷��̾ �׾����� ��� ��ġ�� ��������ġ�� �����Ѵ�.
	if (character.die == 1)
	{
		int tileX = FIELD_PLAYER_RESPAWN_TILE_X;
		int tileY = FIELD_PLAYER_RESPAWN_TILE_Y;
		player.SetRespawn(tileX, tileY);
		DB_PlayerRespawn(player);
	}

	// �÷��̾ ���Ϳ� Ÿ�Ͽ� ����ϰ�, ĳ���͸� �����ϰ�, �ֺ� ������Ʈ���� �ε��Ѵ�.
	InitializePlayerEnvironment(player);

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnSessionJoin. player join. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, player.GetSessionId(), player.GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);
}


// ������ ������ ������ ������ �������� ȣ���
void CGameContentsField::OnSessionDisconnected(__int64 sessionId)
{
	PROFILE_BEGIN("CGameContentsField::OnSessionDisconnected");

	// �÷��̾� ��ü ���
	auto iter = _mapPlayer.find(sessionId);
	if (iter == _mapPlayer.end())
	{
		int* p = 0;
		*p = 0;
		return;
	}
		

	CPlayer& player = *iter->second;
	auto& pos = player.GetPosition();
	player.SetDead();

	// �÷��̾ ���Ϳ� Ÿ�Ͽ��� �����ϰ� �ֺ��� ĳ���� ���� ��Ŷ�� ����
	_tile.RemoveObject(pos.tileX, pos.tileY, ETileObjectType::PLAYER, player);
	_sector.RemoveObject(pos.sectorX, pos.sectorY, ESectorObjectType::PLAYER, player);

	CPacket& packetDeleteCharacter = AllocPacket();
	Msg_RemoveObject(packetDeleteCharacter, player);
	SendAroundSector(pos.sectorX, pos.sectorY, packetDeleteCharacter, &player);
	packetDeleteCharacter.SubUseCount();


	// ���� �÷��̾� ������ DB�� ����
	DB_Logout(player);


	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::OnSessionDisconnected. player leave. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, player.GetSessionId(), player.GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);

	// �÷��̾ map���� �����ϰ� free
	_mapPlayer.erase(iter);
	FreePlayer(player);
}

void CGameContentsField::OnError(const wchar_t* szError, ...)
{
	va_list vaList;
	va_start(vaList, szError);
	LOGGING_VALIST(LOGGING_LEVEL_ERROR, szError, vaList);
	va_end(vaList);
}



/* �÷��̾� */

// �÷��̾ Ÿ�ϰ� ���Ϳ� ����ϰ�, ĳ���͸� �����ϰ�, �ֺ� ������Ʈ���� �ε��Ѵ�. �÷��̾� ����, ������ �ÿ� ȣ���Ѵ�.
void CGameContentsField::InitializePlayerEnvironment(CPlayer& player)
{

	// �÷��̾ Ÿ�ϰ� ���Ϳ� ����Ѵ�.
	auto& posPlayer = player.GetPosition();
	_tile.AddObject(posPlayer.tileX, posPlayer.tileY, ETileObjectType::PLAYER, player);
	_sector.AddObject(posPlayer.sectorX, posPlayer.sectorY, ESectorObjectType::PLAYER, player);
	

	// �� ĳ���� ���� ��Ŷ ����
	CPacket& packetCreateMyChr = AllocPacket();
	Msg_CreateMyCharacter(packetCreateMyChr, player);
	SendUnicast(player, packetCreateMyChr);
	packetCreateMyChr.SubUseCount();

	// �� �ֺ��� ������Ʈ ���� ��Ŷ ����
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
				CPlayer& playerOther = *static_cast<CPlayer *>(vecPlayer[i]);
				if (&playerOther == &player)
					continue;

				// �ֺ��� �ٸ� ĳ���� ���� ��Ŷ ����
				CPacket& packetCreateOtherChr = AllocPacket();
				Msg_CreateOtherCharacter(packetCreateOtherChr, playerOther);
				SendUnicast(player, packetCreateOtherChr);
				packetCreateOtherChr.SubUseCount();

				// �ٸ� ĳ���Ͱ� �̵����ϰ�� �̵� ��Ŷ ����
				if (playerOther.IsMove() == true)
				{
					CPacket& packetMoveOtherChr = AllocPacket();
					Msg_MoveCharacter(packetMoveOtherChr, playerOther);
					SendUnicast(player, packetMoveOtherChr);
					packetMoveOtherChr.SubUseCount();
				}
			}

			auto& vecMonster = _sector.GetObjectVector(x, y, ESectorObjectType::MONSTER);
			for (int i = 0; i < vecMonster.size(); i++)
			{
				CMonster& monster = *static_cast<CMonster*>(vecMonster[i]);
				if (monster.IsAlive() == false)
					continue;

				// ������ ���� ���� ��Ŷ ����
				CPacket& packetCreateMonster = AllocPacket();
				Msg_CreateMonsterCharacter(packetCreateMonster, monster, 0);
				SendUnicast(player, packetCreateMonster);
				packetCreateMonster.SubUseCount();

				// ���Ͱ� �����̰� ���� ��� ������ ���� �̵� ��Ŷ ����
				if (monster.IsMove())
				{
					CPacket& packetMoveMonster = AllocPacket();
					Msg_MoveMonster(packetMoveMonster, monster);
					SendUnicast(player, packetMoveMonster);
					packetMoveMonster.SubUseCount();
				}
			}

			auto& vecCrystal = _sector.GetObjectVector(x, y, ESectorObjectType::CRYSTAL);
			for (int i = 0; i < vecCrystal.size(); i++)
			{
				CCrystal& crystal = *static_cast<CCrystal*>(vecCrystal[i]);

				// ������ ũ����Ż ���� ��Ŷ ����
				CPacket& packetCreateCrystal = AllocPacket();
				Msg_CreateCrystal(packetCreateCrystal, crystal);
				SendUnicast(player, packetCreateCrystal);
				packetCreateCrystal.SubUseCount();
			}
		}
	}

	// �� ĳ���͸� �ֺ� �÷��̾�鿡�� ����
	CPacket& packetCreateMyChrToOther = AllocPacket();
	Msg_CreateOtherCharacter(packetCreateMyChrToOther, player);
	SendAroundSector(posPlayer.sectorX, posPlayer.sectorY, packetCreateMyChrToOther, &player);
	packetCreateMyChrToOther.SubUseCount();
}




// �÷��̾ ���� Ÿ��, ���Ϳ��� ���� Ÿ��, ���ͷ� �ű��.
void CGameContentsField::MovePlayerTileAndSector(CPlayer& player)
{
	// Ÿ���� �޶������� Ÿ�� �̵�
	auto& pos = player.GetPosition();
	if (pos.tileY != pos.tileY_old || pos.tileX != pos.tileX_old)
	{
		_tile.RemoveObject(pos.tileX_old, pos.tileY_old, ETileObjectType::PLAYER, player);
		_tile.AddObject(pos.tileX, pos.tileY, ETileObjectType::PLAYER, player);
	}

	// ���Ͱ� ���ٸ� ���̻� ������ ����
	if (pos.sectorY == pos.sectorY_old && pos.sectorX == pos.sectorX_old)
	{
		return;
	}

	PROFILE_BEGIN("CGameContentsField::MovePlayerTileAndSector");

	// ���� �̵�
	_sector.RemoveObject(pos.sectorX_old, pos.sectorY_old, ESectorObjectType::PLAYER, player);
	_sector.AddObject(pos.sectorX, pos.sectorY, ESectorObjectType::PLAYER, player);

	// ���� ���� ������ X����, Y���� ���ϱ�
	int oldXLeft = max(pos.sectorX_old - _sectorViewRange, 0);
	int oldXRight = min(pos.sectorX_old + _sectorViewRange, _sector.GetMaxX() - 1);
	int oldYDown = max(pos.sectorY_old - _sectorViewRange, 0);
	int oldYUp = min(pos.sectorY_old + _sectorViewRange, _sector.GetMaxY() - 1);

	// ���� ���� ������ X����, Y���� ���ϱ�
	int newXLeft = max(pos.sectorX - _sectorViewRange, 0);
	int newXRight = min(pos.sectorX + _sectorViewRange, _sector.GetMaxX() - 1);
	int newYDown = max(pos.sectorY - _sectorViewRange, 0);
	int newYUp = min(pos.sectorY + _sectorViewRange, _sector.GetMaxY() - 1);

	// ������ ���� ���Ե� ���Ϳ� ���� ó��
	CPacket& packetCreateMyChr = AllocPacket();
	Msg_CreateOtherCharacter(packetCreateMyChr, player);

	for (int y = newYDown; y <= newYUp; y++)
	{
		for (int x = newXLeft; x <= newXRight; x++)
		{
			if (y <= oldYUp && y >= oldYDown && x <= oldXRight && x >= oldXLeft)
				continue;

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer& playerOther = *static_cast<CPlayer*>(vecPlayer[i]);
				if (&playerOther == &player)
					continue;

				// �� ĳ���͸� ��뿡�� ����
				SendUnicast(playerOther, packetCreateMyChr);

				// ��� ĳ���͸� ������ ����
				CPacket& packetCreateOtherChr = AllocPacket();
				Msg_CreateOtherCharacter(packetCreateOtherChr, playerOther);
				SendUnicast(player, packetCreateOtherChr);
				packetCreateOtherChr.SubUseCount();

				// ��� ĳ���Ͱ� �����̰� ���� ��� ������ ĳ���� �̵� ��Ŷ ����
				if (playerOther.IsMove())
				{
					CPacket& packetMoveOtherChr = AllocPacket();
					Msg_MoveCharacter(packetMoveOtherChr, playerOther);
					SendUnicast(player, packetMoveOtherChr);
					packetMoveOtherChr.SubUseCount();
				}
			}

			auto& vecMonster = _sector.GetObjectVector(x, y, ESectorObjectType::MONSTER);
			for (int i = 0; i < vecMonster.size(); i++)
			{
				CMonster& monster = *static_cast<CMonster*>(vecMonster[i]);
				if (monster.IsAlive() == false)
					continue;

				// ������ ���� ���� ��Ŷ ����
				CPacket& packetCreateMonster = AllocPacket();
				Msg_CreateMonsterCharacter(packetCreateMonster, monster, 0);
				SendUnicast(player, packetCreateMonster);
				packetCreateMonster.SubUseCount();

				// ���Ͱ� �����̰� ���� ��� ������ ���� �̵� ��Ŷ ����
				if (monster.IsMove())
				{
					CPacket& packetMoveMonster = AllocPacket();
					Msg_MoveMonster(packetMoveMonster, monster);
					SendUnicast(player, packetMoveMonster);
					packetMoveMonster.SubUseCount();
				}
			}

			auto& vecCrystal = _sector.GetObjectVector(x, y, ESectorObjectType::CRYSTAL);
			for (int i = 0; i < vecCrystal.size(); i++)
			{
				CCrystal& crystal = *static_cast<CCrystal*>(vecCrystal[i]);

				// ������ ũ����Ż ���� ��Ŷ ����
				CPacket& packetCreateCrystal = AllocPacket();
				Msg_CreateCrystal(packetCreateCrystal, crystal);
				SendUnicast(player, packetCreateCrystal);
				packetCreateCrystal.SubUseCount();
			}

		}
	}
	packetCreateMyChr.SubUseCount();





	// �������� ���ܵ� Ÿ�Ͽ� ���� ó��
	CPacket& packetDeleteMyChr = AllocPacket();
	Msg_RemoveObject(packetDeleteMyChr, player);
	for (int y = oldYDown; y <= oldYUp; y++)
	{
		for (int x = oldXLeft; x <= oldXRight; x++)
		{
			if (y <= newYUp && y >= newYDown && x <= newXRight && x >= newXLeft)
				continue;

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer& playerOther = *static_cast<CPlayer*>(vecPlayer[i]);
				
				// �� ĳ���� ������Ŷ�� ��뿡�� ����
				SendUnicast(playerOther, packetDeleteMyChr);

				// ��� ĳ���� ������Ŷ�� ������ ����
				CPacket& packetDeleteOtherChr = AllocPacket();
				Msg_RemoveObject(packetDeleteOtherChr, playerOther);
				SendUnicast(player, packetDeleteOtherChr);
				packetDeleteOtherChr.SubUseCount();
			}

			auto& vecMonster = _sector.GetObjectVector(x, y, ESectorObjectType::MONSTER);
			for (int i = 0; i < vecMonster.size(); i++)
			{
				CMonster& monster = *static_cast<CMonster*>(vecMonster[i]);
				if (monster.IsAlive() == false)
					continue;

				// ������ ���� ���� ��Ŷ ����
				CPacket& packetDeleteMonster = AllocPacket();
				Msg_RemoveObject(packetDeleteMonster, monster);
				SendUnicast(player, packetDeleteMonster);
				packetDeleteMonster.SubUseCount();
			}

			auto& vecCrystal = _sector.GetObjectVector(x, y, ESectorObjectType::CRYSTAL);
			for (int i = 0; i < vecCrystal.size(); i++)
			{
				CCrystal& crystal = *static_cast<CCrystal*>(vecCrystal[i]);

				// ������ ũ����Ż ���� ��Ŷ ����
				CPacket& packetDeleteCrystal = AllocPacket();
				Msg_RemoveObject(packetDeleteCrystal, crystal);
				SendUnicast(player, packetDeleteCrystal);
				packetDeleteCrystal.SubUseCount();
			}
		}
	}
	packetDeleteMyChr.SubUseCount();
}








/* ���� */

// ���͸� ���� Ÿ��, ���Ϳ��� ���� Ÿ��, ���ͷ� �ű��.
void CGameContentsField::MoveMonsterTileAndSector(CMonster& monster)
{
	// Ÿ���� �ٸ��ٸ� Ÿ���̵�
	auto& pos = monster.GetPosition();
	if (pos.tileX_old != pos.tileX || pos.tileY_old != pos.tileY)
	{
		_tile.RemoveObject(pos.tileX_old, pos.tileY_old, ETileObjectType::MONSTER, monster);
		_tile.AddObject(pos.tileX, pos.tileY, ETileObjectType::MONSTER, monster);
	}

	// ���Ͱ� ���ٸ� ���̻� ������ ����
	if (pos.sectorX_old == pos.sectorX && pos.sectorY_old == pos.sectorY)
	{
		return;
	}

	PROFILE_BEGIN("CGameContentsField::MoveMonsterTileAndSector");

	// ���� �̵�
	_sector.RemoveObject(pos.sectorX_old, pos.sectorY_old, ESectorObjectType::MONSTER, monster);
	_sector.AddObject(pos.sectorX, pos.sectorY, ESectorObjectType::MONSTER, monster);

	// ���� ���� ������ X����, Y���� ���ϱ�
	int oldXLeft = max(pos.sectorX_old - _sectorViewRange, 0);
	int oldXRight = min(pos.sectorX_old + _sectorViewRange, _sector.GetMaxX() - 1);
	int oldYDown = max(pos.sectorY_old - _sectorViewRange, 0);
	int oldYUp = min(pos.sectorY_old + _sectorViewRange, _sector.GetMaxY() - 1);

	// ���� ���� ������ X����, Y���� ���ϱ�
	int newXLeft = max(pos.sectorX - _sectorViewRange, 0);
	int newXRight = min(pos.sectorX + _sectorViewRange, _sector.GetMaxX() - 1);
	int newYDown = max(pos.sectorY - _sectorViewRange, 0);
	int newYUp = min(pos.sectorY + _sectorViewRange, _sector.GetMaxY() - 1);


	// ������ ���� ���Ե� ���Ϳ� ���� ó��
	CPacket& packetCreateMonster = AllocPacket();
	Msg_CreateMonsterCharacter(packetCreateMonster, monster, 0);
	for (int y = newYDown; y <= newYUp; y++)
	{
		for (int x = newXLeft; x <= newXRight; x++)
		{
			if (y <= oldYUp && y >= oldYDown && x <= oldXRight && x >= oldXLeft)
				continue;

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer& playerOther = *static_cast<CPlayer*>(vecPlayer[i]);

				// �÷��̾�� ���� ���� ��Ŷ ����
				SendUnicast(playerOther, packetCreateMonster);
			}
		}
	}
	packetCreateMonster.SubUseCount();


	// �������� ���ܵ� Ÿ�Ͽ� ���� ó��
	CPacket& packetDeleteMonster = AllocPacket();
	Msg_RemoveObject(packetDeleteMonster, monster);
	for (int y = oldYDown; y <= oldYUp; y++)
	{
		for (int x = oldXLeft; x <= oldXRight; x++)
		{
			if (y <= newYUp && y >= newYDown && x <= newXRight && x >= newXLeft)
				continue;

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer& playerOther = *static_cast<CPlayer*>(vecPlayer[i]);

				// ���� ���� ��Ŷ�� �÷��̾�� ����
				SendUnicast(playerOther, packetDeleteMonster);
			}
		}
	}
	packetDeleteMonster.SubUseCount();
}


// ���Ͱ� ������
void CGameContentsField::MonsterAttack(CMonster& monster)
{
	if (monster.IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::MonsterAttack");

	
	// �ֺ��� ���� �̵� ��Ŷ�� �����Ͽ� ������ �ٲ۴�. �׸��� �ֺ��� ���� ��Ŷ ����
	auto& posMon = monster.GetPosition();
	auto& statMon = monster.GetStatus();
	CPacket& packetMoveMonster = AllocPacket();    // �̵� ��Ŷ
	Msg_MoveMonster(packetMoveMonster, monster);
	SendAroundSector(posMon.sectorX, posMon.sectorY, packetMoveMonster, nullptr);
	packetMoveMonster.SubUseCount();

	CPacket& packetMonsterAttack = AllocPacket();  // ���� ��Ŷ
	Msg_MonsterAttack(packetMonsterAttack, monster);
	SendAroundSector(posMon.sectorX, posMon.sectorY, packetMonsterAttack, nullptr);
	packetMonsterAttack.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::MonsterAttack. monster attack. id:%lld, tile:(%d, %d), pos:(%.3f, %.3f), rotation:%d\n"
		, monster.GetClientId(), posMon.tileX, posMon.tileY, posMon.x, posMon.y, statMon.rotation);

	float monsterPosX = posMon.x;
	float monsterPosY = posMon.y;
	int degree = (360 - statMon.rotation + 90) % 360;  // rotation�� 360�� ������ ��ȯ

	// ���� 0�� �������� ȸ����Ű�� ���� cos, sin��
	float R[2] = { _math.GetCosine(360 - degree), _math.GetSine(360 - degree) };


	// ���� ������ �ش��ϴ� Ÿ�� �����ϱ�
	// ���ݹ��� Ÿ��: ���� Ÿ�� ���� �����¿� +2ĭ
	// ���ݹ��� ��ǥ: ���Ͱ� �ٶ󺸴� �������� ���� _attackRange �� �ݿ�
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
				CPlayer& playerOther = *static_cast<CPlayer*>(vecPlayer[i]);
				if (playerOther.IsAlive() == false)
					continue;

				// ���� ��ġ�� �������� ���� ��, �÷��̾��� ��ǥ�� �������� �ű�� 0�� �������� ȸ����Ŵ
				auto& posPlayer = playerOther.GetPosition();
				float playerPosX = (posPlayer.x - monsterPosX) * R[0] - (posPlayer.y - monsterPosY) * R[1];
				float playerPosY = (posPlayer.x - monsterPosX) * R[1] + (posPlayer.y - monsterPosY) * R[0];

				// �÷��̾��� ��ǥ�� ���� pMonster->_attackRange ���� pMonster->_attackRange * 2 �簢�� ���̸� hit
				if (playerPosX > 0 && playerPosX < statMon.attackRange && playerPosY > -statMon.attackRange && playerPosY < statMon.attackRange)
				{
					playerOther.Hit(statMon.damage);

					// ����� ��Ŷ ����
					CPacket& packetDamage = AllocPacket();
					Msg_Damage(packetDamage, monster, playerOther, statMon.damage);
					SendAroundSector(posPlayer.sectorX, posPlayer.sectorY, packetDamage, nullptr);
					packetDamage.SubUseCount();

					LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::MonsterAttack. hit player. sessionId:%lld, accountNo:%lld, target tile:(%d, %d), target pos:(%.3f, %.3f)\n"
						, playerOther.GetSessionId(), playerOther.GetAccountNo(), posPlayer.tileX, posPlayer.tileY, posPlayer.x, posPlayer.y);
				}
			}

		}
	}
}

// ������ Ÿ���� ã��
void CGameContentsField::MonsterFindTarget(CMonster& monster)
{
	PROFILE_BEGIN("CGameContentsField::MonsterFindTarget");

	auto& statMon = monster.GetStatus();
	auto& posMon = monster.GetPosition();

	int viewSectorRange = dfFIELD_POS_TO_SECTOR(statMon.viewRange) + 1;
	int initialSectorX = dfFIELD_POS_TO_SECTOR(posMon.x_initial);
	int initialSectorY = dfFIELD_POS_TO_SECTOR(posMon.y_initial);

	int minX = max(posMon.sectorX - viewSectorRange, 0);
	int maxX = min(posMon.sectorX + viewSectorRange, _sector.GetMaxX() - 1);
	int minY = max(posMon.sectorY - viewSectorRange, 0);
	int maxY = min(posMon.sectorY + viewSectorRange, _sector.GetMaxY() - 1);

	// ���ʹ� �þ� ������ ������ ��ġ���� �����Ͽ� ã���� �Ѵ�.
	int midX = minX + rand() % (maxX - minX + 1);
	int midY = minY + rand() % (maxY - minY + 1);
	
	for (int y = midY; y <= maxY; y++)
	{
		for (int x = midX; x <= maxX; x++)
		{
			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer& player = *static_cast<CPlayer*>(vecPlayer[i]);
				if (player.IsAlive() == false)
					continue;

				// �ֺ��� �÷��̾ �ִٸ� Ÿ������ �����ϵ��� �Ѵ�.
				monster.SetTargetPlayer(&player);
			}
		}
		for (int x = minX; x < midX; x++)
		{
			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer& player = *static_cast<CPlayer*>(vecPlayer[i]);
				if (player.IsAlive() == false)
					continue;

				// �ֺ��� �÷��̾ �ִٸ� Ÿ������ �����ϵ��� �Ѵ�.
				monster.SetTargetPlayer(&player);
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
				CPlayer& player = *static_cast<CPlayer*>(vecPlayer[i]);
				if (player.IsAlive() == false)
					continue;

				// �ֺ��� �÷��̾ �ִٸ� Ÿ������ �����ϵ��� �Ѵ�.
				monster.SetTargetPlayer(&player);
			}
		}
		for (int x = minX; x < midX; x++)
		{
			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer& player = *static_cast<CPlayer*>(vecPlayer[i]);
				if (player.IsAlive() == false)
					continue;

				// �ֺ��� �÷��̾ �ִٸ� Ÿ������ �����ϵ��� �Ѵ�.
				monster.SetTargetPlayer(&player);
			}
		}
	}
}





/* �÷��̾�� ��Ŷ ���� */

// �Ѹ��� ������
int CGameContentsField::SendUnicast(CPlayer& player, CPacket& packet)
{

	//packet.AddUseCount();
	//player._vecPacketBuffer.push_back(packet);
	SendPacket(player.GetSessionId(), packet);  // �ӽ�

	return 1;
}






// �ֺ� ���� ��ü�� ������
int CGameContentsField::SendAroundSector(int sectorX, int sectorY, CPacket& packet, const CPlayer* except)
{
	PROFILE_BEGIN("CGameContentsField::SendAroundSector");

	auto& vecAroundSector = _sector.GetAroundSector(sectorX, sectorY);

	int sendCount = 0;
	for (int i = 0; i < vecAroundSector.size(); i++)
	{
		auto& vecPlayer = vecAroundSector[i]->GetObjectVector(ESectorObjectType::PLAYER);
		for (int i = 0; i < vecPlayer.size(); i++)
		{
			CPlayer& player = *static_cast<CPlayer*>(vecPlayer[i]);
			if (&player == except)
				continue;

			//packet.AddUseCount();
			//player._vecPacketBuffer.push_back(packet);
			SendPacket(player.GetSessionId(), packet);  // �ӽ�
			sendCount++;
		}
	}

	return sendCount;
}




// 2�� ������ �ֺ� ���� ��ü�� ������
int CGameContentsField::SendTwoAroundSector(int sectorX1, int sectorY1, int sectorX2, int sectorY2, CPacket& packet, const CPlayer* except)
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
	// y ���� 2�� Ÿ�� ���� �� ���� ������ ū ������, x �൵ 2�� Ÿ�� ���� �� ���� ������ ū ������ �����Ѵ�.
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			// ���� (x,y)�� sector1 �������� ������ �ʰ� sector2 �������� ������ �ʴ´ٸ� �Ѿ
			if (!(y >= downY1 && y <= upY1 && x >= leftX1 && x <= rightX1)
				&& !(y >= downY2 && y <= upY2 && x >= leftX2 && x <= rightX2))
			{
				continue;
			}

			auto& vecPlayer = _sector.GetObjectVector(x, y, ESectorObjectType::PLAYER);
			for (int i = 0; i < vecPlayer.size(); i++)
			{
				CPlayer& player = *static_cast<CPlayer*>(vecPlayer[i]);
				if (&player == except)
					continue;

				//packet.AddUseCount();
				//player._vecPacketBuffer.push_back(packet);
				SendPacket(player.GetSessionId(), packet); // �ӽ�
				sendCount++;
			}
		}
	}

	return sendCount;
}





/* ��Ŷ ó�� */

// ĳ���� �̵� ��û ó��
void CGameContentsField::PacketProc_MoveCharacter(CPacket& packet, CPlayer& player)
{
	if (player.IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_MoveCharacter");

	INT64 clientId;
	float posX;
	float posY;
	USHORT rotation;
	BYTE VKey;
	BYTE HKey;
	packet >> clientId >> posX >> posY >> rotation >> VKey >> HKey;
	
	player.MovePosition(posX, posY, rotation, VKey, HKey);

	// �ɾҴ� �Ͼ�� �� �÷��̾� HP ����
	if (player.IsSit())
	{
		player.SetSit(false);
		CPacket& packetPlayerHp = AllocPacket();
		Msg_PlayerHP(packetPlayerHp, player);
		SendUnicast(player, packetPlayerHp);
		packetPlayerHp.SubUseCount();

		// DB�� ���� ��û
		DB_PlayerRenewHP(player);
	}


	// Ÿ�� �� ���� �̵�
	auto& pos = player.GetPosition();
	auto& character = player.GetCharacter();
	if (pos.tileY != pos.tileY_old || pos.tileX != pos.tileX_old)
	{
		MovePlayerTileAndSector(player);
	}


	// �� �ֺ��� ĳ���� �̵� ��Ŷ ����
	CPacket& packetMoveChr = AllocPacket();
	Msg_MoveCharacter(packetMoveChr, player);
	SendAroundSector(pos.sectorX, pos.sectorY, packetMoveChr, &player);
	packetMoveChr.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_MoveCharacter. move player. sessionId:%lld, accountNo:%lld, tile (%d, %d) to (%d, %d), pos (%.3f, %.3f) to (%.3f, %.3f), rotation:%d\n"
		, player.GetSessionId(), player.GetAccountNo(), pos.tileX_old, pos.tileY_old, pos.tileX, pos.tileY, pos.x_old, pos.y_old, pos.x, pos.y, character.rotation);
}




// ĳ���� ���� ��û ó��
void CGameContentsField::PacketProc_StopCharacter(CPacket& packet, CPlayer& player)
{
	if (player.IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_StopCharacter");

	INT64 clientId;
	float x;
	float y;
	USHORT rotation;
	packet >> clientId >> x >> y >> rotation;

	player.MovePosition(x, y, rotation, 0, 0);

	// �ɾҴ� �Ͼ�� �� �÷��̾� HP ����
	if (player.IsSit())
	{
		player.SetSit(false);
		CPacket& packetPlayerHp = AllocPacket();
		Msg_PlayerHP(packetPlayerHp, player);
		SendUnicast(player, packetPlayerHp);
		packetPlayerHp.SubUseCount();

		// DB�� ���� ��û
		DB_PlayerRenewHP(player);
	}


	// Ÿ�� ��ǥ�� �޶����� Ÿ�� �� ���� �̵�
	auto& pos = player.GetPosition();
	auto& character = player.GetCharacter();
	if (pos.tileY != pos.tileY_old || pos.tileX != pos.tileX_old)
	{
		MovePlayerTileAndSector(player);
	}


	// �� �ֺ��� ĳ���� ���� ��Ŷ ����
	CPacket& packetStopChr = AllocPacket();
	Msg_StopCharacter(packetStopChr, player);
	SendAroundSector(pos.sectorX, pos.sectorY, packetStopChr, &player);
	packetStopChr.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_StopCharacter. stop player. sessionId:%lld, accountNo:%lld, tile (%d, %d) to (%d, %d), pos (%.3f, %.3f) to (%.3f, %.3f), rotation:%d\n"
		, player.GetSessionId(), player.GetAccountNo(), pos.tileX_old, pos.tileY_old, pos.tileX, pos.tileY, pos.x_old, pos.y_old, pos.x, pos.y, character.rotation);
}



// �ݱ� ��û
void CGameContentsField::PacketProc_Pick(CPacket& packet, CPlayer& player)
{
	if (player.IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_Pick");

	INT64 clientId;
	packet >> clientId;

	auto& pos = player.GetPosition();
	auto& character = player.GetCharacter();

	// �ֺ� ���Ϳ� �Ա� �׼� ����
	CPacket& packetPickAction = AllocPacket();
	Msg_Pick(packetPickAction, player);
	SendAroundSector(pos.sectorX, pos.sectorY, packetPickAction, nullptr);
	packetPickAction.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Pick. pick. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, player.GetSessionId(), player.GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);

	// �ֺ� Ÿ�Ͽ� �ִ� ��� ũ����Ż ȹ��
	int minY = max(pos.tileY - _config.tilePickRange, 0);
	int maxY = min(pos.tileY + _config.tilePickRange, dfFIELD_TILE_MAX_Y - 1);
	int minX = max(pos.tileX - _config.tilePickRange, 0);
	int maxX = min(pos.tileX + _config.tilePickRange, dfFIELD_TILE_MAX_X - 1);
	int pickAmount = 0;
	const CCrystal* pFirstCrystal = nullptr;
	for (int y = minY; y <= maxY; y++)
	{
		for (int x = minX; x <= maxX; x++)
		{
			const std::vector<CObject*>& vecCrystal = _tile.GetObjectVector(x, y, ETileObjectType::CRYSTAL);
			for (int i = 0; i < vecCrystal.size(); i++)
			{
				CCrystal& crystal = *static_cast<CCrystal*>(vecCrystal[i]);
				auto& posCrystal = crystal.GetPosition();
				auto& statCrystal = crystal.GetStatus();

				if (crystal.IsAlive() == false)
					continue;

				if (pFirstCrystal == nullptr)
					pFirstCrystal = &crystal;

				player.AddCrystal(statCrystal.amount);
				pickAmount += statCrystal.amount;

				// �ֺ��� ũ����Ż ���� ��Ŷ ����
				CPacket& packetDeleteCrystal = AllocPacket();
				Msg_RemoveObject(packetDeleteCrystal, crystal);
				SendAroundSector(posCrystal.sectorX, posCrystal.sectorY, packetDeleteCrystal, nullptr);
				packetDeleteCrystal.SubUseCount();

				// ũ����Ż ����ó��
				crystal.SetDead();

				LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Pick. get crystal. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f), crystal:%d (+%d)\n"
					, player.GetSessionId(), player.GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y, character.crystal, statCrystal.amount);

			}
		}
	}

	if (pickAmount > 0)
	{
		// �ֺ��� ũ����Ż ȹ�� ��Ŷ ����
		CPacket& packetPickCrystal = AllocPacket();
		Msg_PickCrystal(packetPickCrystal, player, *pFirstCrystal);
		SendAroundSector(pos.sectorX, pos.sectorY, packetPickCrystal, nullptr);
		packetPickCrystal.SubUseCount();

		// DB�� ����
		DB_PlayerGetCrystal(player);
	}

}

// ĳ���� ���� ��û ó��1
void CGameContentsField::PacketProc_Attack1(CPacket& packet, CPlayer& player)
{
	if (player.IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_Attack1");

	INT64 clientId;
	packet >> clientId;

	auto& pos = player.GetPosition();
	auto& character = player.GetCharacter();

	// �ֺ��� ���� ��Ŷ ����
	CPacket& packetAttack = AllocPacket();
	Msg_Attack1(packetAttack, player);
	SendAroundSector(pos.sectorX, pos.sectorY, packetAttack, &player);
	packetAttack.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Attack1. attack1. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, player.GetSessionId(), player.GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);


	float playerPosX = pos.x;
	float playerPosY = pos.y;
	int degree = (360 - character.rotation + 90) % 360;  // rotation�� 360�� ������ ��ȯ

	// ���� 0�� �������� ȸ����Ű�� ���� cos, sin��
	float R[2] = { _math.GetCosine(360 - degree), _math.GetSine(360 - degree) };


	// ���ݹ����� ������ Ÿ��: �� ��ġ ���� �����¿� 2�� Ÿ��
	// ���ݹ���: ���� �ٶ󺸴� �������� �ʺ� 0.5��~1.0��, ���� 1.5�� �簢��
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
				CMonster& monster = *static_cast<CMonster*>(vecMonster[i]);
				if (monster.IsAlive() == false)
					continue;

				// �÷��̾� ��ġ�� �������� ���� ��, ������ ��ǥ�� �������� �ű�� 0�� �������� ȸ����Ŵ
				auto& posMon = monster.GetPosition();
				float monsterPosX = (posMon.x - playerPosX) * R[0] - (posMon.y - playerPosY) * R[1];
				float monsterPosY = (posMon.x - playerPosX) * R[1] + (posMon.y - playerPosY) * R[0];

				// ������ ��ǥ�� ���� 1.5 ���� 1.5 �簢�� ���̸� hit
				if (monsterPosX > -0.5 && monsterPosX < 1.0 && monsterPosY > -0.75 && monsterPosY < 0.75)
				{
					monster.Hit(character.damageAttack1);

					// ����� ��Ŷ ����
					CPacket& packetDamage = AllocPacket();
					Msg_Damage(packetDamage, player, monster, character.damageAttack1);
					SendTwoAroundSector(pos.sectorX, pos.sectorY, posMon.sectorX, posMon.sectorY, packetDamage, nullptr);
					packetDamage.SubUseCount();

					LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Attack1. hit monster. sessionId:%lld, accountNo:%lld, target tile:(%d, %d), target pos:(%.3f, %.3f)\n"
						, player.GetSessionId(), player.GetAccountNo(), posMon.tileX, posMon.tileY, posMon.x, posMon.y);
				}
			}

		}
	}
}




// ĳ���� ���� ��û ó��2
void CGameContentsField::PacketProc_Attack2(CPacket& packet, CPlayer& player)
{
	if (player.IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_Attack2");

	INT64 clientId;
	packet >> clientId;

	auto& pos = player.GetPosition();
	auto& character = player.GetCharacter();

	// �ֺ��� ���� ��Ŷ ����
	CPacket& packetAttack = AllocPacket();
	Msg_Attack2(packetAttack, player);
	SendAroundSector(pos.sectorX, pos.sectorY, packetAttack, &player);
	packetAttack.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Attack2. attack2. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, player.GetSessionId(), player.GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);


	// ���� �ٶ󺸴� �������� 0.5��, 1.5���� ��ǥ�� ã��
	float playerPosX = pos.x;
	float playerPosY = pos.y;
	int degree = (360 - character.rotation + 90) % 360;  // rotation�� 360�� ������ ��ȯ
	float tailPosX = _math.GetCosine(degree + 180) * 0.5f + playerPosX;  // 1.5 ��ġ�� x
	float tailPosY = _math.GetSine(degree + 180) * 0.5f + playerPosY;    // 1.5 ��ġ�� y
	float headPosX = _math.GetCosine(degree) * 1.5f + playerPosX;        // 2.5 ��ġ�� x
	float headPosY = _math.GetSine(degree) * 1.5f + playerPosY;          // 2.5 ��ġ�� y

	// ���� 0�� �������� ȸ����Ű�� ���� cos, sin��
	float R[2] = { _math.GetCosine(360 - degree), _math.GetSine(360 - degree) };

	// ���� ������ �ش��ϴ� Ÿ�� ã��
	// ���ݹ����� ������ Ÿ��: ���� �ٶ󺸴� �������� 0.5�� ~ 1.5�� ��ġ�� �����ϴ� Ÿ��
	// ���ݹ���: ���� �ٶ󺸴� �������� �ʺ� 0.5��~1.5��, ���� 1.0�� �簢��
	int minTileX = max(dfFIELD_POS_TO_TILE(min(tailPosX, headPosX)) - 1, 0);
	int maxTileX = min(dfFIELD_POS_TO_TILE(max(tailPosX, headPosX)) + 2, dfFIELD_TILE_MAX_X - 1);
	int minTileY = max(dfFIELD_POS_TO_TILE(min(tailPosY, headPosY)) - 1, 0);
	int maxTileY = min(dfFIELD_POS_TO_TILE(max(tailPosY, headPosY)) + 2, dfFIELD_TILE_MAX_Y - 1);

	for (int y = minTileY; y <= maxTileY; y++)
	{
		for (int x = minTileX; x <= maxTileX; x++)
		{
			const std::vector<CObject*>& vecMonster = _tile.GetObjectVector(x, y, ETileObjectType::MONSTER);
			for (int i = 0; i < vecMonster.size(); i++)
			{
				CMonster& monster = *static_cast<CMonster*>(vecMonster[i]);
				if (monster.IsAlive() == false)
					continue;

				// �÷��̾� ��ġ�� �������� ���� ��, ������ ��ǥ�� �������� �ű�� 0�� �������� ȸ����Ŵ
				auto& posMon = monster.GetPosition();
				float monsterPosX = (posMon.x - playerPosX) * R[0] - (posMon.y - playerPosY) * R[1];
				float monsterPosY = (posMon.x - playerPosX) * R[1] + (posMon.y - playerPosY) * R[0];

				// ������ ��ǥ�� ���� 2.0 ���� 1.0 �簢�� ���̸� hit
				if (monsterPosX > -0.5 && monsterPosX < 1.5 && monsterPosY > -0.6 && monsterPosY < 0.6)
				{
					monster.Hit(character.damageAttack2);

					// ����� ��Ŷ ����
					CPacket& packetDamage = AllocPacket();
					Msg_Damage(packetDamage, player, monster, character.damageAttack2);
					SendTwoAroundSector(pos.sectorX, pos.sectorY, posMon.sectorX, posMon.sectorY, packetDamage, nullptr);
					packetDamage.SubUseCount();

					LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Attack2. hit monster. sessionId:%lld, accountNo:%lld, target tile:(%d, %d), target pos:(%.3f, %.3f)\n"
						, player.GetSessionId(), player.GetAccountNo(), posMon.tileX, posMon.tileY, posMon.x, posMon.y);
				}
			}

		}
	}
}




// �ٴڿ� �ɱ� ��û ó��
void CGameContentsField::PacketProc_Sit(CPacket& packet, CPlayer& player)
{
	if (player.IsAlive() == false)
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_Sit");

	INT64 clientId;
	packet >> clientId;

	// �÷��̾� �ɱ� ó��
	player.SetSit(true);

	// �ֺ� Ÿ�Ͽ� �ɱ� ����
	auto& pos = player.GetPosition();
	CPacket& packetSit = AllocPacket();
	Msg_Sit(packetSit, player);
	SendAroundSector(pos.sectorX, pos.sectorY, packetSit, &player);
	packetSit.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Sit. sit. sessionId:%lld, accountNo:%lld, tile:(%d, %d), pos:(%.3f, %.3f)\n"
		, player.GetSessionId(), player.GetAccountNo(), pos.tileX, pos.tileY, pos.x, pos.y);
}


// �÷��̾� ���� �� �ٽ��ϱ� ��û
void CGameContentsField::PacketProc_PlayerRestart(CPacket& packet, CPlayer& player)
{
	if (player.IsAlive() == true)  // ������� �ʾҴٸ� ����
		return;

	PROFILE_BEGIN("CGameContentsField::PacketProc_PlayerRestart");

	// �÷��̾ Ÿ�ϰ� ���Ϳ��� �����ϰ� �ֺ��� ĳ���� ���� ��Ŷ�� ������.
	auto& pos = player.GetPosition();
	_tile.RemoveObject(pos.tileX, pos.tileY, ETileObjectType::PLAYER, player);
	_sector.RemoveObject(pos.sectorX, pos.sectorY, ESectorObjectType::PLAYER, player);
	CPacket& packetDeleteCharacter = AllocPacket();
	Msg_RemoveObject(packetDeleteCharacter, player);
	SendAroundSector(pos.sectorX, pos.sectorY, packetDeleteCharacter, nullptr);
	packetDeleteCharacter.SubUseCount();

	// ������ ����
	player.SetRespawn(FIELD_PLAYER_RESPAWN_TILE_X, FIELD_PLAYER_RESPAWN_TILE_Y);
	DB_PlayerRespawn(player);

	// �ٽ��ϱ� ��Ŷ ����
	CPacket& packetPlayerRestart = AllocPacket();
	Msg_PlayerRestart(packetPlayerRestart);
	SendUnicast(player, packetPlayerRestart);
	packetPlayerRestart.SubUseCount();

	// �÷��̾ Ÿ�ϰ� ���Ϳ� ����ϰ�, ĳ���͸� �����ϰ�, �ֺ� ������Ʈ���� �ε��Ѵ�.
	InitializePlayerEnvironment(player);

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_PlayerRestart. respawn player. sessionId:%lld, accountNo:%lld, tile:(%d, %d) to (%d, %d)\n"
		, player.GetSessionId(), player.GetAccountNo(), pos.tileX_old, pos.tileY_old, pos.tileX, pos.tileY);

}


// �׽�Ʈ ���� ��û ó��
void CGameContentsField::PacketProc_Echo(CPacket& packet, CPlayer& player)
{
	INT64 accountNo;
	LONGLONG sendTick;
	packet >> accountNo >> sendTick;

	CPacket& packetEcho = AllocPacket();
	Msg_Echo(packetEcho, player, sendTick);
	SendUnicast(player, packetEcho);
	packetEcho.SubUseCount();

	LOGGING(LOGGING_LEVEL_DEBUG, L"CGameContentsField::PacketProc_Echo. echo. sessionId:%lld, accountNo:%lld\n"
		, player.GetSessionId(), player.GetAccountNo());
}




/* ��Ŷ�� ������ �Է� */

// �� ĳ���� ���� ��Ŷ
void CGameContentsField::Msg_CreateMyCharacter(CPacket& packet, const CPlayer& player)
{
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
	auto& account = player.GetAccountInfo();
	packet << (WORD)en_PACKET_CS_GAME_RES_CREATE_MY_CHARACTER << player.GetClientId() << character.characterType;
	packet.PutData((const char*)account.szNick, sizeof(account.szNick));
	packet << pos.x << pos.y << character.rotation << character.crystal << character.hp << character.exp << character.level;
}

// �ٸ� ĳ���� ���� ��Ŷ
void CGameContentsField::Msg_CreateOtherCharacter(CPacket& packet, const CPlayer& player)
{
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
	auto& account = player.GetAccountInfo();
	packet << (WORD)en_PACKET_CS_GAME_RES_CREATE_OTHER_CHARACTER << player.GetClientId() << character.characterType;
	packet.PutData((const char*)account.szNick, sizeof(account.szNick));
	packet << pos.x << pos.y << character.rotation << character.level << character.respawn << character.die << character.sit;
}

// ���� ���� ��Ŷ
void CGameContentsField::Msg_CreateMonsterCharacter(CPacket& packet, const CMonster& monster, BYTE respawn)
{
	auto& status = monster.GetStatus();
	auto& pos = monster.GetPosition();
	packet << (WORD)en_PACKET_CS_GAME_RES_CREATE_MONSTER_CHARACTER << monster.GetClientId() << pos.x << pos.y << status.rotation << respawn;
}

// ĳ����, ������Ʈ ���� ��Ŷ
void CGameContentsField::Msg_RemoveObject(CPacket& packet, const CObject& object)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_REMOVE_OBJECT << object.GetClientId();
}

// ĳ���� �̵� ��Ŷ
void CGameContentsField::Msg_MoveCharacter(CPacket& packet, const CPlayer& player)
{
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
	packet << (WORD)en_PACKET_CS_GAME_RES_MOVE_CHARACTER << player.GetClientId() << pos.x << pos.y << character.rotation << character.VKey << character.HKey;
}

// ĳ���� ���� ��Ŷ
void CGameContentsField::Msg_StopCharacter(CPacket& packet, const CPlayer& player)
{
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
	packet << (WORD)en_PACKET_CS_GAME_RES_STOP_CHARACTER << player.GetClientId() << pos.x << pos.y << character.rotation;
}

// ���� �̵� ��Ŷ
void CGameContentsField::Msg_MoveMonster(CPacket& packet, const CMonster& monster)
{
	auto& stat = monster.GetStatus();
	auto& pos = monster.GetPosition();
	packet << (WORD)en_PACKET_CS_GAME_RES_MOVE_MONSTER << monster.GetClientId() << pos.x << pos.y << stat.rotation;
}

// ĳ���� ����1 ��Ŷ
void CGameContentsField::Msg_Attack1(CPacket& packet, const CPlayer& player)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_ATTACK1 << player.GetClientId();
}

// ĳ���� ����1 ��Ŷ
void CGameContentsField::Msg_Attack2(CPacket& packet, const CPlayer& player)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_ATTACK2 << player.GetClientId();
}

// ���� ���� ��Ŷ
void CGameContentsField::Msg_MonsterAttack(CPacket& packet, const CMonster& monster)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_MONSTER_ATTACK << monster.GetClientId();
}

// ���ݴ�󿡰� ������� ���� ��Ŷ
void CGameContentsField::Msg_Damage(CPacket& packet, const CObject& attackObject, const CObject& targetObject, int damage)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_DAMAGE << attackObject.GetClientId() << targetObject.GetClientId() << damage;
}

// ���� ��� ��Ŷ
void CGameContentsField::Msg_MonsterDie(CPacket& packet, const CMonster& monster)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_MONSTER_DIE << monster.GetClientId();
}

// ũ����Ż ���� ��Ŷ
void CGameContentsField::Msg_CreateCrystal(CPacket& packet, const CCrystal& crystal)
{
	auto& stat = crystal.GetStatus();
	auto& pos = crystal.GetPosition();
	packet << (WORD)en_PACKET_CS_GAME_RES_CREATE_CRISTAL << crystal.GetClientId() << stat.crystalType << pos.posX << pos.posY;
}

// ũ����Ż �Ա� ��û
void CGameContentsField::Msg_Pick(CPacket& packet, const CPlayer& player)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_PICK << player.GetClientId();
}

// �ٴڿ� �ɱ� ��Ŷ
void CGameContentsField::Msg_Sit(CPacket& packet, const CPlayer& player)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_SIT << player.GetClientId();
}

// ũ����Ż ȹ�� ��Ŷ
void CGameContentsField::Msg_PickCrystal(CPacket& packet, const CPlayer& player, const CCrystal& crystal)
{
	auto& character = player.GetCharacter();
	packet << (WORD)en_PACKET_CS_GAME_RES_PICK_CRISTAL << player.GetClientId() << crystal.GetClientId() << character.crystal;
}

// �÷��̾� HP ���� ��Ŷ
void CGameContentsField::Msg_PlayerHP(CPacket& packet, const CPlayer& player)
{
	auto& character = player.GetCharacter();
	packet << (WORD)en_PACKET_CS_GAME_RES_PLAYER_HP << character.hp;
}

// �÷��̾� ���� ��Ŷ
void CGameContentsField::Msg_PlayerDie(CPacket& packet, const CPlayer& player, int minusCrystal)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_PLAYER_DIE << player.GetClientId() << minusCrystal;
}

// �÷��̾� ���� �� �ٽ��ϱ� ��Ŷ
void CGameContentsField::Msg_PlayerRestart(CPacket& packet)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_PLAYER_RESTART;
}

// �׽�Ʈ ���� ��Ŷ
void CGameContentsField::Msg_Echo(CPacket& packet, const CPlayer& player, LONGLONG sendTick)
{
	packet << (WORD)en_PACKET_CS_GAME_RES_ECHO << player.GetAccountNo() << sendTick;
}


/* DB�� ������ ���� */
void CGameContentsField::DB_Login(CPlayer& player)
{
	auto& account = player.GetAccountInfo();
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
	std::wstring strIP(account.sIP);
	strIP += L":";
	strIP += std::to_wstring(account.port);
	_pDBConn->PostQueryRequest(
		L"INSERT INTO `logdb`.`gamelog` (`type`, `code`, `accountno`, `servername`, `param1`, `param2`, `param3`, `param4`, `message`)"
		" VALUES (%d, %d, %lld, '%s', %d, %d, %d, %d, '%s');"
		, 1, 11, player.GetAccountNo(), L"Game", pos.tileX, pos.tileY, character.crystal, character.hp, strIP.c_str()
	);
}

void CGameContentsField::DB_Logout(CPlayer& player)
{
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
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
		, player.GetAccountNo()
		, 1, 12, player.GetAccountNo(), L"Game", pos.tileX, pos.tileY, character.crystal, character.hp
	);
	player.SetDBUpdateTime();
}

void CGameContentsField::DB_PlayerDie(CPlayer& player)
{
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
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
		, player.GetAccountNo()
		, 3, 31, player.GetAccountNo(), L"Game", pos.tileX, pos.tileY, character.crystal
	);
	player.SetDBUpdateTime();
}

void CGameContentsField::DB_PlayerRespawn(CPlayer& player)
{
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
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
		, player.GetAccountNo()
		, 3, 31, player.GetAccountNo(), L"Game", pos.tileX, pos.tileY, character.crystal
	);
	player.SetDBUpdateTime();
}

void CGameContentsField::DB_PlayerGetCrystal(CPlayer& player)
{
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
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
		, player.GetAccountNo()
		, 4, 41, player.GetAccountNo(), L"Game", character.crystal - character.oldCrystal, character.crystal
	);
	player.SetDBUpdateTime();
}

void CGameContentsField::DB_PlayerRenewHP(CPlayer& player)
{
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
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
		, player.GetAccountNo()
		, 5, 51, player.GetAccountNo(), L"Game", character.sitStartHp, character.hp, (int)((GetTickCount64() - character.sitStartTime) / 1000)
	);
	player.SetDBUpdateTime();
}



void CGameContentsField::DB_InsertPlayerInfo(CPlayer& player)
{
	auto& character = player.GetCharacter();
	auto& pos = player.GetPosition();
	_pDBConn->PostQueryRequest(
		L"START TRANSACTION;"
		" INSERT INTO `gamedb`.`character` (`accountno`, `charactertype`, `posx`, `posy`, `tilex`, `tiley`, `rotation`, `crystal`, `hp`, `exp`, `level`, `die`)"
		" VALUES(%lld, %d, %.3f, %.3f, %d, %d, %d, %d, %d, %lld, %d, %d);"
		" "
		" INSERT INTO `logdb`.`gamelog` (`type`, `code`, `accountno`, `servername`, `param1`)"
		" VALUES (%d, %d, %lld, '%s', %d);"
		" COMMIT;"
		, player.GetAccountNo(), character.characterType, pos.x, pos.y, pos.tileX, pos.tileY, character.rotation, character.crystal, character.hp, character.exp, character.level, character.die
		, 3, 32, player.GetAccountNo(), L"Game", character.characterType
	);
	player.SetDBUpdateTime();
}





/* utils */
void CGameContentsField::ReadAreaInfoFromConfigJson(const CJsonParser& configJson, int areaNumber)
{
	// json ������ "Area#" ��ҿ��� �����͸� �о _vecAreaInfo �� �Է��Ѵ�.
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










