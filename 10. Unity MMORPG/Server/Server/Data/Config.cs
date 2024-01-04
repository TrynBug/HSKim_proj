using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Data
{
    public static class Config
    {
        /* room */
        public const int FPS = 4;

        /* map */
        public const int CellMultiple = 2;
        public const float CellWidth = 1f / CellMultiple;
        public const float CellHeight = 1f / CellMultiple;
    }
}
