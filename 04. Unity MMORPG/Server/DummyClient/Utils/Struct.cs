﻿using DummyClient.Game;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace DummyClient
{
    public struct Vector2
    {
        public float x;
        public float y;

        public Vector2(float x, float y) { this.x = x; this.y = y; }

        public static Vector2 operator +(Vector2 a, Vector2 b)
        {
            return new Vector2(a.x + b.x, a.y + b.y);
        }

        public static Vector2 operator -(Vector2 a, Vector2 b)
        {
            return new Vector2(a.x - b.x, a.y - b.y);
        }

        public static Vector2 operator *(Vector2 a, Vector2 b)
        {
            return new Vector2(a.x * b.x, a.y * b.y);
        }

        public static Vector2 operator /(Vector2 a, Vector2 b)
        {
            return new Vector2(a.x / b.x, a.y / b.y);
        }

        public static Vector2 operator +(Vector2 a, float b)
        {
            return new Vector2(a.x + b, a.y + b);
        }

        public static Vector2 operator -(Vector2 a, float b)
        {
            return new Vector2(a.x - b, a.y - b);
        }

        public static Vector2 operator *(Vector2 a, float b)
        {
            return new Vector2(a.x * b, a.y * b);
        }

        public static Vector2 operator /(Vector2 a, float b)
        {
            return new Vector2(a.x / b, a.y / b);
        }

        public float squareMagnitude { get { return (x * x + y * y); } }
        public float magnitude { get { return (float)Math.Sqrt(squareMagnitude); } }
        public float cellDistFromZero { get { return Math.Abs(x) + Math.Abs(y); } }
        public Vector2 normalized { get { return new Vector2(x / magnitude, y / magnitude); } }

        public float Sum { get { return x + y; } }
        public Vector2 Abs { get { return new Vector2(Math.Abs(x), Math.Abs(y)); } }


        public override string? ToString() { return $"({x:f2},{y:f2})"; }
    }




    public struct Vector2Int
    {
        public int x;
        public int y;

        public Vector2Int(int x, int y) { this.x = x; this.y = y; }

        public static Vector2Int up { get { return new Vector2Int(0, 1); } }
        public static Vector2Int down { get { return new Vector2Int(0, -1); } }
        public static Vector2Int left { get { return new Vector2Int(-1, 0); } }
        public static Vector2Int right { get { return new Vector2Int(1, 0); } }

        public static Vector2Int one { get { return new Vector2Int(1, 1); } }
        public static Vector2Int zero { get { return new Vector2Int(0, 0); } }

        public static Vector2Int operator +(Vector2Int a, Vector2Int b)
        {
            return new Vector2Int(a.x + b.x, a.y + b.y);
        }

        public static Vector2Int operator -(Vector2Int a, Vector2Int b)
        {
            return new Vector2Int(a.x - b.x, a.y - b.y);
        }

        public static Vector2Int operator *(Vector2Int a, Vector2Int b)
        {
            return new Vector2Int(a.x * b.x, a.y * b.y);
        }

        public static Vector2Int operator /(Vector2Int a, Vector2Int b)
        {
            return new Vector2Int(a.x / b.x, a.y / b.y);
        }

        public static Vector2Int operator +(Vector2Int a, int b)
        {
            return new Vector2Int(a.x + b, a.y + b);
        }

        public static Vector2Int operator -(Vector2Int a, int b)
        {
            return new Vector2Int(a.x - b, a.y - b);
        }

        public static Vector2Int operator *(Vector2Int a, int b)
        {
            return new Vector2Int(a.x * b, a.y * b);
        }

        public static Vector2Int operator /(Vector2Int a, int b)
        {
            return new Vector2Int(a.x / b, a.y / b);
        }

        public static bool operator == (Vector2Int a, Vector2Int b)
        {
            return a.x == b.x && a.y == b.y;
        }

        public static bool operator !=(Vector2Int a, Vector2Int b)
        {
            return !(a == b);
        }

        public int sqrtMagnitude { get { return (x * x + y * y); } }
        public float magnitude { get { return (float)Math.Sqrt(sqrtMagnitude); } }
        public int cellDistFromZero { get { return Math.Abs(x) + Math.Abs(y); } }

        public override string? ToString() { return $"({x},{y})"; }
    }







    public class SkillUseInfo
    {
        public Data.SkillData skill = null;
        public int lastUseTime = 0;
        public bool casted = false;
    }

    public class RoomTransferInfo
    {
        public PlayerController player;
        public int prevRoomId;
        public int prevTeleportId;
        public int nextRoomId;
    }
}