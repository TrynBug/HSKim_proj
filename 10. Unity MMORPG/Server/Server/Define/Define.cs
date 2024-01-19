using Google.Protobuf.WellKnownTypes;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    public enum InfoLevel
    {
        Identity,
        Position,
        Stat,
        All,
    }

    #region Equipment
    public enum EquipmentType
    {
        Empty,
        Armor,
        Back,
        Cloth,
        Helmet,
        Ride,
        Weapon,
    }

    public enum EquipmentSubType
    { 
        Empty,
        Armor_Leather,
        Armor_Steel,
        Back_Cape,
        Cloth_Fabric,
        Helmet_Leather,
        Helmet_Steel,
        Ride_Horse,
        Weapon_Axe,
        Weapon_Bow,
        Weapon_Dagger,
        Weapon_Hammer,
        Weapon_Mace,
        Weapon_Shield,
        Weapon_Spear,
        Weapon_SteelShield,
        Weapon_Sword,
        Weapon_Ward,
    }
    #endregion
}
