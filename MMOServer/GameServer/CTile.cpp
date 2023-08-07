#include <Windows.h>
#include <vector>
#include <string>
#include <stdexcept>
#include "CObject.h"
#include "CTile.h"


CTile::CTile(int x, int y)
	: _x(x)
	, _y(y)
{

}

CTile::~CTile()
{
}




void CTile::AddObject(ETileObjectType type, CObject& object)
{
	_vecObject[(UINT)type].push_back(&object);
}


void CTile::RemoveObject(ETileObjectType type, CObject& object)
{
	bool remove = false;
	std::vector<CObject*>& vec = _vecObject[(UINT)type];
	for (int i = 0; i < vec.size(); i++)
	{
		if (vec[i] == &object)
		{
			vec[i] = vec.back();
			vec.pop_back();
			remove = true;
			break;
		}
	}
	if (remove == false)
	{
		int* p = 0;
		*p = 0;
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

const std::vector<CObject*>& CTileGrid::GetObjectVector(int x, int y, ETileObjectType type) const
{
	CheckCoordinate(x, y);
	return _tile[y][x].GetObjectVector(type);
}

void CTileGrid::AddObject(int x, int y, ETileObjectType type, CObject& obj)
{
	obj.VirtualDummy();  // 디버그
	CheckCoordinate(x, y);
	_tile[y][x].AddObject(type, obj);
}


void CTileGrid::RemoveObject(int x, int y, ETileObjectType type, CObject& obj)
{
	obj.VirtualDummy();  // 디버그
	CheckCoordinate(x, y);
	_tile[y][x].RemoveObject(type, obj);
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