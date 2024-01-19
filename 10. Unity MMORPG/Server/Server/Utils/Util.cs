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
