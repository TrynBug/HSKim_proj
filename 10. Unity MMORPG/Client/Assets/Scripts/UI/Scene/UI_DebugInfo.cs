using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using static Define;

public class UI_DebugInfo : UI_Scene
{
    // Start is called before the first frame update
    void Start()
    {
        Bind<TextMeshProUGUI>(typeof(UIDebugInfo));
    }

    // Update is called once per frame
    void Update()
    {
        TextMeshProUGUI text = GetText((int)UIDebugInfo.Text);
        text.text = $"RTT : {Managers.Time.RTTms :f0} ms";
    }
}
