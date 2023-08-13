#pragma once

namespace netlib_game
{
	class CPacket;
}

namespace gameserver
{
	class CObject;
	class CPlayer;
	class CMonster;
	class CCrystal;

	using CPacket = netlib_game::CPacket;
	using CObject_t = std::shared_ptr<CObject>;
	using CPlayer_t = std::shared_ptr<CPlayer>;
	using CMonster_t = std::shared_ptr<CMonster>;
	using CCrystal_t = std::shared_ptr<CCrystal>;


	struct _TransferPlayerData
	{
		CPlayer_t pPlayer;
	};


	enum class EContentsThread
	{
		AUTH = 0,
		FIELD = 1,
		END
	};


	#define dfFIELD_POS_MAX_X     200.f
	#define dfFIELD_POS_MAX_Y     100.f
	#define dfFIELD_TILE_MAX_X    400
	#define dfFIELD_TILE_MAX_Y    200

	#define dfFIELD_NUM_TILE_X_PER_SECTOR    4
	#define dfFIELD_SECTOR_MAX_X           100           // dfFIELD_TILE_MAX_X / dfFIELD_NUM_TILE_X_PER_SECTOR
	#define dfFIELD_SECTOR_MAX_Y            50           //    
	#define dfFIELD_SECTOR_VIEW_RANGE       5
	#define dfFIELD_NUM_MAX_AROUND_SECTOR  ((dfFIELD_SECTOR_VIEW_RANGE + dfFIELD_SECTOR_VIEW_RANGE + 1) * (dfFIELD_SECTOR_VIEW_RANGE + dfFIELD_SECTOR_VIEW_RANGE + 1))


	#define dfFIELD_TILE_TO_SECTOR(tile)     ((tile) / dfFIELD_NUM_TILE_X_PER_SECTOR)
	#define dfFIELD_TILE_TO_POS(tile)        ((float)(tile) / 2.f)
	#define dfFIELD_POS_TO_TILE(pos)         ((int)((pos) * 2.f))
	#define dfFIELD_POS_TO_SECTOR(pos)       dfFIELD_TILE_TO_SECTOR(dfFIELD_POS_TO_TILE(pos))

}
