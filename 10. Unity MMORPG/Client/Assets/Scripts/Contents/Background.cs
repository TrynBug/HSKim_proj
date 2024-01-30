using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class Background : MonoBehaviour
{
    [SerializeField]
    public float OffsetStep = 0.1f;

    void LateUpdate()
    {
        gameObject.transform.position = new Vector3(Camera.main.transform.position.x, Camera.main.transform.position.y, 0);

        

        int layerNumber = 0;
        float offset = -OffsetStep;
        foreach(Transform layer in gameObject.transform)
        {
            layerNumber++;
            offset += OffsetStep;
            foreach (Transform obj in layer.transform)
            {
                obj.localPosition = gameObject.transform.position * offset;
            }
        }
    }
}
