#pragma once

class CObject
{
public:
	CObject();
	virtual ~CObject() {}

	virtual void VirtualDummy() { __sVirtualCount++; } // 디버그
	static INT64 __sVirtualCount; // 디버그
public:
	void Init();
	virtual void Update() = 0;

	INT64 GetClientId() const { return _clientId; }
	void SetClientId(INT64 clientId) { _clientId = clientId; }

	__int64 __validation = 0x12345; // 디버그

private:
	INT64 _clientId;   // object 고유 번호
private:
	static INT64 __sNextClientId;


};

