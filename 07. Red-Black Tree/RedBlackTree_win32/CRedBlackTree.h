#pragma once

#include <stdlib.h>

template <typename _Kty, typename _Vty>
class CRedBlackTree
{
private:
	enum class eColor
	{
		BLACK = 0,
		RED
	};

	struct Pair
	{
		_Kty first;
		_Vty second;
	};

	struct Node
	{
		Pair pair;
		eColor color;
		Node* pParent;
		Node* pLeft;
		Node* pRight;

		Node(_Kty inKey, _Vty inVal, eColor color, Node* pParent)
			: pair{ inKey, inVal }, color(color), pParent(pParent), pLeft(_pNil), pRight(_pNil)
		{}
	};

	Node* _pRoot;
	size_t _numOfNodes;

	// Nil node
	static Node* _pNil;


public:
	CRedBlackTree();
	~CRedBlackTree();
	
public:
	size_t Size() { return _numOfNodes; }  // size ��ȯ

	bool Insert(_Kty key, _Vty val);  // insert
	
	class iterator;
	iterator Begin();                       // ������ȸ���� ù ��� iterator ��ȯ
	iterator End();                         // end iterator ��ȯ
	iterator Find(const _Kty& key);         // key�� �ش��ϴ� iterator ��ȯ
	iterator Erase(const iterator& iter);   // ��� ����

	void Clear();                           // ��� ��� ����

	iterator GetRoot() { return iterator(this, _pRoot); }  // root ��ȯ

private:
	Node* _getInorderSuccessor(Node* pNode);    // ���� �ļ��� ��ȯ
	Node* _delete(Node* pDeleteNode);           // ��� ����
	void _leftRotate(Node* pTargetNode);        // ��ȸ��
	void _rightRotate(Node* pTargetNode);       // ��ȸ��
	void _insertFixUp(Node* pInsertedNode);     // insert �� �뷱�����߱�
	void _deleteFixUp(Node* pTargetNode);       // delete �� �뷱�����߱�
	void _clear(Node* pNode);                   // ��� ��� ����



public:
	// ������ȸ iterator
	class iterator {
		friend class CRedBlackTree;

	protected:
		CRedBlackTree* _pOwner;
		Node* _pNode; // iterator�� ����Ű�� ���. null �� ��� end iterator

	public:
		iterator()
			: _pOwner(nullptr), _pNode(nullptr)
		{}
		iterator(CRedBlackTree* pOwner, Node* pNode)
			: _pOwner(pOwner), _pNode(pNode)
		{}

	public:
		bool operator == (const iterator& other) { return (_pOwner == _pOwner && _pNode == other._pNode); }
		bool operator != (const iterator& other) { return !(*this == other); }
		const Pair& operator * () const { return _pNode->pair; }     // * �����ڷ� <key, data> pair�� ������ �� �ְ� �Ѵ�.
		const Pair* operator -> () const { return &(_pNode->pair); } // -> �����ڷ� <key, data> pair�� ������ �� �ְ� �Ѵ�.

		// ������ȸ �������� ���� ���� �̵�
		iterator& operator ++ () {
			_pNode = _pOwner->_getInorderSuccessor(_pNode);
			return (*this);
		}

	public:
		iterator GetParent() const { return iterator(_pOwner, _pNode->pParent); }
		iterator GetLeftChild() const { return iterator(_pOwner, _pNode->pLeft); }
		iterator GetRightChild() const { return iterator(_pOwner, _pNode->pRight); }

		bool IsRed() const { return _pNode->color == eColor::RED; }
		bool IsBlack() const { return _pNode->color == eColor::BLACK; }
		bool IsNil() const { return _pNode == _pNil; }
		bool IsLeaf() const { return _pNode != _pNil && _pNode->pLeft == _pNil && _pNode->pRight == _pNil; }
		bool IsRoot() const { return _pNode == _pOwner->_pRoot; }
		bool IsLeftChild() const { return _pNode == _pNode->pParent->pLeft; }
		bool IsRightChild() const { return _pNode == _pNode->pParent->pRight; }
	};

};

// static ��� �ʱ�ȭ
template <typename _Kty, typename _Vty>
typename CRedBlackTree<_Kty, _Vty>::Node* CRedBlackTree<_Kty, _Vty>::_pNil = nullptr;


// ������
template <typename _Kty, typename _Vty>
CRedBlackTree<_Kty, _Vty>::CRedBlackTree()
	: _numOfNodes(0)
{
	// Nil ��� ����
	if(_pNil == nullptr)
	{ 
		_pNil = (Node*)malloc(sizeof(Node));
		_pNil->color = eColor::BLACK;
		_pNil->pParent = nullptr;
		_pNil->pLeft = nullptr;
		_pNil->pRight = nullptr;
	}

	// root �ʱ�ȭ
	_pRoot = _pNil;
}


// �Ҹ���
template <typename _Kty, typename _Vty>
CRedBlackTree<_Kty, _Vty>::~CRedBlackTree()
{
	Clear();
	delete _pNil;
}



template <typename _Kty, typename _Vty>
bool CRedBlackTree<_Kty, _Vty>::Insert(_Kty key, _Vty val)
{
	// root�� Nil�� ��� root�� ����
	if (_pRoot == _pNil)
	{
		_pRoot = new Node(key, val, eColor::BLACK, _pNil);
		_numOfNodes++;
		return true;
	}

	Node* pNode = _pRoot;
	while (true)
	{
		// key�� �����庸�� ���� ���
		if (key < pNode->pair.first)
		{
			// ���� �ڽ��� Nil�� ��� ���� �ڽĿ� �����Ѵ�.
			if (pNode->pLeft == _pNil)
			{
				pNode->pLeft = new Node(key, val, eColor::RED, pNode);
				_numOfNodes++;
				_insertFixUp(pNode->pLeft); // re-balance
				return true;
			}
			// �׷��� ������ ���� �ڽ� ���� �̵�
			else
			{
				pNode = pNode->pLeft;
			}
		}
		// key�� �����庸�� Ŭ ���
		else if (key > pNode->pair.first)
		{
			// ������ �ڽ��� Nil�� ��� ������ �ڽĿ� �����Ѵ�.
			if (pNode->pRight == _pNil)
			{
				pNode->pRight = new Node(key, val, eColor::RED, pNode);
				_numOfNodes++;
				_insertFixUp(pNode->pRight); // re-balance
				return true;
			}
			// �׷��� ������ ������ �ڽ� ���� �̵�
			else
			{
				pNode = pNode->pRight;
			}
		}
		// key�� ���� ���� ���� ��� �� ����
		else
		{
			pNode->pair.second = val;
			return false;
		}
	}
}




// ������ȸ ���� ���� ù node�� ����Ű�� iterator ��ȯ
// ���� ���� �Ʒ� node�� ����Ű�� iterator�� ��ȯ�Ѵ�.
template<typename _Kty, typename _Vty>
typename CRedBlackTree<_Kty, _Vty>::iterator CRedBlackTree<_Kty, _Vty>::Begin()
{
	Node* pNode = _pRoot;
	if (pNode == _pNil)
	{
		return End();
	}

	while (true)
	{
		if (pNode->pLeft == _pNil)
			break;
		pNode = pNode->pLeft;
	}
	return iterator(this, pNode);
}

// end iterator ��ȯ
template<typename _Kty, typename _Vty>
typename CRedBlackTree<_Kty, _Vty>::iterator CRedBlackTree<_Kty, _Vty>::End()
{
	return iterator(this, nullptr);
}


// tree���� key���� �ش��ϴ� node�� ã�Ƽ� iterator�� ��ȯ��.
template<typename _Kty, typename _Vty>
typename CRedBlackTree<_Kty, _Vty>::iterator CRedBlackTree<_Kty, _Vty>::Find(const _Kty& key)
{
	Node* pNode = _pRoot;
	while (true)
	{
		if (pNode == _pNil)
		{
			return iterator(this, nullptr); // ���� ���� node�� Nil�� ��� end iterator ��ȯ
		}
		else if (key < pNode->pair.first)
		{
			pNode = pNode->pLeft;
		}
		else if (key > pNode->pair.first)
		{
			pNode = pNode->pRight;
		}
		else if (key == pNode->pair.first)
		{
			return iterator(this, pNode); // key ���� ���ٸ� iterator ��ȯ
		}
	}
}


// ���޹��� iterator�� �ش��ϴ� ��带 �����Ѵ�.
template<typename _Kty, typename _Vty>
typename CRedBlackTree<_Kty, _Vty>::iterator CRedBlackTree<_Kty, _Vty>::Erase(const iterator& iter)
{
	if (End() == iter)
		return End();

	Node* pSuccessor = _delete(iter._pNode); // ��带 �����ϰ� ������ ����� �����ļ��ڸ� ����Ű�� ��带 �޴´�.
	return iterator(this, pSuccessor);
}


// ��� ��� ����
template<typename _Kty, typename _Vty>
void CRedBlackTree<_Kty, _Vty>::Clear()
{
	_clear(_pRoot);
	_pRoot = _pNil;
	_numOfNodes = 0;
}





// ���޹��� ����� �����ļ��ڸ� ã�´�.
template <typename _Kty, typename _Vty>
typename CRedBlackTree<_Kty, _Vty>::Node* CRedBlackTree<_Kty, _Vty>::_getInorderSuccessor(Node* pNode)
{
	Node* pSuccessor;
	// ������ �ڽ��� �ִ� ���, ������ �ڽ����� ���� ���� �ڽ��� ���� ������ ������
	if (pNode->pRight != _pNil)
	{
		pSuccessor = pNode->pRight;
		while (pSuccessor->pLeft != _pNil)
		{
			pSuccessor = pSuccessor->pLeft;
		}
	}
	// ������ �ڽ��� ���� ���, �ڽ��� �θ��� �����ڽ��� �� ���� ���� �ö�. �׶� �θ� �����ļ���
	else {
		pSuccessor = pNode;
		while (true)
		{
			// ���� root node���� �ö󰬴ٸ� ���� �ļ��ڴ� ����
			if (pSuccessor->pParent == _pNil)
			{
				pSuccessor = nullptr;
				break;
			}
			// ���� ���� ��尡 �θ��� ���� �ڽ��̸� �θ� �ļ�����
			else if (pSuccessor->pParent->pLeft == pSuccessor)
			{
				pSuccessor = pSuccessor->pParent;
				break;
			}
			// �׷��� ������ �θ� ���� �Ѿ
			else
			{
				pSuccessor = pSuccessor->pParent;
			}
		}
	}
	return pSuccessor;
}





// ��带 �����ϰ� ������ ����� �����ļ��ڸ� ����Ű�� ��带 ��ȯ�Ѵ�.
template <typename _Kty, typename _Vty>
typename CRedBlackTree<_Kty, _Vty>::Node* CRedBlackTree<_Kty, _Vty>::_delete(Node* pDeleteNode)
{
	// ������ų ����� ���� �ļ��� ��带 ã�Ƶд�.
	Node* pSuccessor = _getInorderSuccessor(pDeleteNode);

	// �ڽ��� 2���� ���
	if (pDeleteNode->pLeft != _pNil && pDeleteNode->pRight != _pNil)
	{
		// ������ �ڸ��� ���� �ļ����� ���� �����Ų��.
		pDeleteNode->pair = pSuccessor->pair;
		// ���� �ļ��� ��带 ����(fix up�� _delete �Լ� ������ �����)
		_delete(pSuccessor);
		// ���� ������ ��尡 ���� �ļ��ڰ� ��
		pSuccessor = pDeleteNode;
	}

	// �ڽ��� 1���̰� ���� �ڽĸ� �ִ� ���
	else if (pDeleteNode->pLeft != _pNil)
	{
		// ������ ��尡 root�� ��� �ڽĳ�带 ��Ʈ�� �����.
		if (pDeleteNode == _pRoot)
		{
			_pRoot = pDeleteNode->pLeft;
			_pRoot->pParent = _pNil;
		}
		// ���� �θ��� ���� �ڽ��� ���
		else if (pDeleteNode == pDeleteNode->pParent->pLeft)
		{
			pDeleteNode->pParent->pLeft = pDeleteNode->pLeft;
			pDeleteNode->pLeft->pParent = pDeleteNode->pParent;
		}
		// ���� �θ��� ������ �ڽ��� ���
		else
		{
			pDeleteNode->pParent->pRight = pDeleteNode->pLeft;
			pDeleteNode->pLeft->pParent = pDeleteNode->pParent;
		}

		// ������ ����� ���� BLACK �̶�� fix up
		if (pDeleteNode->color == eColor::BLACK)
		{
			_deleteFixUp(pDeleteNode->pLeft);
		}
		delete pDeleteNode;
		_numOfNodes--;
	}

	// �ڽ��� 1���̰� ������ �ڽĸ� �ִ� ���
	else if (pDeleteNode->pRight != _pNil)
	{
		// ������ ��尡 root�� ��� �ڽĳ�带 ��Ʈ�� �����.
		if (pDeleteNode == _pRoot)
		{
			_pRoot = pDeleteNode->pRight;
			_pRoot->pParent = _pNil;
		}
		// ���� �θ��� ���� �ڽ��� ���
		else if (pDeleteNode == pDeleteNode->pParent->pLeft)
		{
			pDeleteNode->pParent->pLeft = pDeleteNode->pRight;
			pDeleteNode->pRight->pParent = pDeleteNode->pParent;
		}
		// ���� �θ��� ������ �ڽ��� ���
		else
		{
			pDeleteNode->pParent->pRight = pDeleteNode->pRight;
			pDeleteNode->pRight->pParent = pDeleteNode->pParent;
		}

		// ������ ����� ���� BLACK �̶�� fix up
		if (pDeleteNode->color == eColor::BLACK)
		{
			_deleteFixUp(pDeleteNode->pRight);
		}
		delete pDeleteNode;
		_numOfNodes--;
	}

	// �ڽ��� �ϳ��� ���� ���
	else {
		// ������ ��尡 root�� ��� root ����
		if (pDeleteNode == _pRoot)
		{
			_pRoot = _pNil;
			delete pDeleteNode;
			_numOfNodes--;
			return pSuccessor;
		}
		// ���� �ڽ��� ���� �ڽ��� ��� �θ����� �����ڽ��� Nil�� �����Ѵ�
		else if (pDeleteNode == pDeleteNode->pParent->pLeft)
		{
			pDeleteNode->pParent->pLeft = _pNil;
		}
		// ���� �ڽ��� ������ �ڽ��� ��� �θ����� �������ڽ��� Nil�� �����Ѵ�
		else
		{
			pDeleteNode->pParent->pRight = _pNil;
		}

		// ������ ����� ���� BLACK �̶�� fix up
		if (pDeleteNode->color == eColor::BLACK)
		{
			_pNil->pParent = pDeleteNode->pParent;
			_deleteFixUp(_pNil);
		}
		delete pDeleteNode;
		_numOfNodes--;
	}

	return pSuccessor;
}






// target�� �������� ��ȸ���Ѵ�.
template <typename _Kty, typename _Vty>
void CRedBlackTree<_Kty, _Vty>::_leftRotate(Node* pTargetNode)
{
	Node* pRightSubtree = pTargetNode->pRight;

	// target�� ������ �ڽ��� right subtree�� ���� �ڽ����� �Ѵ�.
	pTargetNode->pRight = pRightSubtree->pLeft;
	if (pRightSubtree->pLeft != _pNil)
	{
		pRightSubtree->pLeft->pParent = pTargetNode;
	}
	// right subtree�� �θ� target�� �θ�� �ٲ۴�.
	pRightSubtree->pParent = pTargetNode->pParent;

	// target�� root ���ٸ� right subtree�� root�� ��.
	if (pTargetNode == _pRoot)
	{
		_pRoot = pRightSubtree;
	}
	// �׷��� �ʰ� target�� ���� �ڽ��̾��ٸ� target�� �θ��� �����ڽ��� right subtree�� ��
	else if (pTargetNode == pTargetNode->pParent->pLeft)
	{
		pTargetNode->pParent->pLeft = pRightSubtree;
	}
	// �׷��� �ʰ� target�� ������ �ڽ��̾��ٸ� target�� �θ��� �������ڽ��� right subtree�� ��
	else
	{
		pTargetNode->pParent->pRight = pRightSubtree;
	}

	// right subtree�� ���� �ڽ��� target����, target�� �θ� right subtree�� ����
	pRightSubtree->pLeft = pTargetNode;
	pTargetNode->pParent = pRightSubtree;
}





// target�� �������� ��ȸ���Ѵ�.
template <typename _Kty, typename _Vty>
void CRedBlackTree<_Kty, _Vty>::_rightRotate(Node* pTargetNode)
{
	Node* pLeftSubtree = pTargetNode->pLeft;

	// target�� ���� �ڽ��� left subtree�� ������ �ڽ����� �Ѵ�.
	pTargetNode->pLeft = pLeftSubtree->pRight;
	if (pLeftSubtree->pRight != _pNil)
	{
		pLeftSubtree->pRight->pParent = pTargetNode;
	}
	// left subtree�� �θ� target�� �θ�� �ٲ۴�.
	pLeftSubtree->pParent = pTargetNode->pParent;

	// target�� root ���ٸ� left subtree�� root�� ��.
	if (pTargetNode == _pRoot)
	{
		_pRoot = pLeftSubtree;
	}
	// �׷��� �ʰ� target�� ���� �ڽ��̾��ٸ� target�� �θ��� �����ڽ��� left subtree�� ��
	else if (pTargetNode == pTargetNode->pParent->pLeft)
	{
		pTargetNode->pParent->pLeft = pLeftSubtree;
	}
	// �׷��� �ʰ� target�� ������ �ڽ��̾��ٸ� target�� �θ��� �������ڽ��� left subtree�� ��
	else
	{
		pTargetNode->pParent->pRight = pLeftSubtree;
	}

	// left subtree�� ������ �ڽ��� target����, target�� �θ� left subtree�� ����
	pLeftSubtree->pRight = pTargetNode;
	pTargetNode->pParent = pLeftSubtree;
}





// insert �� �� tree�� �뷱���� �����.
template <typename _Kty, typename _Vty>
void CRedBlackTree<_Kty, _Vty>::_insertFixUp(Node* pInsertedNode)
{
	Node* pTargetNode = pInsertedNode;
	Node* pUncleNode;
	// �θ� RED �̸� ��� ������
	while (pTargetNode->pParent->color == eColor::RED)
	{
		// �θ� ���θ��� ���� �ڽ��� ���
		if(pTargetNode->pParent == pTargetNode->pParent->pParent->pLeft)
		{
			// ���� ��带 �����ص�
			pUncleNode = pTargetNode->pParent->pParent->pRight;
			// case 1: ���� ��尡 RED��
			if (pUncleNode->color == eColor::RED)
			{
				// �θ�, ����, ���θ� ����� ���� �ٲ�
				pTargetNode->pParent->color = eColor::BLACK;
				pUncleNode->color = eColor::BLACK;
				pTargetNode->pParent->pParent->color = eColor::RED;
				// target�� ���θ�� �ٲ�
				pTargetNode = pTargetNode->pParent->pParent;
			}
			// case 2: ���� ��尡 BLACK �̰�, target�� �θ�� �ݴ������
			else if (pTargetNode == pTargetNode->pParent->pRight)
			{
				// �θ� �������� left rotate
				pTargetNode = pTargetNode->pParent;
				_leftRotate(pTargetNode);
			}
			// case 3: ���� ��尡 BLACK �̰�, target�� �θ�� ����������
			else
			{
			// �θ�� ���θ��� ���� �ٲ�
			pTargetNode->pParent->color = eColor::BLACK;
			pTargetNode->pParent->pParent->color = eColor::RED;
			// ���θ� �������� right rotate
			_rightRotate(pTargetNode->pParent->pParent);
			}
		}
		// �θ� ���θ��� ������ �ڽ��� ���
		else
		{
		// ���� ��带 �����ص�
		pUncleNode = pTargetNode->pParent->pParent->pLeft;
		// case 1: ���� ��尡 RED��
		if (pUncleNode->color == eColor::RED)
		{
			// �θ�, ����, ���θ� ����� ���� �ٲ�
			pTargetNode->pParent->color = eColor::BLACK;
			pUncleNode->color = eColor::BLACK;
			pTargetNode->pParent->pParent->color = eColor::RED;
			// target�� ���θ�� �ٲ�
			pTargetNode = pTargetNode->pParent->pParent;
		}
		// case 2: ���� ��尡 BLACK �̰�, target�� �θ�� �ݴ������
		else if (pTargetNode == pTargetNode->pParent->pLeft)
		{
			// �θ� �������� right rotate
			pTargetNode = pTargetNode->pParent;
			_rightRotate(pTargetNode);
		}
		// case 3: ���� ��尡 BLACK �̰�, target�� �θ�� ����������
		else
		{
			// �θ�� ���θ��� ���� �ٲ�
			pTargetNode->pParent->color = eColor::BLACK;
			pTargetNode->pParent->pParent->color = eColor::RED;
			// ���θ� �������� left rotate
			_leftRotate(pTargetNode->pParent->pParent);
		}
		}
	}

	_pRoot->color = eColor::BLACK;
}


// delete �� �� tree�� �뷱���� �����.
template <typename _Kty, typename _Vty>
void CRedBlackTree<_Kty, _Vty>::_deleteFixUp(Node* pTargetNode)
{
	Node* pTarget = pTargetNode;
	Node* pParent;
	Node* pSibling;

	// case 1. target�� ����
	if (pTarget->color == eColor::RED)
	{
		pTarget->color = eColor::BLACK;
		return;
	}

	if (pTarget->pParent == _pNil)
		return;

	// case 2.1 target�� ���̰� ������ ������
	pParent = pTarget->pParent;
	if (pTarget == pParent->pLeft)
	{
		pSibling = pParent->pRight;
		if (pSibling->color == eColor::RED)
		{
			pParent->color = eColor::RED;
			pSibling->color = eColor::BLACK;
			_leftRotate(pParent);
			pSibling = pParent->pRight;
		}
	}
	else
	{
		pSibling = pParent->pLeft;
		if (pSibling->color == eColor::RED)
		{
			pParent->color = eColor::RED;
			pSibling->color = eColor::BLACK;
			_rightRotate(pParent);
			pSibling = pParent->pLeft;
		}
	}

	// case 2.2 target�� ���̰� ������ ���̰� ������ ���� �ڽ��� ��� ��
	if (pParent->color == eColor::BLACK
		&& pSibling->color == eColor::BLACK
		&& pSibling->pLeft->color == eColor::BLACK
		&& pSibling->pRight->color == eColor::BLACK)
	{
		pSibling->color = eColor::RED;
		_deleteFixUp(pParent);
		return;
	}


	// case 2.3 target�� �θ� �����̰� ������ ���̰� ������ �����ڽ��� ��, �������ڽ��� ��
	if (pParent->color == eColor::RED
		&& pSibling->color == eColor::BLACK
		&& pSibling->pLeft->color == eColor::BLACK
		&& pSibling->pRight->color == eColor::BLACK)
	{
		pSibling->color = eColor::RED;
		pParent->color = eColor::BLACK;
		return;
	}


	// case 2.4 target�� ������ ��, ������ ���� �ڽ��� ����, ������ ������ �ڽ��� ��
	if (pTarget == pParent->pLeft
		&& pSibling->color == eColor::BLACK
		&& pSibling->pLeft->color == eColor::RED
		&& pSibling->pRight->color == eColor::BLACK)
	{
		pSibling->color = eColor::RED;
		pSibling->pLeft->color = eColor::BLACK;
		_rightRotate(pSibling);
		pSibling = pParent->pRight;
	}
	else if (pTarget == pParent->pRight
		&& pSibling->color == eColor::BLACK
		&& pSibling->pRight->color == eColor::RED
		&& pSibling->pLeft->color == eColor::BLACK)
	{
		pSibling->color = eColor::RED;
		pSibling->pRight->color = eColor::BLACK;
		_leftRotate(pSibling);
		pSibling = pParent->pLeft;
	}

	// case 2.5 target�� ������ ��, ������ ������ �ڽ��� ����
	if (pTarget == pParent->pLeft
		&& pSibling->color == eColor::BLACK
		&& pSibling->pRight->color == eColor::RED)
	{
		pSibling->color = pParent->color;
		pParent->color = eColor::BLACK;
		pSibling->pRight->color = eColor::BLACK;
		_leftRotate(pParent);
	}
	else if (pTarget == pParent->pRight
		&& pSibling->color == eColor::BLACK
		&& pSibling->pLeft->color == eColor::RED)
	{
		pSibling->color = pParent->color;
		pParent->color = eColor::BLACK;
		pSibling->pLeft->color = eColor::BLACK;
		_rightRotate(pParent);
	}

}




// ��� ��� ����
template<typename _Kty, typename _Vty>
void CRedBlackTree<_Kty, _Vty>::_clear(Node* pNode)
{
	if (pNode == _pNil)
		return;

	_clear(pNode->pLeft);
	_clear(pNode->pRight);
	delete pNode;
}
