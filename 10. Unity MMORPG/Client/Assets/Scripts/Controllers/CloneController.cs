using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using static Define;
using Data;
using ServerCore;
using System;
using UnityEngine.Rendering;



// CreatureController 의 클론
// 디버그 모드에서 Creature의 Dest 위치를 표시하는데 사용된다.
public class CloneController : SPUMController
{
    public bool IsDestroyed { get; private set; } = false;

    SPUMController _target;

    public virtual void Init(SPUMController target)
    {
        if (target == null)
        {
            IsDestroyed = true;
            return;
        }
        _target = target;

        base.Init(target.Info);
        SetHpBarActive(false);


        // sorting order 조정
        SortingGroup sort = UnitRoot.GetOrAddComponent<SortingGroup>();
        sort.sortingOrder = 5;

        // 투명도 조절
        SetTransparentMaterial();

    }

    protected override void UpdateController()
    {
        if (_target == null)
        {
            IsDestroyed = true;
            return;
        }



        if (transform.position == _target.transform.position)
        {
            UnitRoot.SetActive(false);
            return;
        }
        else
        {
            UnitRoot.SetActive(true);
        }


        Pos = _target.Dest;
        Dir = _target.Dir;
        LookDir = _target.LookDir;
        State = _target.State;
    }

    // 모든 sprite에 투명 material 적용
    void SetTransparentMaterial()
    {
        SPUM_Prefabs spumRoot = GetComponent<SPUM_Prefabs>();
        if (spumRoot == null)
            return;
        if (spumRoot._spriteOBj == null)
            return;
        Material material = Resources.Load<Material>("Texture/TransparentMaterial");
        if (material == null)
            return;
        List<Material> materials = new List<Material> { material };
        _setTransparentMaterial(spumRoot._spriteOBj._itemList, materials);
        _setTransparentMaterial(spumRoot._spriteOBj._eyeList, materials);
        _setTransparentMaterial(spumRoot._spriteOBj._hairList, materials);
        _setTransparentMaterial(spumRoot._spriteOBj._bodyList, materials);
        _setTransparentMaterial(spumRoot._spriteOBj._clothList, materials);
        _setTransparentMaterial(spumRoot._spriteOBj._armorList, materials);
        _setTransparentMaterial(spumRoot._spriteOBj._pantList, materials);
        _setTransparentMaterial(spumRoot._spriteOBj._weaponList, materials);
        _setTransparentMaterial(spumRoot._spriteOBj._backList, materials);
        if (spumRoot._spriteOBj._spHorseSPList == null)
            return;
        _setTransparentMaterial(spumRoot._spriteOBj._spHorseSPList._spList, materials);
        
    }

    void _setTransparentMaterial(List<SpriteRenderer> sprites, List<Material> materials)
    {
        if (sprites == null || materials == null)
            return;
        foreach (SpriteRenderer sprite in sprites)
            sprite.SetSharedMaterials(materials);
    }
}
