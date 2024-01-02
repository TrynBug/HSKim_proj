using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

// 키보드 입력을 처리하는 매니저
public class InputManager
{
    public Action KeyAction = null;
    public Action<Define.MouseEvent> MouseAction = null;

    bool _pressed = false;  // 마우스 눌림 여부
    float _pressedTime = 0;  // 마우스가 눌려져있던 시간

    public void Clear()
    {
        KeyAction = null;
        MouseAction = null;
    }

    public void OnUpdate()
    {
        // UI 버튼이 클릭되었다면 종료
        if (EventSystem.current.IsPointerOverGameObject())
           return;

        // 키보드를 눌렀을 때 KeyAction 대리자 호출
        if (Input.anyKey && KeyAction != null)  
            KeyAction.Invoke();

        // 마우스 액션에 따라 MouseAction 대리자 호출
        if(MouseAction != null)
        {
            // 마우스 왼쪽이 눌려져 있는 경우
            if (Input.GetMouseButton(0))  
            {
                // 이전에는 마우스가 눌려져 있지 않았고 지금 눌려짐
                if(!_pressed)
                {
                    MouseAction.Invoke(Define.MouseEvent.PointerDown);  // 마우스가 처음 눌려짐 이벤트
                    _pressedTime = Time.time;
                }

                MouseAction.Invoke(Define.MouseEvent.Press);  // 마우스가 눌려져 있음 이벤트
                _pressed = true;
            }
            // 마우스 왼쪽이 눌려져 있지 않는 경우
            else
            {
                // 이전에 마우스가 눌려져 있었는 경우
                if (_pressed)
                {
                    // 마우스를 누른지 0.2초가 안지났을 경우
                    if (Time.time < _pressedTime + 0.2)
                        MouseAction.Invoke(Define.MouseEvent.Click);  // 마우스를 눌렀다 바로 뗀 이벤트

                    MouseAction.Invoke(Define.MouseEvent.PointerUp);  // 마우스를 눌렀다 뗀 이벤트
                }
                    
                _pressed = false;
                _pressedTime = 0;
            }
        }
    }
}
