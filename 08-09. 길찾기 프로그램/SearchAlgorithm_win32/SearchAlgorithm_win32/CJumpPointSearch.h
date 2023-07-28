#pragma once

// CREATE_COLOR_GROUP�� ���ǵǾ� ������ cell ���� ���� �׷� ������ �����Ѵ�.
// ���� �׷� ������ _arr2CellColorGroup �� ����ȴ�.
#define CREATE_COLOR_GROUP


enum class eJPSNodeType
{
	NONE,
	OPEN,
	CLOSE,
	END
};

enum class eJPSNodeDirection
{
	NONE,
	UU,
	RU,
	RR,
	RD,
	DD,
	LD,
	LL,
	LU,
};


class JPSNode
{
public:
	int _x;
	int _y;
	JPSNode* _pParent;
	float _valG;
	float _valH;
	float _valF;
	eJPSNodeType _type;
	eJPSNodeDirection _direction;

public:
	JPSNode(int x, int y, JPSNode* pParent, float valG, float valH, eJPSNodeType type, eJPSNodeDirection direction)
		: _x(x), _y(y), _pParent(pParent), _valG(valG), _valH(valH), _valF(valG + valH)
		, _type(type), _direction(direction)
	{ }

	JPSNode()
		: _x(0), _y(0), _pParent(nullptr), _valG(0.f), _valH(0.f), _valF(0.f)
		, _type(eJPSNodeType::NONE), _direction(eJPSNodeDirection::NONE)
	{}

	void SetMembers(int x, int y, JPSNode* pParent, float valG, float valH, eJPSNodeType type, eJPSNodeDirection direction)
	{
		_x = x;
		_y = y;
		_pParent = pParent;
		_valG = valG;
		_valH = valH;
		_valF = valG + valH;
		_type = type;
		_direction = direction;
	}
};




class CJumpPointSearch
{
private:
	enum class eCellInfo
	{
		FREE,
		OPEN,
		CLOSE,
		WALL,
		DESTINATION,
		END = 127,
	};

private:
	POINT _ptStart;
	POINT _ptEnd;
	int _gridRows;
	int _gridCols;

	bool _isSearchStepByStep;      // step by step search ������ ����
	bool _isFoundDest;
	char** _arr2CellInfo;          // cell �Ӽ�. (char)eCellInfo �� ������ ������. 
	                               // �迭�� ũ��� (_gridRows + 2) * (_gridCols + 2) �ε�, ��ü grid�� �ѷ��δ� ������ wall�� ����� ���ؼ��̴�.
	                               // ������ �� �迭�� ����� ���� (x, y) ��ǥ�� (x+1, y+1) �� �����Ͽ� ����Ѵ�.

	JPSNode* _pStartNode;
	std::multimap<float, JPSNode*> _mulmapOpenList;  // open list
	std::vector<JPSNode*> _vecCloseList;             // close list
	std::vector<POINT> _vecPath;                     // ��� ���� ����	
	std::vector<POINT> _vecSmoothPath;               // Bresenham �˰������� ������ path



#ifdef CREATE_COLOR_GROUP
	unsigned char** _arr2CellColorGroup;   // cell ���� �׷�. _colorGroup �� ������ ������. �迭�� ũ��� _gridRows * _gridCols �̴�.
	unsigned char _colorGroup;
#endif


public:
	CJumpPointSearch();
	~CJumpPointSearch();

public:
	// �Ķ���� ����
	// _arr2Wall�� grid Wall ����. ���� 1�̸� wall, 0�̸� wall�� �ƴ�.
	void SetParam(int startRow, int startCol, int endRow, int endCol, const char* const* arr2Wall, int gridRows, int gridCols);

	// search
	bool Search();

	// step by step search
	bool SearchStepByStep();  // ����. ���� �� SetParam �Լ��� �Ķ���͸� �����ؾ���
	bool SearchNextStep();    // ���� step search

	// get
	const std::multimap<float, JPSNode*>& GetOpenList() { return _mulmapOpenList; }
	const std::vector<JPSNode*>& GetCloseList() { return _vecCloseList; }
	const std::vector<POINT>& GetPath() { return _vecPath; }
	const std::vector<POINT>& GetSmoothPath() { return _vecSmoothPath; }   // Bresenham �˰������� ������ path ���
	int GetGridRows() { return _gridRows; }
	int GetGridCols() { return _gridCols; }
	bool IsSucceeded() { return _isFoundDest; }
	
	

#ifdef CREATE_COLOR_GROUP
	const unsigned char* const* GetCellColorGroup() { return _arr2CellColorGroup; }
	int GetNumOfColorGroup() { return (int)_colorGroup; }
#endif
	

	// clear
	void Clear();

private:
	void _createCellInfoArray();
	void _createDestNode(JPSNode* pParentNode, float valG, eJPSNodeDirection direction);

	void _searchUpward(JPSNode* pParentNode);
	void _searchRightUp(JPSNode* pParentNode);
	void _searchRight(JPSNode* pParentNode);
	void _searchRightDown(JPSNode* pParentNode);
	void _searchDown(JPSNode* pParentNode);
	void _searchLeftDown(JPSNode* pParentNode);
	void _searchLeft(JPSNode* pParentNode);
	void _searchLeftUp(JPSNode* pParentNode);

	bool _lookRight(int x, int y);
	bool _lookUpward(int x, int y);
	bool _lookDown(int x, int y);
	bool _lookLeft(int x, int y);




};