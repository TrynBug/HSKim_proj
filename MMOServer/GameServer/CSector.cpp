#include <Windows.h>
#include <vector>
#include <string>
#include <stdexcept>

#include "defineGameServer.h"
#include "CObject.h" // �����
#include "CSector.h"


CSector::CSector(int x, int y)
	: _x(x)
	, _y(y)
{
}

CSector::~CSector()
{
}



void CSector::AddAroundSector(CSector* pSector)
{
	_vecAroundSector.push_back(pSector);
}


void CSector::AddObject(ESectorObjectType type, CObject& object)
{
	_vecObject[(UINT)type].push_back(&object);
}


void CSector::RemoveObject(ESectorObjectType type, CObject& object)
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







CSectorGrid::CSectorGrid(int maxX, int maxY, int viewRange)
	: _maxX(maxX)
	, _maxY(maxY)
	, _viewRange(viewRange)
{

	// sector �迭 �޸� �Ҵ�
	_sector = new CSector*[_maxY];
	_sector[0] = (CSector*)malloc(sizeof(CSector) * _maxY * _maxX);
	for (int y = 0; y < _maxY; y++)
		_sector[y] = _sector[0] + (y * _maxX);

	// sector ������ ȣ��
	for (int y = 0; y < _maxY; y++)
	{
		for (int x = 0; x < _maxX; x++)
		{
			new (&_sector[y][x]) CSector(x, y);
		}
	}

	// sector�� �ֺ� sector ���(�ڽ�����)
	// �ݵ�� y�� ���� ���� ���Ͱ� �켱����, y�� ���� ���ٸ� x�� ���� ���� ���Ͱ� �켱���� ��ϵǾ�� ��.
	for (int y = 0; y < _maxY; y++)
	{
		for (int x = 0; x < _maxX; x++)
		{
			for (int aroundY = y - _viewRange; aroundY <= y + _viewRange; aroundY++)
			{
				for (int aroundX = x - _viewRange; aroundX <= x + _viewRange; aroundX++)
				{
					if (aroundY < 0 || aroundY >= _maxY || aroundX < 0 || aroundX >= _maxX)
						continue;

					_sector[y][x].AddAroundSector(&_sector[aroundY][aroundX]);
				}
			}
		}
	}
}


CSectorGrid::~CSectorGrid()
{
	// sector �Ҹ��� ȣ��
	for (int y = 0; y < _maxY; y++)
	{
		for (int x = 0; x < _maxX; x++)
		{
			(_sector[y][x]).~CSector();
		}
	}

	delete _sector[0];
	delete[] _sector;
}

int CSectorGrid::GetNumOfObject(int x, int y, ESectorObjectType type) const
{
	CheckCoordinate(x, y);
	return _sector[y][x].GetNumOfObject(type);
}

const std::vector<CObject*>& CSectorGrid::GetObjectVector(int x, int y, ESectorObjectType type) const
{
	CheckCoordinate(x, y);
	return _sector[y][x].GetObjectVector(type);
}

const std::vector<CSector*>& CSectorGrid::GetAroundSector(int x, int y) const
{
	CheckCoordinate(x, y);
	return _sector[y][x].GetAroundSector();
}

void CSectorGrid::AddObject(int x, int y, ESectorObjectType type, CObject& obj)
{
	obj.VirtualDummy();  // �����
	CheckCoordinate(x, y);
	_sector[y][x].AddObject(type, obj);
}


void CSectorGrid::RemoveObject(int x, int y, ESectorObjectType type, CObject& obj)
{
	obj.VirtualDummy();  // �����
	CheckCoordinate(x, y);
	_sector[y][x].RemoveObject(type, obj);
}


void CSectorGrid::CheckCoordinate(int x, int y) const
{
	if (x < 0 || x >= _maxX || y < 0 || y >= _maxY)
	{
		std::string message = "invalid sector coordinate: (";
		message += std::to_string(x) + ", " + std::to_string(y) + ")\n";
		throw std::out_of_range(message);
	}
}

