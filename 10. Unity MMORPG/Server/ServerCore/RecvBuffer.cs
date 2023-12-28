using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace ServerCore
{
    // 수신용 버퍼. 1개의 byte 배열을 가진다.
    // readPos와 writePos로 읽을 위치와 쓸 위치를 관리한다.
    // 코드 상에서 recv 하기 전 readPos와 writePos를 앞으로 당기는 작업을 미리 해야 한다. 이것은 Clean() 함수로 수행한다.
    // recv 한 뒤에는 받은 데이터 크기만큼 WritePos를 뒤로 밀어야 한다. 이것은 OnWrite() 함수로 수행한다.
    // read 한 뒤에는 읽은 데이터 크기만큼 ReadPos를 뒤로 밀어야 한다. 이것은 OnRead() 함수로 수행한다.
    public class RecvBuffer
    {
        ArraySegment<byte> _buffer;
        int _readPos;
        int _writePos;

        public RecvBuffer(int bufferSize)
        {
            _buffer = new ArraySegment<byte>(new byte[bufferSize], 0, bufferSize);
        }

        public int DataSize { get { return _writePos - _readPos; } }      // 버퍼내의 데이터 크기
        public int FreeSize { get { return _buffer.Count - _writePos; } } // 버퍼내의 남은 공간 크기


        public ArraySegment<byte> ReadSegment   // 버퍼의 데이터 영역을 리턴함
        {
            get { return new ArraySegment<byte>(_buffer.Array, _buffer.Offset + _readPos, DataSize); }
        }

        public ArraySegment<byte> WriteSegment   // 버퍼의 쓰기가능한 영역을 리턴함
        {
            get { return new ArraySegment<byte>(_buffer.Array, _buffer.Offset + _writePos, FreeSize); }
        }


        // readPos와 writePos를 최대한 앞으로 당긴다. 데이터 복사가 발생할 수 있다.
        public void Clean()
        {
            int dataSize = DataSize;
            if(dataSize == 0)
            {
                // 버퍼에 데이터가 없다면 커서 위치만 리셋
                _readPos = 0;
                _writePos = 0;
            }
            else
            {
                // 버퍼에 데이터가 있다면 데이터를 앞으로 복사
                Array.Copy(_buffer.Array, _buffer.Offset + _readPos, _buffer.Array, _buffer.Offset, dataSize);
                _readPos = 0;
                _writePos = dataSize;
            }
        }

        // 버퍼에서 데이터를 읽은 다음 호출할 함수. readPos를 뒤로 민다.
        public bool OnRead(int numOfBytes)
        {
            if (numOfBytes > DataSize)
                return false;
            _readPos += numOfBytes;
            return true;
        }

        // 버퍼에 데이터를 쓴 다음 호출할 함수. writePos를 뒤로 민다.
        public bool OnWrite(int numOfBytes)
        {
            if (numOfBytes > FreeSize)
                return false;
            _writePos += numOfBytes;
            return true;
        }
    }
}
