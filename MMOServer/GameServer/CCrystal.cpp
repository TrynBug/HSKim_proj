#include <iostream>
#include <Windows.h>
#include "defineGameServer.h"
#include "CCrystal.h"



CCrystal::CCrystal()
{

}

CCrystal::~CCrystal()
{
}

void CCrystal::Init(float posX, float posY, BYTE crystalType)
{
	CObject::Init();

	_pos.SetPosition(posX, posY);

	_status.crystalType = crystalType;
	_status.amount = crystalType * 10;
	
	_bAlive = true;
}


void CCrystal::SetDead()
{
	_bAlive = false;
}


// @Override
void CCrystal::Update()
{

}



void CCrystal::Position::SetPosition(float posX, float posY)
{
	posX = posX;
	posY = posY;
	tileX = dfFIELD_POS_TO_TILE(posX);
	tileY = dfFIELD_POS_TO_TILE(posY);
	sectorX = dfFIELD_TILE_TO_SECTOR(tileX);
	sectorY = dfFIELD_TILE_TO_SECTOR(tileY);
}