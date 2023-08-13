#pragma once

#include <string>
#include <stdexcept>

namespace netlib_game
{

	class CPacket
	{
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
			_checkWriteOverflow(sizeof(char));
			_buf[_offsetRear] = chValue;
			_offsetRear++;
			return *this;
		}
		CPacket& operator << (unsigned char byValue)
		{
			_checkWriteOverflow(sizeof(unsigned char));
			*(unsigned char*)(_buf + _offsetRear) = byValue;
			_offsetRear++;
			return *this;
		}
		CPacket& operator << (short shValue)
		{
			_checkWriteOverflow(sizeof(short));
			*(short*)(_buf + _offsetRear) = shValue;
			_offsetRear += sizeof(short);
			return *this;
		}
		CPacket& operator << (unsigned short wValue)
		{
			_checkWriteOverflow(sizeof(unsigned short));
			*(unsigned short*)(_buf + _offsetRear) = wValue;
			_offsetRear += sizeof(unsigned short);
			return *this;
		}
		CPacket& operator << (int iValue)
		{
			_checkWriteOverflow(sizeof(int));
			*(int*)(_buf + _offsetRear) = iValue;
			_offsetRear += sizeof(int);
			return *this;
		}
		CPacket& operator << (unsigned int uValue)
		{
			_checkWriteOverflow(sizeof(unsigned int));
			*(unsigned int*)(_buf + _offsetRear) = uValue;
			_offsetRear += sizeof(unsigned int);
			return *this;
		}
		CPacket& operator << (long lValue)
		{
			_checkWriteOverflow(sizeof(long));
			*(long*)(_buf + _offsetRear) = lValue;
			_offsetRear += sizeof(long);
			return *this;
		}
		CPacket& operator << (unsigned long ulValue)
		{
			_checkWriteOverflow(sizeof(unsigned long));
			*(unsigned long*)(_buf + _offsetRear) = ulValue;
			_offsetRear += sizeof(unsigned long);
			return *this;
		}
		CPacket& operator << (__int64 i64Value)
		{
			_checkWriteOverflow(sizeof(__int64));
			*(__int64*)(_buf + _offsetRear) = i64Value;
			_offsetRear += sizeof(__int64);
			return *this;
		}
		CPacket& operator << (unsigned __int64 u64Value)
		{
			_checkWriteOverflow(sizeof(unsigned __int64));
			*(unsigned __int64*)(_buf + _offsetRear) = u64Value;
			_offsetRear += sizeof(unsigned __int64);
			return *this;
		}
		CPacket& operator << (float fValue)
		{
			_checkWriteOverflow(sizeof(float));
			*(float*)(_buf + _offsetRear) = fValue;
			_offsetRear += sizeof(float);
			return *this;
		}
		CPacket& operator << (double dValue)
		{
			_checkWriteOverflow(sizeof(double));
			*(double*)(_buf + _offsetRear) = dValue;
			_offsetRear += sizeof(double);
			return *this;
		}
		CPacket& operator << (void* pValue)
		{
			_checkWriteOverflow(sizeof(void*));
			*(void**)(_buf + _offsetRear) = pValue;
			_offsetRear += sizeof(void*);
			return *this;
		}


		/* take out (inline) */
		CPacket& operator >> (char& rchValue)
		{
			_checkReadOverflow(sizeof(char));
			rchValue = _buf[_offsetFront];
			_offsetFront++;
			return *this;
		}
		CPacket& operator >> (unsigned char& rbyValue)
		{
			_checkReadOverflow(sizeof(unsigned char));
			rbyValue = *(unsigned char*)(_buf + _offsetFront);
			_offsetFront++;
			return *this;
		}
		CPacket& operator >> (short& rshValue)
		{
			_checkReadOverflow(sizeof(short));
			rshValue = *(short*)(_buf + _offsetFront);
			_offsetFront += sizeof(short);
			return *this;
		}
		CPacket& operator >> (unsigned short& rwValue)
		{
			_checkReadOverflow(sizeof(unsigned short));
			rwValue = *(unsigned short*)(_buf + _offsetFront);
			_offsetFront += sizeof(unsigned short);
			return *this;
		}
		CPacket& operator >> (int& riValue)
		{
			_checkReadOverflow(sizeof(int));
			riValue = *(int*)(_buf + _offsetFront);
			_offsetFront += sizeof(int);
			return *this;
		}
		CPacket& operator >> (unsigned int& ruValue)
		{
			_checkReadOverflow(sizeof(unsigned int));
			ruValue = *(unsigned int*)(_buf + _offsetFront);
			_offsetFront += sizeof(unsigned int);
			return *this;
		}
		CPacket& operator >> (long& rlValue)
		{
			_checkReadOverflow(sizeof(long));
			rlValue = *(long*)(_buf + _offsetFront);
			_offsetFront += sizeof(long);
			return *this;
		}
		CPacket& operator >> (unsigned long& rulValue)
		{
			_checkReadOverflow(sizeof(unsigned long));
			rulValue = *(unsigned long*)(_buf + _offsetFront);
			_offsetFront += sizeof(unsigned long);
			return *this;
		}
		CPacket& operator >> (__int64& ri64Value)
		{
			_checkReadOverflow(sizeof(__int64));
			ri64Value = *(__int64*)(_buf + _offsetFront);
			_offsetFront += sizeof(__int64);
			return *this;
		}
		CPacket& operator >> (unsigned __int64& ru64Value)
		{
			_checkReadOverflow(sizeof(unsigned __int64));
			ru64Value = *(unsigned __int64*)(_buf + _offsetFront);
			_offsetFront += sizeof(unsigned __int64);
			return *this;
		}
		CPacket& operator >> (float& rfValue)
		{
			_checkReadOverflow(sizeof(float));
			rfValue = *(float*)(_buf + _offsetFront);
			_offsetFront += sizeof(float);
			return *this;
		}
		CPacket& operator >> (double& rdValue)
		{
			_checkReadOverflow(sizeof(double));
			rdValue = *(double*)(_buf + _offsetFront);
			_offsetFront += sizeof(double);
			return *this;
		}
		CPacket& operator >> (void** rpValue)
		{
			_checkReadOverflow(sizeof(void*));
			*rpValue = *(void**)(_buf + _offsetFront);
			_offsetFront += sizeof(void*);
			return *this;
		}

		/* put arbitrary data (inline) */
		void PutData(const char* data, int len)
		{
			_checkWriteOverflow(len);
			memcpy(_buf + _offsetRear, data, len);
			_offsetRear += len;
		}

		/* take arbitrary data (inline) */
		void TakeData(char* buffer, int len)
		{
			_checkReadOverflow(len);
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
		long GetUseCount() { return _useCount; }

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

	private:
		inline void _checkWriteOverflow(int sizeWrite)
		{
			if (_offsetRear + sizeWrite >= _size)
				throw std::overflow_error(std::string("Packet buffer overflow has occurred during write size ") + std::to_string(sizeWrite) + " data.\n");
		}
		inline void _checkReadOverflow(int sizeRead)
		{
			if (_offsetFront + sizeRead > _offsetRear)
				throw std::overflow_error(std::string("Packet buffer overflow has occurred during read size ") + std::to_string(sizeRead) + " data.\n");
		}

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
	};

}