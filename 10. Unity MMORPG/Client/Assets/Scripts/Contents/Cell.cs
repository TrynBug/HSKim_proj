using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;


public class Cell
{
    bool _collider;
    BaseController _object;
    List<BaseController> _movingObjects = new List<BaseController>();

    public bool Collider { get { return _collider; } }
    public BaseController Object { 
        get { return _object; }
        set { _object = value; }
    }
    public List<BaseController> MovingObjects { get { return _movingObjects; } }



}

