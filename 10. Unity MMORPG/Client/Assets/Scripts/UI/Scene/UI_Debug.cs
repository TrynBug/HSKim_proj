using Google.Protobuf.Protocol;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using static Define;

public class UI_Debug : UI_Scene
{
    void Start()
    {
        Bind<TextMeshProUGUI>(typeof(UIDebugInfo));

        Managers.Input.KeyAction -= AcceptDebugInput;
        Managers.Input.KeyAction += AcceptDebugInput;
    }

    void Update()
    {
        TextMeshProUGUI text = GetText((int)UIDebugInfo.Text);
        text.text = $"RTT : {Managers.Time.RTTms :f0} ms\n";
        text.text += $"Map : {Managers.Map.MapId}\n";
        
        if(Managers.Object.MyPlayer != null)
        {
            text.text += $"Position : ({Managers.Object.MyPlayer.Pos.x:f2}, {Managers.Object.MyPlayer.Pos.y:f2})\n";
            text.text += $"Cell : ({Managers.Object.MyPlayer.Cell.x}, {Managers.Object.MyPlayer.Cell.y})\n";
        }


        text.text += _debugCommand;

    }



    string _debugCommand = string.Empty;
    void AcceptDebugInput()
    {
        if (Input.GetKeyDown(KeyCode.Slash))
        {
            if (_debugCommand == string.Empty)
            {
                _debugCommand = "/";
            }
            else
            {
                C_DebugCommand debugPacket = new C_DebugCommand();
                debugPacket.Command = string.Copy(_debugCommand);
                Managers.Network.Send(debugPacket);
                _debugCommand = string.Empty;
            }
        }
        else
        {
            if (_debugCommand != string.Empty)
                _debugCommand += Input.inputString;
        }

    }
}
