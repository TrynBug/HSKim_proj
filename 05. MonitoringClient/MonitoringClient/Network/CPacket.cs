using System;
using System.Buffers.Binary;
using System.Collections.Generic;
using System.Data;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms.VisualStyles;
using static System.Windows.Forms.VisualStyles.VisualStyleElement;

namespace MonitoringClient.Network
{
    [StructLayout(LayoutKind.Sequential, Pack = 1)]
    class PacketHeader
    {
        public static int Size { get; set; } = Marshal.SizeOf(typeof(PacketHeader));

        public byte Code { get; set; }
        public ushort Len { get; set; }
        public byte RandKey { get; set; }
        public byte CheckSum { get; set; }

        public byte[] Serialize()
        {
            byte[] buffer = new byte[Marshal.SizeOf(typeof(PacketHeader))];
            buffer[0] = Code;
            Array.Copy(BitConverter.GetBytes(Len), 0, buffer, 1, 2);
            buffer[3] = RandKey;
            buffer[4] = CheckSum;
            return buffer;
        }

        public void Deserialize(byte[] buffer, int offset)
        {
            Code = buffer[offset];
            Len = BitConverter.ToUInt16(buffer, offset + 1);
            RandKey = buffer[offset + 3];
            CheckSum = buffer[offset + 4];
        }
    };


    internal class CPacket
	{
        public CPacket()
		{
            _sizeHeader = PacketHeader.Size;
            _stream = new MemoryStream();
            Clear();
        }

		public void Clear()
		{
            _stream.Position = _sizeHeader;
            _readPosition = _sizeHeader;
            _writePosition = _sizeHeader;
        }


        /* Encode */
        public CPacket Encode(byte[] bytes, int offset, int count)
        {
            _stream.Write(bytes, offset, count);
            return this;
        }
        public CPacket Encode(string str)
        {
            _stream.Write(Encoding.Default.GetBytes(str));
            return this;
        }
        public CPacket Encode(byte value)
        {
			_stream.WriteByte(value);
            return this;
        }
        public CPacket Encode(short value)
		{
			_stream.Write(BitConverter.GetBytes(value));
            return this;
        }
        public CPacket Encode(ushort value)
        {
            _stream.Write(BitConverter.GetBytes(value));
            return this;
        }
        public CPacket Encode(int value)
        {
            _stream.Write(BitConverter.GetBytes(value));
            return this;
        }
        public CPacket Encode(uint value)
        {
            _stream.Write(BitConverter.GetBytes(value));
            return this;
        }
        public CPacket Encode(long value)
        {
            _stream.Write(BitConverter.GetBytes(value));
            return this;
        }
        public CPacket Encode(ulong value)
        {
            _stream.Write(BitConverter.GetBytes(value));
            return this;
        }
        public CPacket Encode(float value)
        {
            _stream.Write(BitConverter.GetBytes(value));
            return this;
        }
        public CPacket Encode(double value)
        {
            _stream.Write(BitConverter.GetBytes(value));
            return this;
        }


        /* Decode */
        public CPacket Decode(out byte value)
        {
            byte[] buffer = _stream.GetBuffer();
            value = buffer[_readPosition];
            _readPosition += sizeof(byte);
            return this;
        }
        public CPacket Decode(out short value)
        {
            byte[] buffer = _stream.GetBuffer();
            value = BitConverter.ToInt16(buffer, _readPosition);
            _readPosition += sizeof(short);
            return this;
        }
        public CPacket Decode(out ushort value)
        {
            byte[] buffer = _stream.GetBuffer();
            value = BitConverter.ToUInt16(buffer, _readPosition);
            _readPosition += sizeof(ushort);
            return this;
        }
        public CPacket Decode(out int value)
        {
            byte[] buffer = _stream.GetBuffer();
            value = BitConverter.ToInt32(buffer, _readPosition);
            _readPosition += sizeof(int);
            return this;
        }
        public CPacket Decode(out uint value)
        {
            byte[] buffer = _stream.GetBuffer();
            value = BitConverter.ToUInt32(buffer, _readPosition);
            _readPosition += sizeof(uint);
            return this;
        }
        public CPacket Decode(out long value)
        {
            byte[] buffer = _stream.GetBuffer();
            value = BitConverter.ToInt64(buffer, _readPosition);
            _readPosition += sizeof(long);
            return this;
        }
        public CPacket Decode(out ulong value)
        {
            byte[] buffer = _stream.GetBuffer();
            value = BitConverter.ToUInt64(buffer, _readPosition);
            _readPosition += sizeof(ulong);
            return this;
        }
        public CPacket Decode(out float value)
        {
            byte[] buffer = _stream.GetBuffer();
            value = BitConverter.ToSingle(buffer, _readPosition);
            _readPosition += sizeof(float);
            return this;
        }
        public CPacket Decode(out double value)
        {
            byte[] buffer = _stream.GetBuffer();
            value = BitConverter.ToDouble(buffer, _readPosition);
            _readPosition += sizeof(double);
            return this;
        }

        /* header */
        public CPacket EncodeHeader(PacketHeader header)
        {
            long position = _stream.Position;
            _stream.Position = 0;
            _stream.Write(header.Serialize());
            _stream.Position = position;
            return this;
        }

        public CPacket EncodeHeader(byte PacketCode)
        {
            PacketHeader header = new PacketHeader();
            header.Code = PacketCode;
            header.Len = (ushort)(_stream.Length - _sizeHeader);
            header.RandKey = (byte)(new Random()).Next(0, byte.MaxValue);
            header.CheckSum = 0;

            byte[] buffer = _stream.GetBuffer();
            for (int i = _sizeHeader; i < _stream.Length; i++)
                header.CheckSum += buffer[i];

            long position = _stream.Position;
            _stream.Position = 0;
            _stream.Write(header.Serialize());
            _stream.Position = position;
            return this;
        }

        public CPacket DecodeHeader(out PacketHeader header)
        {
            long position = _stream.Position;
            _stream.Position = 0;
            header = new PacketHeader();
            header.Deserialize(_stream.GetBuffer(), 0);
            _stream.Position = position;
            return this;
        }

        /* 암호화 */
        public void Encrypt(byte encryptKey)
        {
            byte[] buffer = _stream.GetBuffer();
            PacketHeader header = new PacketHeader();
            header.Deserialize(buffer, 0);

            byte valP = 0;
            byte prevData = 0;
            for(int i = _sizeHeader - 1; i < _stream.Length; i++)
            {
                valP = (byte)(buffer[i] ^ (valP + header.RandKey + i + 1 - (_sizeHeader - 1)));
                buffer[i] = (byte)(valP ^ (prevData + encryptKey + i + 1 - (_sizeHeader - 1)));
                prevData = buffer[i];
            }
        }

        public bool Decipher(byte encryptKey)
        {
            byte[] buffer = _stream.GetBuffer();
            PacketHeader header = new PacketHeader();
            header.Deserialize(buffer, 0);

            byte valP = 0;
            byte prevData = 0;
            byte prevValP = 0;
            for (int i = _sizeHeader - 1; i < _stream.Length; i++)
            {
                valP = (byte)(buffer[i] ^ (prevData + encryptKey + i + 1 - (_sizeHeader - 1)));
                prevData = buffer[i];
                buffer[i] = (byte)(valP ^ (prevValP + header.RandKey + i + 1 - (_sizeHeader - 1)));
                prevValP = valP;
            }
            header.Deserialize(buffer, 0);

            byte checkSum = 0;
            for (int i = _sizeHeader; i < _stream.Length; i++)
                checkSum += buffer[i];

            if (header.CheckSum == checkSum)
                return true;
            else
                return false;
        }


        /* get */
        public int GetBufferSize() { return (int)_stream.Length; }               // 버퍼 전체 크기
        //int GetUseSize() { return _offsetRear; }            // 사용중인 헤더+데이터 크기 (헤더는 항상 있다고 가정함)
        //int GetHeaderSize() { return _sizeHeader; }         // 사용중인 헤더 크기 (헤더는 항상 있다고 가정함)
        public ushort GetDataSize() { return Convert.ToUInt16(_stream.Length - _sizeHeader); }  // 사용중인 데이터 크기
		//char* GetRearPtr() { return _buf + _offsetRear; }   // 버퍼의 헤더포함 사용영역 끝 포인터
		//char* GetFrontPtr() { return _buf + _offsetFront; } // 버퍼의 헤더포함 아직 읽지않은영역 시작 포인터
		//char* GetDataPtr() { return _buf + _sizeHeader; }   // 버퍼의 데이터영역 시작 포인터
		//char* GetHeaderPtr() { return _buf; }               // 버퍼의 헤더영역 시작 포인터
        public byte[] GetBuffer() { return _stream.GetBuffer(); }

        ///* buffer control */
        //int MoveWritePos(int iSize);
        //int MoveReadPos(int iSize);


        private int _sizeHeader;
        private MemoryStream _stream;
        private int _readPosition;
        private int _writePosition;

    }
}
