using System.Collections;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;

// popup UI의 부모 클래스
public class UI_Popup : UI_Base
{
    public override void Init()
    {
        Managers.UI.SetCanvas(gameObject, true);
    }

    public virtual void ClosePopupUI()
    {
        Managers.UI.ClosePopupUI(this);
    }
}
