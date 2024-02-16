using System;
using System.Collections;
using System.Collections.Generic;
using TMPro;
using UnityEngine;
using UnityEngine.EventSystems;
using UnityEngine.UI;

// 하나의 UI를 쉽게 관리하기 위한 클래스
// 모든 UI는 이 클래스를 상속받는다.
// UI 내의 모든 오브젝트에 쉽게 접근할 수 있게 해준다.
public abstract class UI_Base : MonoBehaviour
{
    // UI 내의 모든 오브젝트를 저장하는 멤버
    // Type은 게임오브젝트 타입이고, Object[]는 UI 내의 해당 타입의 모든 오브젝트를 가지는 배열이다.
    Dictionary<Type, UnityEngine.Object[]> _objects = new Dictionary<Type, UnityEngine.Object[]>();

    public abstract void Init();

    // 입력받은 enum을 순회하면서 enum 값 이름과 동일한 이름의 T 타입 오브젝트를 찾아 _objects에 저장한다.
    // 예를들면 T=Button, Type=enum Buttons { PointButton, ScoreButton } 이라고 한다면,
    // 현재 UI 내에서 타입이 Button 이고 이름이 "PointButton", "ScoreButton" 인 오브젝트를 찾아 _objects에 저장한다.
    protected void Bind<T>(Type type) where T : UnityEngine.Object
    {
        string[] names = Enum.GetNames(type);    // 전달받은 enum 타입의 값들을 string 배열로 가져온다.
        UnityEngine.Object[] objects = new UnityEngine.Object[names.Length];   // object 배열 생성
        _objects.Add(typeof(T), objects);        // _objects 멤버에 <enum, object배열> 추가

        // 전달받은 enum 타입의 값을 순회
        for (int i = 0; i < names.Length; i++)
        {
            // 타입이 T 이고 이름이 names[i] 인 오브젝트를 찾아 objects[i] 에 저장한다.
            if (typeof(T) == typeof(GameObject))
                objects[i] = Util.FindChild(gameObject, names[i], true);
            else
                objects[i] = Util.FindChild<T>(gameObject, names[i], true);

            if (objects[i] == null)
                Debug.Log($"Failed to bind! {names[i]}");
        }
    }

    // UI 내의 T 타입의 오브젝트 중 idx 번째 오브젝트를 리턴한다.
    protected T Get<T>(int idx) where T : UnityEngine.Object
    {
        UnityEngine.Object[] objects = null;
        if (_objects.TryGetValue(typeof(T), out objects) == false)
            return null;

        return objects[idx] as T;
    }
    protected GameObject GetObject(int idx) { return Get<GameObject>(idx); }
    protected TextMeshProUGUI GetText(int idx) { return Get<TextMeshProUGUI>(idx); }
    protected Button GetButton(int idx) { return Get<Button>(idx); }
    protected Image GetImage(int idx) { return Get<Image>(idx); }

    // go 오브젝트에 type 형식의 이벤트가 발생했을 때 action 함수를 호출하도록 등록한다.
    // go 오브젝트에 UI_EventHandler 컴포넌트가 없다면 등록한다.
    public static void BindEvent(GameObject go, Action<PointerEventData> action, Define.UIEvent type = Define.UIEvent.Click)
    {
        UI_EventHandler evt = Util.GetOrAddComponent<UI_EventHandler>(go);

        switch (type)
        {
            case Define.UIEvent.Click:
                evt.OnClickHandler -= action;
                evt.OnClickHandler += action;
                break;
            case Define.UIEvent.Drag:
                evt.OnDragHandler -= action;
                evt.OnDragHandler += action;
                break;
        };
    }
}
