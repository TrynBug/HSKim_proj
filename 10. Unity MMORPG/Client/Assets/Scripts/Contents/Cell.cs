using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using UnityEngine;


public class Cell
{
    LinkedList<BaseController> _movingObjects = new LinkedList<BaseController>();

    public bool Collider { get; set; } = false;
    public BaseController Object { get; set; } = null;
    public LinkedList<BaseController> MovingObjects { get { return _movingObjects; } }


    public bool HasObject(BaseController obj)
    {
        return (Object == obj || HasMovingObject(obj));
    }

    public bool RemoveObject(BaseController obj)
    {
        if (Object == obj)
        {
            Object = null;
            return true;
        }
        else
        {
            return RemoveMovingObject(obj);
        }
    }


    public void AddMovingObject(BaseController obj)
    {
        MovingObjects.AddLast(obj);
    }

    public bool RemoveMovingObject(BaseController obj)
    {
        return MovingObjects.Remove(obj);
    }

    public bool HasMovingObject(BaseController obj)
    {
        return MovingObjects.Find(obj) != null;
    }
}

