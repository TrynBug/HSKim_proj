using Google.Protobuf.Protocol;
using Server.Data;
using Server.Game;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Security;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    public static class Util
    {
        // Vector2 비교
        public static bool Equals(Vector2 a, Vector2 b)
        {
            return (Math.Abs(a.x - b.x) + Math.Abs(a.y - b.y) < Config.DifferenceTolerance);
        }


        // 현재 방향에 해당하는 벡터 얻기
        public static Vector2 GetDirectionVector(MoveDir dir)
        {
            Vector2 direction;
            switch (dir)
            {
                case MoveDir.Up:
                    direction = new Vector2(1, -1).normalized;
                    break;
                case MoveDir.Down:
                    direction = new Vector2(-1, 1).normalized;
                    break;
                case MoveDir.Left:
                    direction = new Vector2(-1, -1).normalized;
                    break;
                case MoveDir.Right:
                    direction = new Vector2(1, 1).normalized;
                    break;
                case MoveDir.LeftUp:
                    direction = new Vector2(0, -1);
                    break;
                case MoveDir.LeftDown:
                    direction = new Vector2(-1, 0);
                    break;
                case MoveDir.RightUp:
                    direction = new Vector2(1, 0);
                    break;
                case MoveDir.RightDown:
                    direction = new Vector2(0, 1);
                    break;
                default:
                    direction = new Vector2(0, 0);
                    break;
            }

            return direction;
        }


        // 현재 방향에 해당하는 벡터 얻기
        public static Vector2 GetDirectionVector(LookDir dir)
        {
            Vector2 direction;
            switch (dir)
            {
                case LookDir.LookLeft:
                    direction = new Vector2(-1, -1).normalized;
                    break;
                case LookDir.LookRight:
                    direction = new Vector2(1, 1).normalized;
                    break;
                default:
                    direction = new Vector2(0, 0);
                    break;
            }

            return direction;
        }




        // 반대방향 얻기
        public static MoveDir GetOppositeDirection(MoveDir dir)
        {
            MoveDir opposite;
            switch (dir)
            {
                case MoveDir.Up:
                    opposite = MoveDir.Down;
                    break;
                case MoveDir.Down:
                    opposite = MoveDir.Up;
                    break;
                case MoveDir.Left:
                    opposite = MoveDir.Right;
                    break;
                case MoveDir.Right:
                    opposite = MoveDir.Left;
                    break;
                case MoveDir.LeftUp:
                    opposite = MoveDir.RightDown;
                    break;
                case MoveDir.LeftDown:
                    opposite = MoveDir.RightUp;
                    break;
                case MoveDir.RightUp:
                    opposite = MoveDir.LeftDown;
                    break;
                case MoveDir.RightDown:
                    opposite = MoveDir.LeftUp;
                    break;
                default:
                    opposite = dir;
                    break;
            }

            return opposite;
        }


        // origin을 기준으로 dir 방향의 cell을 찾는다.
        public static Vector2Int FindCellOfDirection(MoveDir dir, Vector2Int origin)
        {
            Vector2Int cell;
            switch (dir)
            {
                case MoveDir.Up:
                    cell = new Vector2Int(origin.x + 1, origin.y - 1);
                    break;
                case MoveDir.Down:
                    cell = new Vector2Int(origin.x - 1, origin.y + 1);
                    break;
                case MoveDir.Left:
                    cell = new Vector2Int(origin.x - 1, origin.y - 1);
                    break;
                case MoveDir.Right:
                    cell = new Vector2Int(origin.x + 1, origin.y + 1);
                    break;
                case MoveDir.LeftUp:
                    cell = new Vector2Int(origin.x, origin.y - 1);
                    break;
                case MoveDir.LeftDown:
                    cell = new Vector2Int(origin.x - 1, origin.y);
                    break;
                case MoveDir.RightUp:
                    cell = new Vector2Int(origin.x + 1, origin.y);
                    break;
                case MoveDir.RightDown:
                    cell = new Vector2Int(origin.x, origin.y + 1);
                    break;
                default:
                    cell = origin;
                    break;
            }
            return cell;
        }

        // pos에서 dest로의 방향 얻기
        public static MoveDir GetDirectionToDest(Vector2 pos, Vector2 dest)
        {
            const float r1 = (float)Math.PI / 180f * (22.5f + 45f * 0);
            const float r2 = (float)Math.PI / 180f * (22.5f + 45f * 1);
            const float r3 = (float)Math.PI / 180f * (22.5f + 45f * 2);
            const float r4 = (float)Math.PI / 180f * (22.5f + 45f * 3);
            const float r5 = (float)Math.PI / 180f * -(22.5f + 45f * 3);
            const float r6 = (float)Math.PI / 180f * -(22.5f + 45f * 2);
            const float r7 = (float)Math.PI / 180f * -(22.5f + 45f * 1);
            const float r8 = (float)Math.PI / 180f * -(22.5f + 45f * 0);

            Vector2 diff = (dest - pos).normalized;
            float radian = (float)Math.Atan2(-diff.y, diff.x);

            if (radian > 0)
            {
                if (radian < r1)
                    return MoveDir.RightUp;
                else if (radian < r2)
                    return MoveDir.Up;
                else if (radian < r3)
                    return MoveDir.LeftUp;
                else if (radian < r4)
                    return MoveDir.Left;
                else
                    return MoveDir.LeftDown;
            }
            else
            {
                if (radian < r5)
                    return MoveDir.LeftDown;
                else if (radian < r6)
                    return MoveDir.Down;
                else if (radian < r7)
                    return MoveDir.RightDown;
                else if (radian < r8)
                    return MoveDir.Right;
                else
                    return MoveDir.RightUp;
            }
        }

        // posOrigin을 기준으로, look 방향으로 사각형의 range 범위 내에 target이 위치하는지를 확인함
        public static bool IsTargetInRectRange(Vector2 posOrigin, LookDir look, Vector2 range, Vector2 posTarget)
        {
            // 점을 0도 방향으로 회전시키기 위한 cos, sin 값
            Vector2 R;
            if (look == LookDir.LookLeft)
                R = new Vector2((float)Math.Cos(Math.PI / 180 * 135), (float)Math.Sin(Math.PI / 180 * 135));
            else
                R = new Vector2((float)Math.Cos(Math.PI / 180 * 315), (float)Math.Sin(Math.PI / 180 * 315));

            // target이 range 범위의 사각형 안에 존재하는지 확인
            Vector2 objectPos;
            objectPos.x = (posTarget.x - posOrigin.x) * R.x - (posTarget.y - posOrigin.y) * R.y;
            objectPos.y = (posTarget.x - posOrigin.x) * R.y + (posTarget.y - posOrigin.y) * R.x;
            if (objectPos.x > 0 && objectPos.x < range.x && objectPos.y > -range.y / 2 && objectPos.y < range.y / 2)
            {
                return true;
            }

            return false;

        }
    }







    // set time resolution
    public static class WinApi
    {
        /// <summary>TimeBeginPeriod(). See the Windows API documentation for details.</summary>

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Interoperability", "CA1401:PInvokesShouldNotBeVisible"), System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage"), SuppressUnmanagedCodeSecurity]
        [DllImport("winmm.dll", EntryPoint = "timeBeginPeriod", SetLastError = true)]

        public static extern uint TimeBeginPeriod(uint uMilliseconds);

        /// <summary>TimeEndPeriod(). See the Windows API documentation for details.</summary>

        [System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Interoperability", "CA1401:PInvokesShouldNotBeVisible"), System.Diagnostics.CodeAnalysis.SuppressMessage("Microsoft.Security", "CA2118:ReviewSuppressUnmanagedCodeSecurityUsage"), SuppressUnmanagedCodeSecurity]
        [DllImport("winmm.dll", EntryPoint = "timeEndPeriod", SetLastError = true)]

        public static extern uint TimeEndPeriod(uint uMilliseconds);
    }


}
