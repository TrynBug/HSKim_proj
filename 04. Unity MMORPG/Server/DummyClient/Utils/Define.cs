using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DummyClient
{
    public class Define
    {
        public enum InfoLevel
        {
            Identity,
            Position,
            Stat,
            All,
        }

        /* key input */
        public enum KeyInput
        {
            Up,
            Down,
            Left,
            Right,
            Attack,
            SkillA,
            SkillB,
            SkillC,
            SkillD,
            SkillE,
            SkillF,
            SkillG,
            SkillH,
            SkillI,
            SkillJ,
            SkillK,
            Auto,
            Enter,
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
            Weapon_Wand,
        }
        #endregion

    }

}
