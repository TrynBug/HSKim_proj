#include "CRingbuffer.h"
#include <iostream>

using namespace netserver;

#define min(a, b)  (((a) < (b)) ? (a) : (b)) 
#define max(a, b)  (((a) > (b)) ? (a) : (b)) 

#define RINGBUFFER_DEFAULT_SIZE 10000
#define RINGBUFFER_MAX_SIZE     30000 

CRingbuffer::CRingbuffer(void)
{
	_size = RINGBUFFER_DEFAULT_SIZE;
	_buf = new char[_size + 1];
	_front = _buf;
	_rear = _buf;
}

CRingbuffer::CRingbuffer(int size)
{
	_size = size;
	_buf = new char[_size + 1];
	_front = _buf;
	_rear = _buf;
}

CRingbuffer::~CRingbuffer()
{
	delete[] _buf;
}




/* ringbuffer�� �����͸� ����.
���� rear ��ġ���� �����͸� ����, ���� �� �� rear�� ��ġ�� �������� ������ ��ġ +1 �̴�.
@parameters
src : ������ ������, len : ������ byte ����
@return
���� �� �Է��� ������ byte ũ�⸦ ������.
ringbuffer�� ����� ������ ������ �����ϸ�, 0�� ������. */
int CRingbuffer::Enqueue(const char* src, int len)
{
	if (GetFreeSize() < len || len < 1)
		return 0;

	int directFreesize = GetDirectFreeSize();
	if (directFreesize < len)
	{
		memcpy(_rear, src, directFreesize);
		memcpy(_buf, src + directFreesize, len - directFreesize);
		_rear = _buf + len - directFreesize;
	}
	else
	{
		memcpy(_rear, src, len);
		_rear += len;
		if (_rear == _buf + _size + 1)
			_rear = _buf;
	}
	return len;
}




/* ���� ringbuffer�� ���� ���� byte ũ�⸦ ��´�.
@return
ringbuffer�� ���� ���� byte ũ�� */
int CRingbuffer::GetFreeSize(void)
{
	volatile char* front = _front; // enqueue ������ 1��, dequeue ������ 1���� ��Ƽ������ ȯ�濡�� FreeSize�� �����ϰ� ����ϱ� ���� front�� volatile�� ������
	if (front > _rear)
		return (int)(front - _rear - 1);
	else
		return (int)(_size + front - _rear);
}


/* ���� ringbuffer���� ������� ���� byte ũ�⸦ ��´�.
@return
ringbuffer�� ������� ���� byte ũ�� */
int CRingbuffer::GetUseSize(void)
{
	volatile char* rear = _rear;
	if (_front > rear)
		return (int)(_size + rear - _front + 1);
	else
		return (int)(rear - _front);
}

/* ���� ringbuffer���� rear�� ������ �� ������ ���� ���� byte ũ�⸦ ��´�.
@return
ringbuffer���� rear�� ������ �� ������ ���� ���� byte ũ�� */
int CRingbuffer::GetDirectFreeSize(void)
{
	volatile char* front = _front;
	if (front > _rear)
		return (int)(front - _rear - 1);
	else
	{
		if (front != _buf)
			return (int)(_buf + _size - _rear + 1);
		else
			return (int)(_buf + _size - _rear);
	}

}


/* ���� ringbuffer���� front�� ������ �� ������ ������� ������ byte ũ�⸦ ��´�.
@return
ringbuffer���� front�� ������ �� ������ ������� ���� byte ũ�� */
int CRingbuffer::GetDirectUseSize(void)
{
	volatile char* rear = _rear;
	if (_front > rear)
		return (int)(_buf + _size - _front + 1);
	else
		return (int)(rear - _front);
}





/* ringbuffer�� �����͸� dest�� �д´�.
���� front ��ġ���� �����͸� �а�, ������ŭ front�� ������Ų��.
@parameters
dest : ������ �б� ����, len : ���� byte ����
@return
���� �� ���� ������ byte ũ�⸦ ������.
ringbuffer ���� len ũ�⸸ŭ �����Ͱ� ���� ��� len ��ŭ ����.
ringbuffer ���� len ũ�⸸ŭ �����Ͱ� ���� ��� �����ִ� ��� �����͸� ����. */
int CRingbuffer::Dequeue(char* dest, int len)
{
	if (len < 1)
		return 0;

	int useSize = GetUseSize();
	if (useSize == 0)
		return 0;

	int readBytes = min(len, useSize);
	int directUseSize = GetDirectUseSize();
	if (directUseSize < readBytes)
	{
		memcpy(dest, _front, directUseSize);
		memcpy(dest + directUseSize, _buf, readBytes - directUseSize);
		_front = _buf + readBytes - directUseSize;
	}
	else
	{
		memcpy(dest, _front, readBytes);
		_front += readBytes;
		if (_front == _buf + _size + 1)
			_front = _buf;
	}
	return readBytes;
}




/* ringbuffer�� �����͸� dest�� �д´�.
���� front ��ġ���� �����͸� �а�, front�� ������Ű�� �ʴ´�.
@parameters
dest : ������ �б� ����, len : ���� byte ����
@return
���� �� ���� ������ byte ũ�⸦ ������.
ringbuffer ���� len ũ�⸸ŭ �����Ͱ� ���� ��� len ��ŭ ����.
ringbuffer ���� len ũ�⸸ŭ �����Ͱ� ���� ��� �����ִ� ��� �����͸� ����. */
int CRingbuffer::Peek(char* dest, int len)
{
	if (len < 1)
		return 0;

	int useSize = GetUseSize();
	if (useSize == 0)
		return 0;

	int readBytes = min(len, useSize);
	int directUseSize = GetDirectUseSize();
	if (directUseSize < readBytes)
	{
		memcpy(dest, _front, directUseSize);
		memcpy(dest + directUseSize, _buf, readBytes - directUseSize);
	}
	else
	{
		memcpy(dest, _front, readBytes);
	}
	return readBytes;
}



/* ringbuffer�� �����͸� dest�� �д´�. �� �� ���� ������� ũ�⸦ ������� �ʰ� �̸� ���޹޴´�.
���� front ��ġ���� �����͸� �а�, front�� ������Ű�� �ʴ´�.
@parameters
dest : ������ �б� ����, len : ���� byte ����
@return
���� �� ���� ������ byte ũ�⸦ ������.
ringbuffer ���� len ũ�⸸ŭ �����Ͱ� ���� ��� len ��ŭ ����.
ringbuffer ���� len ũ�⸸ŭ �����Ͱ� ���� ��� �����ִ� ��� �����͸� ����. */
int CRingbuffer::Peek(char* dest, int len, int useSize, int directUseSize)
{
	if (len < 1)
		return 0;

	if (useSize == 0)
		return 0;

	int readBytes = min(len, useSize);
	if (directUseSize < readBytes)
	{
		memcpy(dest, _front, directUseSize);
		memcpy(dest + directUseSize, _buf, readBytes - directUseSize);
	}
	else
	{
		memcpy(dest, _front, readBytes);
	}
	return readBytes;
}



/* front�� ��ġ�� len ��ŭ �ڷ� �ű��.
@parameters
len : front�� ��ġ�� �ڷ� �ű� byte ũ�� */
void CRingbuffer::MoveFront(int dist)
{
	_front = _buf + (_front - _buf + dist) % (_size + 1);
}

/* rear�� ��ġ�� len ��ŭ �ڷ� �ű��.
@parameters
len : rear�� ��ġ�� �ڷ� �ű� byte ũ�� */
void CRingbuffer::MoveRear(int dist)
{
	_rear = _buf + (_rear - _buf + dist) % (_size + 1);
}


/* ���۸� ����. */
void CRingbuffer::Clear(void)
{
	_front = _buf;
	_rear = _buf;
}

/* ���� ũ�⸦ �ø��ų� ���δ�.
@parameters
size : �ø��ų� ���� ������ ũ��
@return
�ø� ���� ���� ũ��. �ø� �� �ִ� �ִ� ũ��� RINGBUFFER_MAX_SIZE �̴�.
*/
int CRingbuffer::Resize(int size)
{
	int newSize = min(max(1, size), RINGBUFFER_MAX_SIZE);
	char* newBuf = new char[newSize + 1];
	int readBytes = Dequeue(newBuf, newSize);

	delete _buf;
	_size = newSize;
	_buf = newBuf;
	_front = newBuf;
	_rear = newBuf + readBytes;

	return newSize;
}