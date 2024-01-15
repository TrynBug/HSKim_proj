using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Define
{
    // 이 값들은 유니티 Layer와 동일해야 함
    public enum Layer
    {
        Monster = 8,
        Wall = 9,
        Ground = 10,
        Plants = 11,
        Block = 12,
        Player = 13,
    }

    public enum Scene
    {
        Unknown,
        Login,
        Lobby,
        Game,
    }

    public enum Sound
    {
        Bgm,
        Effect,
        MaxCount,
    }

    public enum UIEvent
    {
        Click,
        Drag,
    }

    public enum MouseEvent
    {
        Press,         // 마우스를 누르고 있음
        PointerDown,   // 마우스가 처음 눌려짐
        PointerUp,     // 마우스가 눌렀다가 뗌
        Click,         // 마우스를 눌렀다가 바로(0.2초 내에) 뗌
    }

    public enum CameraMode
    {
        QuarterView,
    }


    public enum InfoLevel
    {
        Identity,
        Position,
        Stat,
        All,
    }







    /* UI */
    public enum UIDebugInfo
    {
        Text
    }


    /* Number Prefab */
    public enum NumberType
    {
        Damage,
        Heal,
        Exp,
    }




    #region Equipment
    public enum EquipmentType
    {
        Armor,
        Back,
        Cloth,
        Helmet,
        Ride,
        Weapon,
    }

    public enum EquipmentSubType
    {
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
