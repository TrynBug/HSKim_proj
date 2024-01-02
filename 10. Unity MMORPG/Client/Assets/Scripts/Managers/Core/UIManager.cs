using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class UIManager
{
    int _order = 10;  // popup UI 창의 시작 sort order

    Stack<UI_Popup> _popupStack = new Stack<UI_Popup>();  // popup된 UI들을 스택순서로 관리
    UI_Scene _sceneUI = null;

    public GameObject Root  // 유니티 hierarchy 상에서 모든 UI의 부모 오브젝트
    {
        get
        {
            GameObject root = GameObject.Find("@UI_Root");
            if (root == null)
                root = new GameObject { name = "@UI_Root" };
            return root;
        }
    }

    // UIManager에게 자신의 sorting order를 지정해달라고 요청함
    public void SetCanvas(GameObject go, bool sort = true)
    {
        // canvas 가져오기
        Canvas canvas = Util.GetOrAddComponent<Canvas>(go);
        canvas.renderMode = RenderMode.ScreenSpaceOverlay;
        // canvas 내에 canvas가 중첩되어 있을 때 부모 canvas의 sorting order값에 상관없이 내 canvas는 따로 sorting order를 가지도록함
        canvas.overrideSorting = true;   

        // sorting order 설정
        if(sort)
        {
            canvas.sortingOrder = _order;
            _order++;
        }
        else
        {
            canvas.sortingOrder = 0;
        }
    }

    // scene UI 생성
    // T 이름의 스크립트 컴포넌트가 부착된 name 이름의 UI 오브젝트를 생성함
    public T ShowSceneUI<T>(string name = null) where T : UI_Scene
    {
        if (string.IsNullOrEmpty(name))
            name = typeof(T).Name;

        // Scene UI 생성
        GameObject go = Managers.Resource.Instantiate($"UI/Scene/{name}");
        T sceneUI = Util.GetOrAddComponent<T>(go);
        _sceneUI = sceneUI;

        // UI는 유니티 Hierarchy 상에서 UI_Root 아래에 있도록 한다.
        go.transform.SetParent(Root.transform);

        return sceneUI;
    }

    // popup UI 생성
    // T 이름의 스크립트 컴포넌트가 부착된 name 이름의 UI 오브젝트를 생성함
    public T ShowPopupUI<T>(string name = null) where T : UI_Popup
    {
        if (string.IsNullOrEmpty(name))
            name = typeof(T).Name;

        // UI 생성 및 stack에 등록
        GameObject go = Managers.Resource.Instantiate($"UI/Popup/{name}");
        T popup = Util.GetOrAddComponent<T>(go);
        _popupStack.Push(popup);

        // UI는 유니티 Hierarchy 상에서 UI_Root 아래에 있도록 한다.
        go.transform.SetParent(Root.transform);

        return popup;
    }

    // UI 오브젝트 아래에 추가할 UI 오브젝트 생성. 프리팹이 UI/SubItem/ 아래에 존재해야 한다.
    // T 이름의 스크립트 컴포넌트가 부착된 name 이름의 UI 오브젝트를 생성함. parent가 null이 아닐 경우 parent를 부모로 지정함
    public T MakeSubItem<T>(Transform parent = null, string name = null) where T : UI_Base
    {
        if (string.IsNullOrEmpty(name))
            name = typeof(T).Name;

        GameObject go = Managers.Resource.Instantiate($"UI/SubItem/{name}");
        if (parent != null)
            go.transform.SetParent(parent);

        return go.GetOrAddComponent<T>();
    }

    // World Space 상에 존재하는 UI 오브젝트 생성. 프리팹이 UI/WorldSpace/ 아래에 존재해야 한다.
    // T 이름의 스크립트 컴포넌트가 부착된 name 이름의 UI 오브젝트를 생성함. parent가 null이 아닐 경우 parent를 부모로 지정함
    public T MakeWorldSpaceUI<T>(Transform parent = null, string name = null) where T : UI_Base
    {
        if (string.IsNullOrEmpty(name))
            name = typeof(T).Name;

        GameObject go = Managers.Resource.Instantiate($"UI/WorldSpace/{name}");
        if (parent != null)
            go.transform.SetParent(parent);

        Canvas canvas = go.GetOrAddComponent<Canvas>();
        canvas.renderMode = RenderMode.WorldSpace;  // Render Mode를 World Space 로 변경
        canvas.worldCamera = Camera.main;           // Event Camera에 main 카메라 부착

        return go.GetOrAddComponent<T>();
    }

    // popup UI 닫기
    // 가장 최근 띄운 UI를 닫는다. 가장 최근 띄운 UI가 popup 이 아니라면 실패한다.
    public void ClosePopupUI(UI_Popup popup)
    {
        if (_popupStack.Count == 0)
            return;

        if(_popupStack.Peek() != popup)
        {
            Debug.Log("Close Popup Failed!");
            return;
        }

        ClosePopupUI();
    }

    // popup UI 닫기
    // 가장 최근 띄운 UI를 닫는다.
    public void ClosePopupUI()
    {
        if (_popupStack.Count == 0)
            return;

        UI_Popup popup = _popupStack.Pop();
        Managers.Resource.Destroy(popup.gameObject);
        popup = null;

        _order--;
    }

    // 모든 popup UI 닫기
    public void CloseAllPopupUI()
    {
        while (_popupStack.Count > 0)
            ClosePopupUI();
    }

    public void Clear()
    {
        CloseAllPopupUI();
        _sceneUI = null;
    }
}
