using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class HpBar : MonoBehaviour
{
    [SerializeField]
    Transform _hpBar = null;
    
    public void SetHpBar(float ratio)
    {
        // "Bar" 게임오브젝트의 transform의 x축 scale을 조정함으로써 HP Bar를 조정한다.
        ratio = Mathf.Clamp(ratio, 0, 1);
        _hpBar.localScale = new Vector3(ratio, 1, 1);
    }
}
