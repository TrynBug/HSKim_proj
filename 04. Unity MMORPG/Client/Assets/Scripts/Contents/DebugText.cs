using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class DebugText : MonoBehaviour
{
    TextMeshPro _text;

    public void Start()
    {
        _text = gameObject.GetComponentInChildren<TextMeshPro>();
    }

    public void SetText(string text)
    {
        if (_text != null)
            _text.text = text;
    }

}
