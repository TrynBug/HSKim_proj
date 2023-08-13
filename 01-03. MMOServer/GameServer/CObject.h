#pragma once

namespace gameserver
{

	class CObject
	{
	public:
		CObject();
		virtual ~CObject() {}

	public:
		void Init();
		virtual void Update() = 0;

		INT64 GetClientId() const { return _clientId; }
		void SetClientId(INT64 clientId) { _clientId = clientId; }

	private:
		INT64 _clientId;   // object ���� ��ȣ
	private:
		static INT64 __sNextClientId;
	};

}