@echo off
call :sub > output.txt 2>&1
exit /b

:sub
START /b /wait ../protoc-25.1-win64/bin/protoc.exe --proto_path=../../PacketGenerator --csharp_out=../../Common/Packet Protocol.proto
IF ERRORLEVEL 1 PAUSE

START ../../PacketGenerator/bin/Debug/PacketGenerator.exe ../../PacketGenerator/Protocol.proto
XCOPY /Y Protocol.cs "../../Server/Packet"
XCOPY /Y Protocol.cs "../../DummyClient/Packet"
XCOPY /Y Protocol.cs "../../../Client/Assets/Scripts/Packet"
XCOPY /Y ServerPacketManager.cs "../../Server/Packet"
XCOPY /Y ClientPacketManager.cs "../../DummyClient/Packet"
XCOPY /Y ClientPacketManager.cs "../../../Client/Assets/Scripts/Packet"