using System.Collections;
using System.Collections.Generic;
using UnityEngine;

namespace Data
{
    public static class Config
    {
        public const int CellMultiple = 2;
        public const float CellWidth = 1f / CellMultiple;
        public const float CellHeight = 1f / CellMultiple;

        public const float ObjectDefaultZ = 2.5f;

        public const float DifferenceTolerance = 0.001f;
    }

}