using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Data
{
    public static class Config
    {
        public static readonly int CellMultiple = 2;
        public static readonly float CellWidth = 1f / CellMultiple;
        public static readonly float CellHeight = 1f / CellMultiple;
    }
}
