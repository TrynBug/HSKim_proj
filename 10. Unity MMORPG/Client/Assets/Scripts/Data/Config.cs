using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Data
{
    public static class Config
    {
        // cell
        public const int CellMultiple = 2;
        public const float CellWidth = 1f / CellMultiple;
        public const float CellHeight = 1f / CellMultiple;

        // object
        public const float ObjectDefaultZ = 2.5f;

        // float
        public const float DifferenceTolerance = 0.001f;

        // move
        public const float MyPlayerMinimumMove = 0.1f;
        public const int MovePacketSendInterval = 200;
    }

}