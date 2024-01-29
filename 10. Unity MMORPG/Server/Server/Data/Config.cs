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
        public const int FPS = 20;

        /* map */
        public const int CellMultiple = 2;

        /* float */
        public const float DifferenceTolerance = 0.001f;

        /* Auto Move */
        public const int AutoMoveRoamingWaitTime = 2000;

        /* skill */
        public const int SkillTimeCorrection = 50;
    }
}
