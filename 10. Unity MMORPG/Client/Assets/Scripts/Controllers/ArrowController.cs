using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Google.Protobuf.Protocol;
using static Define;

// 화살 컨트롤러
// BaseController를 상속받는다.
public class ArrowController : BaseController
{
    // 이동 방향
    [SerializeField]
    public override MoveDir Dir
    {
        get { return base.Dir; }
        set
        {
            base.Dir = value;
            switch (Dir)
            {
                case MoveDir.Up:
                    transform.rotation = Quaternion.Euler(0, 0, 0);
                    break;
                case MoveDir.Down:
                    transform.rotation = Quaternion.Euler(0, 0, 180);
                    break;
                case MoveDir.Left:
                    transform.rotation = Quaternion.Euler(0, 0, 90);
                    break;
                case MoveDir.Right:
                    transform.rotation = Quaternion.Euler(0, 0, 270);
                    break;
            }

        }
    }

    protected override void Init()
    {
        base.Init();
    }

    protected override void UpdateController()
    {
        base.UpdateController();
    }

}
