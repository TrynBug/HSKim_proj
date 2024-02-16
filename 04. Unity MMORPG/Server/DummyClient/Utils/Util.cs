using Google.Protobuf.Protocol;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DummyClient
{
    internal static class Util
    {
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

    }
}
