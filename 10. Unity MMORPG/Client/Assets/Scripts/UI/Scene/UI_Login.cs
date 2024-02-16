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
            InputField inputName = Get<InputField>((int)UILogin.InputName);
            string name = inputName.text;
            if (string.IsNullOrEmpty(name))
                return;

            InputField inputPassword = Get<InputField>((int)UILogin.InputPassword);
            string password = inputPassword.text;
            if (string.IsNullOrEmpty(password))
                return;

            C_Login login = new C_Login();
            login.Name = name;
            login.Password = password;
            
            Managers.Network.Send(login);
        }
    }

}
