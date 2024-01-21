using System;
using System.Collections.Generic;
using System.Drawing;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Unity.VisualScripting;
using UnityEngine;


/*
Bresenham 직선 알고리즘
*/

public class BresenhamPath
{
    enum Case
    {
        RightDown,
		DownRight,
		RightUp,
		UpRight,
		LeftDown,
		DownLeft,
		LeftUp,
		UpLeft,
		END
    };


    public int x { get { return _x; } }
    public int y { get { return _y; } }


    Vector2Int _start;
    Vector2Int _end;

    int _x;
    int _y;
    int _diffX;
    int _diffY;
    int _error;

    Case _case;


    public void Init(Vector2Int start, Vector2Int end)
    {
        _start = start;
        _end = end;

        Reset();
    }

    void Reset()
    {
        _x = _start.x;
        _y = _start.y;
        _diffX = _end.x - _start.x;
        _diffY = _end.y - _start.y;

        if (_diffX > 0)
        {
            if (_diffY > 0)
            {
                if (_diffX > _diffY)
                {
                    // ---------→
                    //          ↓
                    _case = Case.RightDown;
                    _error = Math.Max(0, _diffX / 2 / Math.Max(1, _diffY) - 1) * _diffY;
                }
                else
                {
                    // ---→
                    //    |
                    //    ↓
                    _case = Case.DownRight;
                    _error = Math.Max(0, _diffY / 2 / Math.Max(1, _diffX) - 1) * _diffX;
                }
            }
            else
            {
                if (_diffX > -_diffY)
                {
                    //          ↑
                    // ---------→
                    _case = Case.RightUp;
                    _error = Math.Max(0, _diffX / 2 / Math.Max(1, -_diffY) - 1) * -_diffY;
                }
                else
                {
                    //    ↑
                    //    |
                    // ---→
                    _case = Case.UpRight;
                    _error = Math.Max(0, -_diffY / 2 / Math.Max(1, _diffX) - 1) * _diffX;
                }
            }
        }
        else
        {
            if (_diffY > 0)
            {
                if (-_diffX > _diffY)
                {
                    // ←---------
                    // ↓
                    _case = Case.LeftDown;
                    _error = Math.Max(0, -_diffX / 2 / Math.Max(1, _diffY) - 1) * _diffY;
                }
                else
                {
                    // ←---
                    // |
                    // ↓
                    _case = Case.DownLeft;
                    _error = Math.Max(0, _diffY / 2 / Math.Max(1, -_diffX) - 1) * -_diffX;
                }
            }
            else
            {
                if (_diffX < _diffY)
                {
                    // ↑
                    // ←---------
                    _case = Case.LeftUp;
                    _error = Math.Max(0, -_diffX / 2 / Math.Max(1, -_diffY) - 1) * -_diffY;
                }
                else
                {
                    // ↑
                    // |
                    // ←---
                    _case = Case.UpLeft;
                    _error = Math.Max(0, -_diffY / 2 / Math.Max(1, -_diffX) - 1) * -_diffX;
                }
            }
        }

    }





    public bool Next()
    {
        if (_x == _end.x && _y == _end.y)
            return false;

        switch (_case)
        {
            case Case.RightDown:
                // ---------→
                //          ↓
                _x++;
                _error += _diffY;
                if (_error >= _diffX)
                {
                    _y++;
                    _error -= _diffX;
                }
                break;

            case Case.DownRight:
                // ---→
                //    |
                //    ↓
                _y++;
                _error += _diffX;
                if (_error >= _diffY)
                {
                    _x++;
                    _error -= _diffY;
                }
                break;

            case Case.RightUp:
                //          ↑
                // ---------→
                _x++;
                _error -= _diffY;
                if (_error >= _diffX)
                {
                    _y--;
                    _error -= _diffX;
                }
                break;
            case Case.UpRight:
                //    ↑
                //    |
                // ---→
                _y--;
                _error += _diffX;
                if (_error >= -_diffY)
                {
                    _x++;
                    _error += _diffY;
                }
                break;

            case Case.LeftDown:
                // ←---------
                // ↓
                _x--;
                _error += _diffY;
                if (_error >= -_diffX)
                {
                    _y++;
                    _error += _diffX;
                }
                break;
            case Case.DownLeft:
                // ←---
                // |
                // ↓
                _y++;
                _error -= _diffX;
                if (_error >= _diffY)
                {
                    _x--;
                    _error -= _diffY;
                }
                break;

            case Case.LeftUp:
                // ↑
                // ←---------
                _x--;
                _error -= _diffY;
                if (_error >= -_diffX)
                {
                    _y--;
                    _error += _diffX;
                }
                break;

            case Case.UpLeft:
                // ↑
                // |
                // ←---
                _y--;
                _error -= _diffX;
                if (_error >= -_diffY)
                {
                    _x--;
                    _error += _diffY;
                }
                break;


        }

        return true;
    }
}



