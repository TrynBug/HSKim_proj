#pragma once

namespace netserver
{


class CPacket
{
private:
	char* _buf;
	int _size;
	int _sizeHeader;
	int _offsetFront;
	int _offsetRear;
	bool _isHeaderSet; // ��� ���� ����. PutHeader �Լ� ȣ��� true����
	bool _isEncoded;   // ��ȣȭ ����. ��ȣȭ �� �������� �������־����
	long _useCount;

	static int _sDefaultPacketSize;  // ��Ŷ�� �⺻ ���� ������(�⺻��: 10000)

public:
	CPacket();
	~CPacket();

public:
	/* ��Ŷ �⺻ ũ�� (static) */
	static int GetDefaultPacketSize() { return _sDefaultPacketSize; }
	static void SetDefaultPacketSize(int defaultPacketSize) { _sDefaultPacketSize = defaultPacketSize; }

	/* �ʱ�ȭ */
	void Init(int sizeHeader);

	/* put (inline) */
	CPacket& operator << (char chValue)
	{
		_buf[_offsetRear] = chValue;
		_offsetRear++;
		return *this;
	}
	CPacket& operator << (unsigned char byValue)
	{
		*(unsigned char*)(_buf + _offsetRear) = byValue;
		_offsetRear++;
		return *this;
	}
	CPacket& operator << (short shValue)
	{
		*(short*)(_buf + _offsetRear) = shValue;
		_offsetRear += sizeof(short);
		return *this;
	}
	CPacket& operator << (unsigned short wValue)
	{
		*(unsigned short*)(_buf + _offsetRear) = wValue;
		_offsetRear += sizeof(unsigned short);
		return *this;
	}
	CPacket& operator << (int iValue)
	{
		*(int*)(_buf + _offsetRear) = iValue;
		_offsetRear += sizeof(int);
		return *this;
	}
	CPacket& operator << (unsigned int uValue)
	{
		*(unsigned int*)(_buf + _offsetRear) = uValue;
		_offsetRear += sizeof(unsigned int);
		return *this;
	}
	CPacket& operator << (long lValue)
	{
		*(long*)(_buf + _offsetRear) = lValue;
		_offsetRear += sizeof(long);
		return *this;
	}
	CPacket& operator << (unsigned long ulValue)
	{
		*(unsigned long*)(_buf + _offsetRear) = ulValue;
		_offsetRear += sizeof(unsigned long);
		return *this;
	}
	CPacket& operator << (__int64 i64Value)
	{
		*(__int64*)(_buf + _offsetRear) = i64Value;
		_offsetRear += sizeof(__int64);
		return *this;
	}
	CPacket& operator << (unsigned __int64 u64Value)
	{
		*(unsigned __int64*)(_buf + _offsetRear) = u64Value;
		_offsetRear += sizeof(unsigned __int64);
		return *this;
	}
	CPacket& operator << (float fValue)
	{
		*(float*)(_buf + _offsetRear) = fValue;
		_offsetRear += sizeof(float);
		return *this;
	}
	CPacket& operator << (double dValue)
	{
		*(double*)(_buf + _offsetRear) = dValue;
		_offsetRear += sizeof(double);
		return *this;
	}
	CPacket& operator << (void* pValue)
	{
		*(void**)(_buf + _offsetRear) = pValue;
		_offsetRear += sizeof(void*);
		return *this;
	}


	/* take out (inline) */
	CPacket& operator >> (char& rchValue)
	{
		rchValue = _buf[_offsetFront];
		_offsetFront++;
		return *this;
	}
	CPacket& operator >> (unsigned char& rbyValue)
	{
		rbyValue = *(unsigned char*)(_buf + _offsetFront);
		_offsetFront++;
		return *this;
	}
	CPacket& operator >> (short& rshValue)
	{
		rshValue = *(short*)(_buf + _offsetFront);
		_offsetFront += sizeof(short);
		return *this;
	}
	CPacket& operator >> (unsigned short& rwValue)
	{
		rwValue = *(unsigned short*)(_buf + _offsetFront);
		_offsetFront += sizeof(unsigned short);
		return *this;
	}
	CPacket& operator >> (int& riValue)
	{
		riValue = *(int*)(_buf + _offsetFront);
		_offsetFront += sizeof(int);
		return *this;
	}
	CPacket& operator >> (unsigned int& ruValue)
	{
		ruValue = *(unsigned int*)(_buf + _offsetFront);
		_offsetFront += sizeof(unsigned int);
		return *this;
	}
	CPacket& operator >> (long& rlValue)
	{
		rlValue = *(long*)(_buf + _offsetFront);
		_offsetFront += sizeof(long);
		return *this;
	}
	CPacket& operator >> (unsigned long& rulValue)
	{
		rulValue = *(unsigned long*)(_buf + _offsetFront);
		_offsetFront += sizeof(unsigned long);
		return *this;
	}
	CPacket& operator >> (__int64& ri64Value)
	{
		ri64Value = *(__int64*)(_buf + _offsetFront);
		_offsetFront += sizeof(__int64);
		return *this;
	}
	CPacket& operator >> (unsigned __int64& ru64Value)
	{
		ru64Value = *(unsigned __int64*)(_buf + _offsetFront);
		_offsetFront += sizeof(unsigned __int64);
		return *this;
	}
	CPacket& operator >> (float& rfValue)
	{
		rfValue = *(float*)(_buf + _offsetFront);
		_offsetFront += sizeof(float);
		return *this;
	}
	CPacket& operator >> (double& rdValue)
	{
		rdValue = *(double*)(_buf + _offsetFront);
		_offsetFront += sizeof(double);
		return *this;
	}
	CPacket& operator >> (void** rpValue)
	{
		*rpValue = *(void**)(_buf + _offsetFront);
		_offsetFront += sizeof(void*);
		return *this;
	}

	/* put arbitrary data (inline) */
	void PutData(const char* data, int len)
	{
		memcpy(_buf + _offsetRear, data, len);
		_offsetRear += len;
	}

	/* take arbitrary data (inline) */
	void TakeData(char* buffer, int len)
	{
		memcpy(buffer, _buf + _offsetFront, len);
		_offsetFront += len;
	}

	/* put header (inline) */
	void PutHeader(const char* header)
	{
		memcpy(_buf, header, _sizeHeader);
		_isHeaderSet = true;
	}

	/* get */
	int GetBufferSize() { return _size; }               // ���� ��ü ũ��
	int GetUseSize() { return _offsetRear; }            // ������� ���+������ ũ�� (����� �׻� �ִٰ� ������)
	int GetHeaderSize() { return _sizeHeader; }         // ������� ��� ũ�� (����� �׻� �ִٰ� ������)
	int GetDataSize() { return _offsetRear - _sizeHeader; }  // ������� ������ ũ��
	char* GetRearPtr() { return _buf + _offsetRear; }   // ������ ������� ��뿵�� �� ������
	char* GetFrontPtr() { return _buf + _offsetFront; } // ������ ������� ���� ������������ ���� ������
	char* GetDataPtr() { return _buf + _sizeHeader; }   // ������ �����Ϳ��� ���� ������
	char* GetHeaderPtr() { return _buf; }               // ������ ������� ���� ������
	bool IsHeaderSet() { return _isHeaderSet; }         // ��� ���� ����
	bool IsEncoded() { return _isEncoded; }             // ��ȣȭ �Ϸ� ����

	/* buffer control */
	void Clear();
	int MoveWritePos(int iSize);
	int MoveReadPos(int iSize);

	/* use count */
	long AddUseCount();
	long AddUseCount(int value);
	long SubUseCount();
	
	


	/* ��ȣȭ ���� */
	void SetEncoded() { _isEncoded = true; }
	void SetDecoded() { _isEncoded = false; }

	/* �޸�Ǯ (static) */
	static CPacket* AllocPacket();
	static void FreePacket(CPacket* pPacket);
	static void RegeneratePacketPool(int packetSize, int initialPoolSize, int chunkSize);

	/* �޸�Ǯ �����Ȳ (static) */
	static int GetPoolSize();
	static int GetAllocCount();
	static int GetActualAllocCount();
	static int GetFreeCount();
};





}