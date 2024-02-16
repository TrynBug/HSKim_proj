using System.Collections;
using System.Collections.Generic;
using UnityEngine;

// pooling 대상이라는 것을 표시하는 컴포넌트
// 이 컴포넌트가 부착된 오브젝트는 pooling 대상이다.
public class Poolable : MonoBehaviour
{
    public bool IsUsing = false;   // 현재 pooling이 된 상태인지 여부
}
