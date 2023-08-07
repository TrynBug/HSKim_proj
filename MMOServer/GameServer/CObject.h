#pragma once

class CObject
{
public:
	CObject();
	virtual ~CObject() {}

	virtual void VirtualDummy() { __sVirtualCount++; } // �����
	static INT64 __sVirtualCount; // �����
public:
	void Init();
	virtual void Update() = 0;

	INT64 GetClientId() const { return _clientId; }
	void SetClientId(INT64 clientId) { _clientId = clientId; }

	__int64 __validation = 0x12345; // �����

private:
	INT64 _clientId;   // object ���� ��ȣ
private:
	static INT64 __sNextClientId;


};

