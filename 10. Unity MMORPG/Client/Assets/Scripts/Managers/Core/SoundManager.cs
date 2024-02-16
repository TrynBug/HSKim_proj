using System.Collections;
using System.Collections.Generic;
using UnityEngine;

public class SoundManager
{
    // Sound의 3대 요소
    // MP3 Player    -> AudioSource
    // MP3 음원      -> AudioClip
    // 관객(귀)      -> AudioListener

    // sound를 재생하기 위해서는 AudioSource 컴포넌트가 필요하다.
    // SoundManager를 초기화할 때 이것을 생성한다.
    AudioSource[] _audioSources = new AudioSource[(int)Define.Sound.MaxCount];

    // 효과음 sound 파일을 캐싱하기 위한 멤버
    Dictionary<string, AudioClip> _audioClips = new Dictionary<string, AudioClip>();

    public void Init()
    {
        // "@Sound" 이름의 empty object를 생성한다. 그리고 이 오브젝트는 씬을 넘어가도 파괴되지 않도록 한다.
        GameObject root = GameObject.Find("@Sound");
        if(root == null)
        {
            root = new GameObject { name = "@Sound" };
            Object.DontDestroyOnLoad(root);    // 씬을 넘어가도 파괴되지 않음

            // Sound enum의 각 값마다 AudioSource 컴포넌트가 부착된 오브젝트를 생성하여 _audioSources에 저장한다.
            // 오브젝트들의 부모는 "@Sound" 오브젝트로 한다.
            string[] soundNames = System.Enum.GetNames(typeof(Define.Sound));
            for (int i = 0; i < soundNames.Length - 1; i++)  
            {
                GameObject go = new GameObject { name = soundNames[i] };
                _audioSources[i] = go.AddComponent<AudioSource>();
                go.transform.parent = root.transform;
            }

            // Bgm은 무한반복으로 설정
            _audioSources[(int)Define.Sound.Bgm].loop = true;
        }
    }

    // sound를 stop하고, 캐시를 비운다.
    public void Clear()
    {
        foreach (AudioSource audioSource in _audioSources)
        {
            audioSource.clip = null;
            audioSource.Stop();
        }
        _audioClips.Clear();
    }

    public void Play(string path, Define.Sound type = Define.Sound.Effect, float pitch = 1.0f)
    {
        AudioClip audioClip = GetOrAddAudioClip(path, type);
        Play(audioClip, type, pitch);
    }

    public void Play(AudioClip audioClip, Define.Sound type = Define.Sound.Effect, float pitch = 1.0f)
    {
        if (audioClip == null)
            return;

        switch (type)
        {
            // BGM sound 재생
            case Define.Sound.Bgm:
                {
                    AudioSource audioSource = _audioSources[(int)Define.Sound.Bgm];

                    if (audioSource.isPlaying)  // 이전 BGM이 재생중이라면 멈춤
                        audioSource.Stop();

                    audioSource.pitch = pitch;
                    audioSource.clip = audioClip;
                    audioSource.Play();  // 반복 재생
                    break;
                }

            // 효과음 sound 재생
            case Define.Sound.Effect:
                {
                    AudioSource audioSource = _audioSources[(int)Define.Sound.Effect];
                    audioSource.pitch = pitch;
                    audioSource.PlayOneShot(audioClip);  // 1번만 재생
                    break;
                }
        }
    }

    // audio 파일 캐싱 함수
    AudioClip GetOrAddAudioClip(string path, Define.Sound type = Define.Sound.Effect)
    {
        if (path.Contains("Sounds/") == false)
            path = $"Sounds/{path}";

        AudioClip audioClip = null;
        switch (type)
        {
            // BGM sound 로드
            case Define.Sound.Bgm:
                {
                    audioClip = Managers.Resource.Load<AudioClip>(path);
                    break;
                }

            // 효과음 sound 찾기. 기존에 없었다면 로드함
            case Define.Sound.Effect:
                {
                    if (_audioClips.TryGetValue(path, out audioClip) == false)
                    {
                        audioClip = Managers.Resource.Load<AudioClip>(path);
                        _audioClips.Add(path, audioClip);
                    }
                    break;
                }
        }

        if (audioClip == null)
            Debug.Log($"AudioClip Missing! path:{path}, type:{type.ToString()}");

        return audioClip;
    }

}
