#include "CAStarSearch.h"
#include "CMemoryPool.h"

#define MAKE_KEY(high, low) (((__int64)(high) << 32) | (__int64)(low))
#define GET_X_VALUE(value) ((int)(value >> 32))
#define GET_Y_VALUE(value) ((int)(value))


CMemoryPool<AStarNode> g_pool(0, true, false);



CAStarSearch::CAStarSearch()
	: _pGrid(nullptr)
	, _pStartNode(nullptr)
	, _isSearchStepByStep(false)
	, _isFoundDest(false)
	, _numOfOpenNode(0)
{
}


CAStarSearch::~CAStarSearch()
{
	Clear();
}


void CAStarSearch::SetParam(const CGrid* pGrid)
{
	_pGrid = pGrid;
	_isSearchStepByStep = false;
	_isFoundDest = false;

	Clear();
}


bool CAStarSearch::Search()
{
	SearchStepByStep();
	while (!SearchNextStep())
	{
		continue;
	}

	return true;
}

bool CAStarSearch::SearchStepByStep()
{
	Clear();

	// wall list�� �� ��ǥ �Է�
	const EGridState* const* _arr2Grid = _pGrid->_getGrid();
	int numGridRows = _pGrid->GetNumRows();
	int numGridCols = _pGrid->GetNumCols();
	for (int row = 0; row < numGridRows; row++)
	{
		for (int col = 0; col < numGridCols; col++)
		{
			if (_arr2Grid[row][col] == EGridState::WALL)
			{
				_wallList.insert(MAKE_KEY(col, row));
			}
		}
	}

	// ���� ��� ����
	RC rcStart = _pGrid->GetStartRC();
	RC rcEnd = _pGrid->GetEndRC();
	_pStartNode = g_pool.Alloc();
	_pStartNode->SetMembers(rcStart.col, rcStart.row, nullptr
		, 0.f, (float)(abs(rcEnd.row - rcStart.row) + abs(rcEnd.col - rcStart.col))
		, eAStarNodeType::OPEN);

	// node list�� ���
	_nodeList.insert(std::make_pair(MAKE_KEY(rcStart.col, rcStart.row), _pStartNode));
	_mulmapFValue.insert(std::make_pair(_pStartNode->_valF, MAKE_KEY(rcStart.col, rcStart.row)));
	_numOfOpenNode++;

	// step by step search ������ üũ
	_isSearchStepByStep = true;

	return true;
}


bool CAStarSearch::SearchNextStep()
{
	if (!_isSearchStepByStep)
		return true;

	// ���� open node�� ������ ����
	if (_numOfOpenNode == 0)
	{
		_isSearchStepByStep = false;
		return true;
	}

	// F value�� ���� ���� open ����� ��ǥ ȹ��
	auto iterFValue = _mulmapFValue.begin();
	__int64 keyParent = (*iterFValue).second;
	int x = GET_X_VALUE(keyParent);
	int y = GET_Y_VALUE(keyParent);

	// �ش� ��ǥ�� ��� ȹ��
	AStarNode* pParentNode = (*_nodeList.find(keyParent)).second;

	// �湮�� ��带 open ���� close�� ����
	pParentNode->_type = eAStarNodeType::CLOSE;
	_mulmapFValue.erase(iterFValue);
	_numOfOpenNode--;

	// ��ǥ�� �������� �����ϴٸ� ����
	RC rcEnd = _pGrid->GetEndRC();
	if (x == rcEnd.col && y == rcEnd.row)
	{
		// ���������� ������������ ��θ� ������
		AStarNode* pNextNode = pParentNode;
		while (pNextNode != _pStartNode)
		{
			_vecPath.push_back(POINT{ pNextNode->_x, pNextNode->_y });
			pNextNode = pNextNode->_pParent;
		}
		_vecPath.push_back(POINT{ pNextNode->_x, pNextNode->_y });

		_isSearchStepByStep = false;
		_isFoundDest = true;
		return true;
	}


	// ���� ��� search
	// 8�� ���� ��ǥ, G�� ����ġ ����
	// ��, ������, ����, ���ʾƷ�, �Ʒ�, �����ʾƷ�, ������, ��������
	int arrX[8] = { x     , x - 1, x - 1, x - 1, x    , x + 1, x + 1, x + 1 };
	int arrY[8] = { y - 1 , y - 1, y    , y + 1, y + 1, y + 1, y    , y - 1 };
	float arrG[8] = { 1.f, 1.5f, 1.f, 1.5f, 1.f, 1.5f, 1.f, 1.5f };

	// 8�� ���� search
	__int64 keyNew;
	AStarNode* pNewNode;
	AStarNode* pOpenNode;
	for (int i = 0; i < 8; i++)
	{
		if (arrX[i] < 0 || arrX[i] >= _pGrid->GetNumCols()
			|| arrY[i] < 0 || arrY[i] >= _pGrid->GetNumRows())
		{
			continue;
		}

		// node list, wall list���� ��ǥ�� ��ã������ ��带 �����ϰ�,
		// node list ���� ��ǥ�� ã�Ҵµ� �ش� ��尡 open �̶�� F-value�� �ٽ��ѹ� �˻���
		keyNew = MAKE_KEY(arrX[i], arrY[i]);
		auto iterNode = _nodeList.find(keyNew);


		// node list���� ��ǥ�� ã�� ����
		if (iterNode == _nodeList.end())
		{
			// wall�� �ƴ� ��� ��� ����
			if (_wallList.find(keyNew) == _wallList.end())
			{
				// ��� ����
				pNewNode = g_pool.Alloc();
				pNewNode->SetMembers(arrX[i], arrY[i], pParentNode
					, pParentNode->_valG + arrG[i]
					, (float)(abs(arrX[i] - rcEnd.row) + abs(arrY[i] - rcEnd.col))
					, eAStarNodeType::OPEN);

				// node list�� ���
				_nodeList.insert(std::make_pair(keyNew, pNewNode));
				_mulmapFValue.insert(std::make_pair(pNewNode->_valF, keyNew));
				_numOfOpenNode++;
			}
		}
		// node list���� ��ǥ�� ã��
		else
		{
			pOpenNode = (*iterNode).second;
			// node list���� ��ǥ�� ã�Ҵµ� close ������
			if (pOpenNode->_type == eAStarNodeType::CLOSE)
			{
				continue;
			}
			// node list���� ��ǥ�� ã�Ҵµ� open ������
			else
			{
				float newValG = pParentNode->_valG + arrG[i];
				// ���� ���� ����� G���� ���� ����� G������ �۴ٸ� ���� ����� parent�� ��ü
				if (newValG < pOpenNode->_valG)
				{
					pOpenNode->_pParent = pParentNode;
					pOpenNode->_valG = newValG;
					pOpenNode->_valF = newValG + pOpenNode->_valH;
				}
			}
		}
	}

	return false;
}






// ������ ��� ��带 �����.
void CAStarSearch::Clear()
{
	for (auto iter = _nodeList.begin(); iter != _nodeList.end(); ++iter)
	{
		g_pool.Free((*iter).second);
	}

	_nodeList.clear();
	_wallList.clear();
	_mulmapFValue.clear();
	_vecPath.clear();
	_numOfOpenNode = 0;
}


