using System;
using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using UnityEngine.EventSystems;

// UI 내의 오브젝트에 이벤트가 발생했을 때 콜백 함수를 호출해주는 컴포넌트
// 이 컴포넌트를 오브젝트에 부착하면 이벤트가 발생했을 때 상속받은 인터페이스에 해당하는 콜백함수를 자동으로 호출해준다.
public class UI_EventHandler : MonoBehaviour, IPointerClickHandler, IDragHandler
{
    public Action<PointerEventData> OnClickHandler = null;
    public Action<PointerEventData> OnDragHandler = null;

    public void OnPointerClick(PointerEventData eventData)
    {
        if (OnClickHandler != null)
            OnClickHandler.Invoke(eventData);
    }

    public void OnDrag(PointerEventData eventData)
    {
        if (OnDragHandler != null)
            OnDragHandler.Invoke(eventData);
    }
}
