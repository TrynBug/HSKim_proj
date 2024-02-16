using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using static Define;
using UnityEngine.UI;

public class UI_Game : UI_Scene
{
    void Start()
    {
        Bind<Healthbar>(typeof(UIGame));
    }

    void Update()
    {

    }


    public void SetHealthValue(float maxHP, float currentHP)
    {
        Healthbar HpBar = Get<Healthbar>((int)UIGame.Healthbar);
        if (HpBar == null)
            return;
        HpBar.SetHealthValue(maxHP, currentHP);
    }

    public void SetHealth(float health)
    {
        Healthbar HpBar = Get<Healthbar>((int)UIGame.Healthbar);
        if (HpBar == null)
            return;
        HpBar.SetHealth(health);
    }
}
