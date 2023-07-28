#pragma once

enum class ESectorObjectType
{
	PLAYER,
	MONSTER,
	CRYSTAL,
	END
};

class CObject;

class CSector
{
	friend class CChatServer;

	int _x;
	int _y;

	std::vector<CObject*> _vecObject[(UINT)ESectorObjectType::END];    // type �� object ����
	CSector* _arrAroundSector[dfFIELD_NUM_MAX_AROUND_SECTOR];          // �ֺ� Ÿ��(�ڽ� ����)
	int _numAroundSector;           // �ֺ� Ÿ�� ��

public:
	CSector(int x, int y);
	virtual ~CSector();

public:
	int GetX() { return _x; }
	int GetY() { return _y; }
	int GetNumOfObject(ESectorObjectType type) { return (int)_vecObject[(UINT)type].size(); }
	std::vector<CObject*>& GetObjectVector(ESectorObjectType type) { return _vecObject[(UINT)type]; }
	CSector** GetAroundSector() { return _arrAroundSector; }
	int GetNumOfAroundSector() { return _numAroundSector; }

	void AddAroundSector(CSector* pSector);
	void AddObject(ESectorObjectType type, CObject* pObject);
	void RemoveObject(ESectorObjectType type, CObject* pObject);

};



