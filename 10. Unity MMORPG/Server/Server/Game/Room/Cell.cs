using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace Server.Game
{
    public class Cell
    {
        LinkedList<GameObject> _movingObjects = new LinkedList<GameObject>();

        public bool Collider { get; set; } = false;
        public GameObject Object { get; set; } = null;
        public LinkedList<GameObject> MovingObjects { get { return _movingObjects; } }

        public bool HasObject(GameObject obj)
        {
            return (Object == obj || HasMovingObject(obj));
        }

        public bool RemoveObject(GameObject obj)
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


        public void AddMovingObject(GameObject obj)
        {
            MovingObjects.AddLast(obj);
        }

        public bool RemoveMovingObject(GameObject obj)
        {
            return MovingObjects.Remove(obj);
        }

        public bool HasMovingObject(GameObject obj)
        {
            return MovingObjects.Find(obj) != null;
        }
    }
}
