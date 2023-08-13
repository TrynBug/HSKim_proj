#include "stdafx.h"

#include "CObject.h"
#include "CTile.h"

using namespace gameserver;

CTile::CTile(int x, int y)
	: _x(x)
	, _y(y)
{
	for (int i = 0; i < (UINT)ETileObjectType::END; i++)
		_vecObject[i].clear();
}

CTile::~CTile()
{
}




void CTile::AddObject(ETileObjectType type, CObject_t pObj)
{
	_vecObject[(UINT)type].push_back(std::move(pObj));
}


void CTile::RemoveObject(ETileObjectType type, CObject_t pObj)
{
	bool remove = false;
	std::vector<CObject_t>& vec = _vecObject[(UINT)type];
	for (int i = 0; i < vec.size(); i++)
	{
		if (vec[i] == pObj)
		{
			vec[i] = vec.back();
			vec.pop_back();
			remove = true;
			break;
		}
	}
	if (remove == false)
	{
		std::ostringstream oss;
		oss << "Tried to remove the invalid object from the tile. (x,y):(" << _x << "," << "_y" << "), type:" << (UINT)type << ", object ID : " << pObj->GetClientId() << "\n";
		throw std::runtime_error(oss.str().c_str());
	}
}





CTileGrid::CTileGrid(int maxX, int maxY)
	: _maxX(maxX)
	, _maxY(maxY)
{
	// tile 배열 메모리 할당
	_tile = new CTile*[_maxY];
	_tile[0] = (CTile*)malloc(sizeof(CTile) * _maxY * _maxX);
	for (int y = 0; y < _maxY; y++)
		_tile[y] = _tile[0] + (y * _maxX);

	// tile 생성자 호출
	for (int y = 0; y < _maxY; y++)
	{
		for (int x = 0; x < _maxX; x++)
		{
			new (&_tile[y][x]) CTile(x, y);
		}
	}
}


CTileGrid::~CTileGrid()
{
	// tile 소멸자 호출
	for (int y = 0; y < _maxY; y++)
	{
		for (int x = 0; x < _maxX; x++)
		{
			(_tile[y][x]).~CTile();
		}
	}

	delete _tile[0];
	delete[] _tile;
}

int CTileGrid::GetNumOfObject(int x, int y, ETileObjectType type) const
{ 
	CheckCoordinate(x, y);
	return _tile[y][x].GetNumOfObject(type);
}

const std::vector<CObject_t>& CTileGrid::GetObjectVector(int x, int y, ETileObjectType type) const
{
	CheckCoordinate(x, y);
	return _tile[y][x].GetObjectVector(type);
}

void CTileGrid::AddObject(int x, int y, ETileObjectType type, CObject_t pObj)
{
	CheckCoordinate(x, y);
	_tile[y][x].AddObject(type, std::move(pObj));
}


void CTileGrid::RemoveObject(int x, int y, ETileObjectType type, CObject_t pObj)
{
	CheckCoordinate(x, y);
	_tile[y][x].RemoveObject(type, std::move(pObj));
}


void CTileGrid::CheckCoordinate(int x, int y) const
{
	if (x < 0 || x >= _maxX || y < 0 || y >= _maxY)
	{
		std::string message = "invalid tile coordinate: (";
		message += std::to_string(x) + ", " + std::to_string(y) + ")\n";
		throw std::out_of_range(message);
	}
}