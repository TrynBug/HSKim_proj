using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace ServerCore
{
    // SendBuffer에 TLS를 적용한 클래스
    public class SendBufferHelper
    {
        public static ThreadLocal<SendBuffer> CurrentBuffer = new ThreadLocal<SendBuffer>(() => { return null; });

        private static int _chunkSize = 65535 * 100;
        public static int ChunkSize { get { return _chunkSize; } set { _chunkSize = value; } }

        // 송신버퍼 얻기
        public static ArraySegment<byte> Open(int reserveSize)
        {
            // 첫 Open 호출 시 SendBuffer를 생성한다.
            if (CurrentBuffer.Value == null)
                CurrentBuffer.Value = new SendBuffer(ChunkSize);

            // 버퍼의 남은공간이 reserveSize보다 작을 경우 SendBuffer를 새로 생성하여 교체한다.
            // 이렇게 해도 되는 이유는 Session의 _sendQueue 에서 이전 버퍼를 참조하는 동안에는 이전 버퍼가 메모리에서 해제되지 않을 것이기 때문이다.
            if (CurrentBuffer.Value.FreeSize < reserveSize)
                CurrentBuffer.Value = new SendBuffer(ChunkSize);

            return CurrentBuffer.Value.Open(reserveSize);
        }

        // 송신버퍼에 write한 것을 확정하기
        public static ArraySegment<byte> Close(int usedSize)
        {
            return CurrentBuffer.Value.Close(usedSize);
        }
    }

    // 큰 1개의 byte 배열을 가지는 송신버퍼. 버퍼에 데이터를 write 하기만 한다.
    // 사용법:
    //   1. Open(희망크기) 함수로 write를 희망하는 크기 만큼의 ArraySegment를 얻는다. 이 크기에 모두 write할 필요는 없다.
    //   2. ArraySegment에 데이터를 복사한 다음, 복사한 크기를 기록한다.
    //   3. Close(복사한크기) 함수를 호출하여 버퍼의 usedSize를 전진시키고, 정확히 복사한 영역을 가르키는 ArraySegment를 새로 얻는다.
    //   4. 새로 받은 ArraySegment를 Send 한다.
    public class SendBuffer
    {
        byte[] _buffer;
        int _usedSize = 0;

        public int FreeSize { get { return _buffer.Length - _usedSize; } }  // 버퍼 남은 공간

        public SendBuffer(int chunkSize)
        {
            _buffer = new byte[chunkSize];
        }

        public ArraySegment<byte> Open(int reserveSize)
        {
            if (reserveSize > FreeSize)
                return null;

            return new ArraySegment<byte>(_buffer, _usedSize, reserveSize);
        }

        public ArraySegment<byte> Close(int usedSize)
        {
            ArraySegment<byte> segment = new ArraySegment<byte>(_buffer, _usedSize, usedSize);
            _usedSize += usedSize;
            return segment;

        }
    }
}
