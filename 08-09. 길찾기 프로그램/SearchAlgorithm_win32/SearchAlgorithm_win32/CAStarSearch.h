#pragma once

#include <Windows.h>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "CGrid.h"


enum class eAStarNodeType
{
	OPEN,
	CLOSE,
	END
};

class AStarNode;

class CAStarSearch
{
public:
	CAStarSearch();
	~CAStarSearch();

public:
	// 파라미터 다시 설정
	void SetParam(const CGrid* pGrid);
	void Clear();

public:
	// search 시작
	bool Search();

	// step by step search 수행하기
	bool SearchStepByStep();  // 첫 시작
	bool SearchNextStep();    // 다음 step search

public:
	// get
	const std::vector<POINT>& GetPath() { return _vecPath; }
	const std::unordered_map<__int64, AStarNode*>& GetNodeInfo() { return _nodeList; }
	bool IsSucceeded() { return _isFoundDest; }


private:
	bool _isSearchStepByStep;
	bool _isFoundDest;

	const CGrid* _pGrid;
	AStarNode* _pStartNode;
	std::unordered_map<__int64, AStarNode*> _nodeList;
	std::unordered_set<__int64> _wallList;
	std::multimap<float, __int64> _mulmapFValue;
	int _numOfOpenNode;

	std::vector<POINT> _vecPath;  // 경로 저장 벡터	
};



class AStarNode
{
	friend class CAStarSearch;

public:
	AStarNode()
		: _x(0), _y(0), _pParent(nullptr), _valG(0.f), _valH(0.f), _valF(0.f), _type(eAStarNodeType::OPEN)
	{}
	AStarNode(int x, int y, AStarNode* pParent, float valG, float valH, eAStarNodeType type)
		: _x(x), _y(y), _pParent(pParent), _valG(valG), _valH(valH), _valF(valG + valH), _type(type)
	{}

	void SetMembers(int x, int y, AStarNode* pParent, float valG, float valH, eAStarNodeType type)
	{
		_x = x;
		_y = y;
		_pParent = pParent;
		_valG = valG;
		_valH = valH;
		_valF = valG + valH;
		_type = type;
	}

public:
	int _x;
	int _y;
	AStarNode* _pParent;
	float _valG;
	float _valH;
	float _valF;
	eAStarNodeType _type;
};