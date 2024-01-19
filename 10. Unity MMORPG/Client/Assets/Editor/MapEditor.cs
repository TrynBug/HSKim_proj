using System.Collections;
using System.Collections.Generic;
using UnityEngine;
using Unity.VisualScripting;
using UnityEngine.Tilemaps;
using System.IO;
using System.Text.RegularExpressions;
using Data;
using static UnityEngine.UI.Image;




#if UNITY_EDITOR
using UnityEditor;
#endif

public class MapEditor
{
#if UNITY_EDITOR

    // Prefabs/Map 아래의 각각의 grid 내의 Tilemap_Collision 타일맵에서 갈수없는 cell 정보를 추출하여 파일로 생성하는 함수
    // Tools 메뉴 아래에 GenerateMap 메뉴를 만들고, 해당 메뉴 선택시 아래 함수를 호출한다.
    // 단축키는 Ctrl + Shift + m 으로 한다. attribute에서의 단축키 예약어는 %(Ctrl), #(Shift), &(Alt) 로 지정되어 있다.
    [MenuItem("Tools/Generate Map Data %#m")]
    private static void GenerateMap()
    {
        GenerateByPath("Assets/Resources/Data");
        GenerateByPath("../Server/Common/Data");
    }


    private static void GenerateByPath(string path)
    {
        // Prefabs/Map 아래의 모든 grid 조사
        GameObject[] gameObjects = Resources.LoadAll<GameObject>("Prefabs/Map");


        List<Data.MapData> listMap = new List<Data.MapData>();
        Regex regexPrefabNumber = new Regex("Map_([0-9]{1,})$");
        foreach (GameObject go in gameObjects)
        {
            // prefab 이름에서 끝에 숫자를 추출함
            string prefabNumber = regexPrefabNumber.Replace(go.name, "$1");
            int id;
            if (int.TryParse(prefabNumber, out id) == false)
                continue;
            Data.MapData mapData = new MapData();
            mapData.id = id;

            // grid 가져오기
            Grid grid = go.GetComponent<Grid>();
            Tilemap tmGround = Util.FindChild<Tilemap>(go, "Ground", true);   // Ground 타일맵 찾기
            Tilemap tmCollision = Util.FindChild<Tilemap>(go, "Collision", true);  // Collision 타일맵 찾기
            GameObject goTeleport = Util.FindChild(go, "Teleport", true);  // Teleport 오브젝트 찾기
            GameObject goEnterZone = Util.FindChild(go, "EnterZone", true);  // EnterZone 오브젝트 찾기

            // 1개 cell의 크기
            mapData.cellWidth = grid.cellSize.x;
            mapData.cellHeight = grid.cellSize.y;

            // cell size
            mapData.cellBoundMinX = tmGround.cellBounds.xMin;
            mapData.cellBoundMaxX = tmGround.cellBounds.xMax;
            mapData.cellBoundMinY = tmGround.cellBounds.yMin;
            mapData.cellBoundMaxY = tmGround.cellBounds.yMax;

            // Ground 타일맵의 왼쪽위부터 오른쪽아래로 이동하며 해당 위치에서 collision 타일맵에 타일이 존재하는지를 체크한다.
            // 타일이 존재하면 "1", 없으면 "0"을 write 한다.
            for (int y = tmGround.cellBounds.yMax; y >= tmGround.cellBounds.yMin; y--)
            {
                // collision 타일이 존재하면 char 배열에 "1", 없으면 "0" 입력
                char[] line = new char[tmGround.cellBounds.xMax - tmGround.cellBounds.xMin + 1];
                int lineIndex = 0;
                for (int x = tmGround.cellBounds.xMin; x <= tmGround.cellBounds.xMax; x++)
                {
                    TileBase tile = tmCollision.GetTile(new Vector3Int(x, y, 0));  // collision 타일맵에 타일이 존재하는지를 체크
                    if (tile != null)
                        line[lineIndex] = '1';
                    else
                        line[lineIndex] = '0';
                    lineIndex++;
                }

                // 현재 line에서 collision이 없는 좌측 부분을 모두 1로 변경한다.
                for (int x = 0; x < line.Length; x++)
                {
                    if (line[x] == '1')
                        break;
                    line[x] = '1';
                }

                // 현재 line에서 collision이 없는 우측 부분을 모두 1로 변경한다.
                for (int x = line.Length - 1; x >= 0; x--)
                {
                    if (line[x] == '1')
                        break;
                    line[x] = '1';
                }

                // collision 정보 기록
                mapData.collisions.Add(new string(line));
            }


            // teleport 위치 write
            GameObject goInstant = Object.Instantiate(go);   // tilemap의 LocalToCellInterpolated 함수를 사용하기 위해 Instantiate 함. 안그러면 함수 리턴값이 무조건 0임
            Tilemap tmGroundInstant = Util.FindChild<Tilemap>(goInstant, "Ground", true);
            foreach (Transform teleport in goTeleport.transform)
            {

                TeleportData data = new TeleportData();
                int number;
                if (int.TryParse(teleport.name, out number) == false)
                    continue;
                data.number = number;

                // client pos를 server pos로 변경
                Vector2 clientPos = teleport.position;
                Vector2 serverPos = clientPos += new Vector2(mapData.cellWidth / 2f, -mapData.cellHeight / 2f);
                serverPos = tmGroundInstant.LocalToCellInterpolated(serverPos);
                serverPos = new Vector2(serverPos.x - mapData.cellBoundMinX, mapData.cellBoundMaxY - serverPos.y);

                // write
                data.posX = serverPos.x;
                data.posY = serverPos.y;
                data.width = 1.8f;
                data.height = 0.8f;

                mapData.teleports.Add(data);
            }

            // enter zone 위치 write
            foreach (Transform enter in goEnterZone.transform)
            {
                EnterZoneData data = new EnterZoneData();
                int number;
                if (int.TryParse(enter.name, out number) == false)
                    continue;
                data.number = number;

                // client pos를 server pos로 변경
                Vector2 clientPos = enter.position;
                Vector2 serverPos = clientPos += new Vector2(mapData.cellWidth / 2f, -mapData.cellHeight / 2f);
                serverPos = tmGroundInstant.LocalToCellInterpolated(serverPos);
                serverPos = new Vector2(serverPos.x - mapData.cellBoundMinX, mapData.cellBoundMaxY - serverPos.y);

                // write
                data.posX = serverPos.x;
                data.posY = serverPos.y;

                mapData.enterZones.Add(data);
            }


            GameObject.DestroyImmediate(goInstant);

            // 전체 list에 추가
            listMap.Add(mapData);
        }

        // json 으로 변환후 write
        string jsonText = Newtonsoft.Json.JsonConvert.SerializeObject(listMap, Newtonsoft.Json.Formatting.Indented);
        StreamWriter writer = File.CreateText($"{path}/MapData.json");
        writer.Write("{\n\"maps\": ");
        writer.Write(jsonText);
        writer.Write("\n}");
        writer.Close(); 
    }


    private static void GenerateByPath_oldVersion(string path)
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
                    // collision 타일이 존재하면 "1", 없으면 "0" 추가
                    string line = "";
                    for (int x = tmGround.cellBounds.xMin; x <= tmGround.cellBounds.xMax; x++)
                    {
                        TileBase tile = tmCollision.GetTile(new Vector3Int(x, y, 0));  // collision 타일맵에 타일이 존재하는지를 체크
                        if (tile != null)
                            line += '1';
                        else
                            line += '0';
                    }

                    // 현재 line에서 collision이 없는 좌측 부분을 모두 1로 변경한다.
                    char[] lineChar = line.ToCharArray();
                    for(int x=0; x< lineChar.Length; x++)
                    {
                        if (lineChar[x] == '1')
                            break;
                        lineChar[x] = '1';
                    }

                    // 현재 line에서 collision이 없는 우측 부분을 모두 1로 변경한다.
                    for (int x = lineChar.Length - 1; x >= 0; x--)
                    {
                        if (lineChar[x] == '1')
                            break;
                        lineChar[x] = '1';
                    }

                    // write line
                    string finalLine = new string(lineChar);
                    writer.WriteLine(finalLine);
                }
            }
        }
    }

#endif
}
