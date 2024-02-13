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
        public const double AutoMoveRoamingWaitSecond = 1.5;  // 단위:second
        public const int AutoFindTargetRange = 12;
        public const int AutoRespawnWaitTime = 1000 * 5;   // 단위:ms
        public const double AutoActionWaitSecond = 1.0;    // 단위:second
        public const float AutoRunAwayDistance = 2.5f;
        public const int AutoRunAwayCooltime = 2000;    // 단위:ms

        /* skill */
        public const int SkillTimeCorrection = 50;

        /* DB */
        public const int DBCharacterDataUpdateInterval = 1000 * 180;

        /* login */
        public const int LoginRequestInterval = 1000;
    }
}
