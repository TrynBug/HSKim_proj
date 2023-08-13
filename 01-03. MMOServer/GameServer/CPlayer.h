#pragma once

namespace gameserver
{

	class CPlayer : public CObject
	{
		friend class CGameServer;

	public:
		struct Character;
		struct Position;
		struct Account;

	public:
		CPlayer();
		virtual ~CPlayer();

	public:
		virtual void Update() override;

		/* init */
		void Init(__int64 sessionId);
		void SetAccountInfo(const char* userId, const char* userNick, const wchar_t* ip, unsigned short port);
		void SetCharacterInfo(BYTE characterType);
		void LoadCharacterInfo(BYTE characterType, float posX, float posY, int tileX, int tileY, USHORT rotation, int crystal, int hp, INT64 exp, USHORT level, BYTE die);

		/* get */
		const Character& GetCharacter() const { return _character; }
		const Position& GetPosition() const { return _pos; }
		const Account& GetAccountInfo() const { return _account; }
		__int64 GetSessionId() const { return _sessionId; }
		__int64 GetAccountNo() const { return _accountNo; }
		float GetPosX() const { return _pos.x; }
		float GetPosY() const { return _pos.y; }
		bool IsLogIn() const { return _bLogin; }
		bool IsAlive() const { return _character.state.bAlive; }
		bool IsMove() const { return _character.state.bMove; }
		bool IsJustDied() const { return _character.bJustDied; }
		bool IsJustStandUp() const { return _character.bJustStandUp; }
		bool IsSit() const { return _character.sit == 1; }

		/* set */
		void SetLogin(INT64 accountNo, char sessionKey[64]);
		void SetRespawn(int tileX, int tileY);
		void SetDead() { _character.state.bAlive = false; }
		void SetSit(bool bSit);
		void SetHeartBeatTime() { _lastHeartBeatTime = GetTickCount64(); }
		void SetDBUpdateTime() { _lastDBUpdateTime = GetTickCount64(); }

		/* logic */
		void MovePosition(float posX, float posY, unsigned short rotation, BYTE VKey, BYTE HKey);
		void AddCrystal(int amount);
		void Hit(int damage);

		/* packet buffer */
		void AddPacket(netlib_game::CPacket& packet);

	public:
		/* struct declaration */
		struct Account
		{
			std::wstring sIP;
			unsigned short port;
			WCHAR szId[20];
			WCHAR szNick[20];
			Account() : port(0), szId{ 0, }, szNick{ 0, } {}
		};

		struct Character
		{
			friend class CPlayer;
		public:
			BYTE characterType;

			int maxHp;
			int hp;
			int damageAttack1;
			int damageAttack2;

			INT64 exp;
			unsigned short level;
			int crystal;
			int oldCrystal;
			BYTE die;
			BYTE sit;
			BYTE respawn;
			BYTE VKey;
			BYTE HKey;

			unsigned short rotation;

			int amountSitHPRecovery;     // �ɱ� �����϶� �ʴ� ȸ���Ǵ� HP��
			int sitStartHp;              // �ɱ� �������� ���� HP
			ULONGLONG sitStartTime;     // �ɱ� ������ �ð�
			ULONGLONG lastSitHPRecoveryTime;  // �ɱ�� ���� HP�� ȸ���� ������ �ð�

			bool bJustDied;      // ��� ����
			bool bJustStandUp;   // ��� �Ͼ
			ULONGLONG deadTime;  // ���� �ð�

			struct State
			{
				bool bNewPlayer;    // ���� ������ ���� ����
				bool bAlive;
				bool bMove;
				bool bAttack;
				bool bHit;
			};
			State state;

		private:
			void Update();

			void LoadCharacterInfo(BYTE characterType, USHORT rotation, int crystal, int hp, INT64 exp, USHORT level, BYTE die);
			void SetRespawn();
			void AddCrystal(int amount);
			void Hit(int damage);
			void SetSit(bool bSit);
		};

		struct Position
		{
			friend class CPlayer;
		public:
			float x;
			float y;
			int tileX;
			int tileY;
			int sectorX;
			int sectorY;

			float x_old;
			float y_old;
			int tileX_old;
			int tileY_old;
			int sectorX_old;
			int sectorY_old;

		private:
			void MovePosition(float posX, float posY);
			void MoveTile(int tileX, int tileY);
			void ResetPosition(float posX, float posY);
			void ResetTile(int tileX, int tileY);
		};

	private:
		/* member variable */
		__int64 _sessionId;
		INT64	_accountNo;
		char	_sessionKey[64];	// ������ū

		Account _account;
		Character _character;
		Position _pos;

		bool _bLogin;                  // �α��ο���
		bool _bDisconnect;             // ������� ����
		ULONGLONG _lastHeartBeatTime;  // ���������� heart beat ���� �ð�

		ULONGLONG _lastDBUpdateTime;    // ���������� DB ������Ʈ�� �� �ð�
		std::vector<netlib_game::CPacket*> _vecPacketBuffer;
	};

}