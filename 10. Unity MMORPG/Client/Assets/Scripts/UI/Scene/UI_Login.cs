using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.UI;
using static Define;

public class UI_Login : UI_Scene
{
    void Start()
    {
        Bind<InputField>(typeof(UILogin));

        Managers.Input.KeyAction -= TryLogin;
        Managers.Input.KeyAction += TryLogin;
    }

    void Update()
    {
        
    }


    void TryLogin()
    {
        if (Input.GetKeyDown(KeyCode.Return))
        {
            InputField inputField = Get<InputField>((int)UILogin.InputField);
            string name = inputField.text;
            if (string.IsNullOrEmpty(name))
                return;

            C_Login login = new C_Login();
            login.Name = name;
            Managers.Network.Send(login);
        }
    }

}
