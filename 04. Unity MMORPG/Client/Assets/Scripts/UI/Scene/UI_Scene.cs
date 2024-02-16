using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// popup이 아닌 UI의 부모 클래스
public class UI_Scene : UI_Base
{
    public override void Init()
    {
        Managers.UI.SetCanvas(gameObject, false);
    }
}
