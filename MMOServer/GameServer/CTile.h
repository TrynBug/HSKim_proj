#pragma once

enum class ETileObjectType
{
	PLAYER,
	MONSTER,
	CRYSTAL,
	END
};

class CObject;

class CTile
{
public:
	CTile(int x, int y);
	virtual ~CTile();

	CTile(const CTile&) = delete;
	CTile(const CTile&&) = delete;

public:
	int GetX() const { return _x; }
	int GetY() const { return _y; }
	int GetNumOfObject(ETileObjectType type) const { return (int)_vecObject[(UINT)type].size(); }
	const std::vector<CObject*>& GetObjectVector(ETileObjectType type) const { return _vecObject[(UINT)type]; }

	void AddObject(ETileObjectType type, CObject& object);
	void RemoveObject(ETileObjectType type, CObject& object);

private:
	int _x;
	int _y;

	std::vector<CObject*> _vecObject[(UINT)ETileObjectType::END];    // type ∫∞ object ∫§≈Õ
};




class CTileGrid
{
public:
	CTileGrid(int maxX, int maxY);
	~CTileGrid();

	CTileGrid(const CTileGrid&) = delete;
	CTileGrid(const CTileGrid&&) = delete;

public:
	int GetNumOfObject(int x, int y, ETileObjectType type) const;
	const std::vector<CObject*>& GetObjectVector(int x, int y, ETileObjectType type) const;
	int GetMaxX() const { return _maxX; }
	int GetMaxY() const { return _maxY; }

	void AddObject(int x, int y, ETileObjectType type, CObject& obj);
	void RemoveObject(int x, int y, ETileObjectType type, CObject& obj);

private:
	void CheckCoordinate(int x, int y) const;

private:
	CTile** _tile;
	int _maxX;
	int _maxY;
};



