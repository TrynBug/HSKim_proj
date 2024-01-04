using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Unity.VisualScripting;
using UnityEngine.Tilemaps;
using System.IO;

#if UNITY_EDITOR
using UnityEditor;
#endif

public class MapEditor
{
#if UNITY_EDITOR

    // Prefabs/Map 아래의 각각의 grid 내의 Tilemap_Collision 타일맵에서 갈수없는 cell 정보를 추출하여 파일로 생성하는 함수
    // Tools 메뉴 아래에 GenerateMap 메뉴를 만들고, 해당 메뉴 선택시 아래 함수를 호출한다.
    // 단축키는 Ctrl + Shift + m 으로 한다. attribute에서의 단축키 예약어는 %(Ctrl), #(Shift), &(Alt) 로 지정되어 있다.
    [MenuItem("Tools/GenerateMap %#m")]
    private static void GenerateMap()
    {
        GenerateByPath("Assets/Resources/Map");
        GenerateByPath("../Server/Common/Map");
    }

    private static void GenerateByPath(string path)
    {
        // Prefabs/Map 아래의 모든 grid 조사
        GameObject[] gameObjects = Resources.LoadAll<GameObject>("Prefabs/Map");

        foreach (GameObject go in gameObjects)
        {
            Grid grid = go.GetComponent<Grid>();
            Tilemap tmGround = Util.FindChild<Tilemap>(go, "Ground", true);   // Ground 타일맵 찾기
            Tilemap tmCollision = Util.FindChild<Tilemap>(go, "Collision", true);  // Collision 타일맵 찾기

            // 갈수없는 cell 정보를 추출하여 파일로 생성
            using (var writer = File.CreateText($"{path}/{go.name}.txt"))
            {
                // 1개 cell 크기 write
                writer.WriteLine(grid.cellSize.x);
                writer.WriteLine(grid.cellSize.y);
                
                // Ground 타일맵의 크기를 write한다.
                writer.WriteLine(tmGround.cellBounds.xMin);
                writer.WriteLine(tmGround.cellBounds.xMax);
                writer.WriteLine(tmGround.cellBounds.yMin);
                writer.WriteLine(tmGround.cellBounds.yMax);

                // Ground 타일맵의 왼쪽위부터 오른쪽아래로 이동하며 해당 위치에서 collision 타일맵에 타일이 존재하는지를 체크한다.
                // 타일이 존재하면 "1", 없으면 "0"을 write 한다.
                for (int y = tmGround.cellBounds.yMax; y >= tmGround.cellBounds.yMin; y--)
                {
                    for (int x = tmGround.cellBounds.xMin; x <= tmGround.cellBounds.xMax; x++)
                    {
                        TileBase tile = tmCollision.GetTile(new Vector3Int(x, y, 0));  // collision 타일맵에 타일이 존재하는지를 체크
                        if (tile != null)
                            writer.Write("1");
                        else
                            writer.Write("0");
                    }
                    writer.WriteLine();
                }
            }
        }
    }

#endif
}
