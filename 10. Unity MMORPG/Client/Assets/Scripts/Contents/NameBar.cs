using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;

public class NameBar : MonoBehaviour
{
    TextMeshPro _text;

    public void Init(string text)
    {
        _text = gameObject.GetComponentInChildren<TextMeshPro>();
        if (_text != null)
            _text.text = text;
    }
}
