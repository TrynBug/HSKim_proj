#pragma once

namespace gameserver
{

	class CCrystal : public CObject
	{
	public:
		struct Position;
		struct Status;

	public:
		CCrystal();
		virtual ~CCrystal();
	public:
		void Init(float posX, float posY, BYTE crystalType);

		virtual void Update() override;

		/* get */
		bool IsAlive() const { return _bAlive; }
		const Position& GetPosition() const { return _pos; }
		const Status& GetStatus() const { return _status; }

		/* set */
		void SetDead();

	public:
		/* struct declaration */
		struct Position
		{
			float posX;
			float posY;
			int tileX;
			int tileY;
			int sectorX;
			int sectorY;

			void SetPosition(float posX, float posY);
		};

		struct Status
		{
			BYTE crystalType;
			int amount;
		};
	private:
		Position _pos;
		Status _status;

		bool _bAlive;
	};

}