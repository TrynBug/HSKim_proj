using Server.Data;
using Server.Game;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Numerics;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace Server
{
    public static class Util
    {
        public static Vector2Int PosToCell(Vector2 pos)
        {
            return new Vector2Int((int)(pos.x * (float)Config.CellMultiple), (int)(pos.y * (float)Config.CellMultiple));
        }

        public static Vector2 CellToPos(Vector2Int cell)
        {
            return new Vector2((float)cell.x / (float)Config.CellMultiple, (float)cell.y / (float)Config.CellMultiple);
        }

        public static Vector2 CellToCenterPos(Vector2Int cell)
        {
            Vector2 pos = CellToPos(cell);
            return new Vector2(pos.x + Config.CellWidth, pos.y + Config.CellHeight);
        }


    }
}
