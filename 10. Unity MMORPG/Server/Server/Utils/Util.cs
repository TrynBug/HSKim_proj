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
        public static bool Equals(Vector2 a, Vector2 b)
        {
            return (Math.Abs(a.x - b.x) + Math.Abs(a.y - b.y) < Config.DifferenceTolerance);
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

    }


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
