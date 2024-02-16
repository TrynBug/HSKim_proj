using Google.Protobuf.Protocol;
using Server.Data;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    public class Equipment
    {
        public Item Back { get; private set; }
        public Item Cloth { get; private set; }
        public Item Armor { get; private set; }
        public Item Helmet { get; private set; }
        public Item WeaponLeft { get; private set; }
        public Item WeaponRight { get; private set; }
        public Item Horse { get; private set; }

        public int Damage { get { return Back.damage + Cloth.damage + Armor.damage + Helmet.damage + WeaponLeft.damage + WeaponRight.damage + Horse.damage; } }
        public float RangeX { get { return Back.rangeX + Cloth.rangeX + Armor.rangeX + Helmet.rangeX + WeaponLeft.rangeX + WeaponRight.rangeX + Horse.rangeX; } }
        public float RangeY { get { return Back.rangeY + Cloth.rangeY + Armor.rangeY + Helmet.rangeY + WeaponLeft.rangeY + WeaponRight.rangeY + Horse.rangeY; } }
        public float AttackSpeed { get { return Back.attackSpeed + Cloth.attackSpeed + Armor.attackSpeed + Helmet.attackSpeed + WeaponLeft.attackSpeed + WeaponRight.attackSpeed + Horse.attackSpeed; } }
        public int Hp { get { return Back.hp + Cloth.hp + Armor.hp + Helmet.hp + WeaponLeft.hp + WeaponRight.hp + Horse.hp; } }
        public int Defence { get { return Back.defence + Cloth.defence + Armor.defence + Helmet.defence + WeaponLeft.defence + WeaponRight.defence + Horse.defence; } }
        public float Speed { get { return Back.speed + Cloth.speed + Armor.speed + Helmet.speed + WeaponLeft.speed + WeaponRight.speed + Horse.speed; } }

        public Equipment()
        {
            Back = DataManager.DefaultItem;
            Cloth = DataManager.DefaultItem;
            Armor = DataManager.DefaultItem;
            Helmet = DataManager.DefaultItem;
            WeaponLeft = DataManager.DefaultItem;
            WeaponRight = DataManager.DefaultItem;
            Horse = DataManager.DefaultItem;
        }

        public void SetEquipment(SPUMData spum)
        {
            Back = DataManager.ItemDict.GetValueOrDefault(spum.back, DataManager.DefaultItem);
            Cloth = DataManager.ItemDict.GetValueOrDefault(spum.cloth, DataManager.DefaultItem);
            Armor = DataManager.ItemDict.GetValueOrDefault(spum.armor, DataManager.DefaultItem);
            Helmet = DataManager.ItemDict.GetValueOrDefault(spum.helmet, DataManager.DefaultItem);
            WeaponLeft = DataManager.ItemDict.GetValueOrDefault(spum.weaponLeft, DataManager.DefaultItem);
            WeaponRight = DataManager.ItemDict.GetValueOrDefault(spum.weaponRight, DataManager.DefaultItem);
            Horse = DataManager.ItemDict.GetValueOrDefault(spum.horse, DataManager.DefaultItem);
        }
    }
}
