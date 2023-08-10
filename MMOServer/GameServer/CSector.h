#pragma once

namespace gameserver
{

	enum class ESectorObjectType
	{
		PLAYER,
		MONSTER,
		CRYSTAL,
		END
	};

	class CSector
	{
		friend class CSectorGrid;

	public:
		CSector(int x, int y);
		virtual ~CSector();

		CSector(const CSector&) = delete;
		CSector(const CSector&&) = delete;

	public:
		int GetX() { return _x; }
		int GetY() { return _y; }
		int GetNumOfObject(ESectorObjectType type) { return (int)_vecObject[(UINT)type].size(); }
		const std::vector<CObject_t>& GetObjectVector(ESectorObjectType type) { return _vecObject[(UINT)type]; }
		const std::vector<CSector*>& GetAroundSector() { return _vecAroundSector; }

		void AddObject(ESectorObjectType type, CObject_t pObject);
		void RemoveObject(ESectorObjectType type, CObject_t pObject);

	private:
		void AddAroundSector(CSector* pSector);

	private:
		int _x;
		int _y;

		std::vector<CObject_t> _vecObject[(UINT)ESectorObjectType::END];    // type 별 object 벡터
		std::vector<CSector*> _vecAroundSector;          // 주변 섹터(자신 포함)
	};



	class CSectorGrid
	{
	public:
		CSectorGrid(int maxX, int maxY, int viewRange);
		~CSectorGrid();

		CSectorGrid(const CSectorGrid&) = delete;
		CSectorGrid(const CSectorGrid&&) = delete;

	public:
		int GetMaxX() const { return _maxX; }
		int GetMaxY() const { return _maxY; }
		int GetNumOfObject(int x, int y, ESectorObjectType type) const;
		const std::vector<CObject_t>& GetObjectVector(int x, int y, ESectorObjectType type) const;
		const std::vector<CSector*>& GetAroundSector(int x, int y) const;

		void AddObject(int x, int y, ESectorObjectType type, CObject_t pObj);
		void RemoveObject(int x, int y, ESectorObjectType type, CObject_t pObj);

	private:
		void CheckCoordinate(int x, int y) const;

	private:
		CSector** _sector;
		int _maxX;
		int _maxY;
		int _viewRange;
	};


}