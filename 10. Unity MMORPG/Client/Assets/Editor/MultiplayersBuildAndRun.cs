using System.Collections;
using System.Collections.Generic;
using UnityEditor;
using UnityEngine;
using UnityEngine.UIElements;

// Tools/Run Multiplayer/N Players 메뉴에 해당하는 윈도우
class Window_RunMultiplayer_NPlayers : EditorWindow
{
    [MenuItem("Tools/Run Multiplayer/N Players")]
    public static void ShowWindow()
    {
        EditorWindow wnd = EditorWindow.GetWindow(typeof(Window_RunMultiplayer_NPlayers));
        wnd.titleContent.text = "Run N Players";
    }

    public void CreateGUI()
    {
        // Each editor window contains a root VisualElement object
        VisualElement root = rootVisualElement;

        // 시작 레이블
        Label label = new Label();
        label.name = "lblSettings";
        label.text = "Settings";
        root.Add(label);

        UnsignedIntegerField fieldNumPlayers = new UnsignedIntegerField("Num Players (Max:10)", 2);
        fieldNumPlayers.name = "fieldNumPlayers";
        root.Add(fieldNumPlayers);

        // Run 버튼
        Button button = new Button();
        button.name = "btnRun";
        button.text = "Run";
        button.clicked += () => {
            MultiplayersBuildAndRun.PerformWin64Build((int)fieldNumPlayers.value); 
        };
        root.Add(button);
    }

    void OnGUI()
    {
    }
}

// 게임을 빌드하고 여러개의 클라이언트를 자동으로 실행하는 메뉴
public class MultiplayersBuildAndRun
{
    // Windows 64bit 플랫폼으로 빌드하기
    public static void PerformWin64Build(int playerCount)
    {
        if (playerCount <= 0)
            return;
        int numPlayers = Mathf.Clamp(playerCount, 0, 10);

        // Builds/Win64 폴더 아래에 playerCount 개수만큼 exe파일을 생성한 다음 자동으로 실행한다.
        // exe파일을 따로 생성하는 이유는 exe파일마다 독립된 데이터파일이 있어야하기 때문이다.
        EditorUserBuildSettings.SwitchActiveBuildTarget(BuildTargetGroup.Standalone, BuildTarget.StandaloneWindows);
        for(int i=1; i<= numPlayers; i++)
        {
            BuildPipeline.BuildPlayer(GetScenePaths()
                , "Builds/Win64/" + GetProjectName() + i.ToString() + "/" + GetProjectName() + i.ToString() + ".exe"
                , BuildTarget.StandaloneWindows64
                , BuildOptions.AutoRunPlayer);
        }
    }

    // 유니티 메뉴에 추가(파라미터가 있는 메소드는 메뉴에 추가가 안됨)
    [MenuItem("Tools/Run Multiplayer/2 Players")]
    static void PerformWin64Build2()
    {
        PerformWin64Build(2);
    }

    [MenuItem("Tools/Run Multiplayer/3 Players")]
    static void PerformWin64Build3()
    {
        PerformWin64Build(3);
    }

    [MenuItem("Tools/Run Multiplayer/4 Players")]
    static void PerformWin64Build4()
    {
        PerformWin64Build(4);
    }

    // 프로젝트명 가져오기
    static string GetProjectName()
    {
        string[] s = Application.dataPath.Split('/');
        return s[s.Length - 2];
    }

    // Scene 파일들 경로 가져오기
    static string[] GetScenePaths()
    {
        // EditorBuildSettings.scenes 는 유니티 Build Settings 메뉴에 추가되어 있는 Scene 목록이다.
        string[] scenes = new string[EditorBuildSettings.scenes.Length];

        for(int i=0; i<scenes.Length; i++)
        {
            scenes[i] = EditorBuildSettings.scenes[i].path;
        }

        return scenes;
    }
}
