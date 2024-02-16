using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using static Define;

public class UI_Dead : UI_Popup
{
    float _time = 0;
    float _fadeInTime = 2f;
    void Start()
    {
        Bind<CanvasRenderer>(typeof(UIDead));
    }

    void Update()
    {
        if(_time < _fadeInTime)
        {
            _time += Time.deltaTime;
            CanvasRenderer rendererPanel = Get<CanvasRenderer>((int)UIDead.Panel);
            rendererPanel.SetColor(new Color(1f, 1f, 1f, _time / _fadeInTime));
            CanvasRenderer rendererText = Get<CanvasRenderer>((int)UIDead.Text);
            rendererText.SetColor(new Color(1f, 1f, 1f, _time / _fadeInTime));
        }
    }
}
