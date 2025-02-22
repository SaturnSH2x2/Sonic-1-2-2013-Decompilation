#ifndef AUDIO_H
#define AUDIO_H

#define TRACK_COUNT (0x10)
#define SFX_COUNT   (0x100)
#if !RETRO_USE_ORIGINAL_CODE
#define CHANNEL_COUNT (0x10) // 4 in the original, 16 for convenience
#else
#define CHANNEL_COUNT (0x4)
#endif

#define MAX_VOLUME (100)

struct TrackInfo {
    char fileName[0x40];
    bool trackLoop;
    uint loopPoint;
};

#if !RETRO_USE_ORIGINAL_CODE
struct MusicPlaybackInfo {
    OggVorbis_File vorbisFile;
    int vorbBitstream;
#if RETRO_USING_SDL1_AUDIO
    SDL_AudioSpec spec;
#endif
#if RETRO_USING_SDL2_AUDIO
    SDL_AudioStream *stream;
#endif
    Sint16 *buffer;
    FileInfo fileInfo;
    bool trackLoop;
    uint loopPoint;
    bool loaded;
};
#endif

struct SFXInfo {
    char name[0x40];
    Sint16 *buffer;
    size_t length;
    bool loaded;

#if RETRO_USE_SDLMIXER
    Mix_Chunk* chunk;
    char panL;
    char panR;
    int channelPlaying;
#endif
};

struct ChannelInfo {
    size_t sampleLength;
    Sint16 *samplePtr;
    int sfxID;
    byte loopSFX;
    sbyte pan;
};

enum MusicStatuses {
    MUSIC_STOPPED = 0,
    MUSIC_PLAYING = 1,
    MUSIC_PAUSED  = 2,
    MUSIC_LOADING = 3,
    MUSIC_READY   = 4,
};

extern int globalSFXCount;
extern int stageSFXCount;

extern int masterVolume;
extern int trackID;
extern int sfxVolume;
extern int bgmVolume;
extern bool audioEnabled;

extern bool musicEnabled;
extern int musicStatus;
extern int musicStartPos;
extern int musicPosition;
extern int musicRatio;
extern TrackInfo musicTracks[TRACK_COUNT];

extern SFXInfo sfxList[SFX_COUNT];
extern char sfxNames[SFX_COUNT][0x40];

extern ChannelInfo sfxChannels[CHANNEL_COUNT];

#if !RETRO_USE_ORIGINAL_CODE
extern MusicPlaybackInfo musInfo;

#if RETRO_USE_SDLMIXER
extern byte* trackData[TRACK_COUNT];
extern SDL_RWops* trackRwops[TRACK_COUNT];
extern byte* sfxData[SFX_COUNT];
extern SDL_RWops* sfxRwops[SFX_COUNT];
#endif

#endif

#if RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2_AUDIO
extern SDL_AudioSpec audioDeviceFormat;
#endif

int InitAudioPlayback();
void LoadGlobalSfx();

#if RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2_AUDIO || RETRO_USE_3DS_AUDIO
#if !RETRO_USE_ORIGINAL_CODE
// These functions did exist, but with different signatures
void ProcessMusicStream(Sint32 *stream, size_t bytes_wanted);
void ProcessAudioPlayback(void *data, Uint8 *stream, int len);
void ProcessAudioMixing(Sint32 *dst, const Sint16 *src, int len, int volume, sbyte pan);
#endif

#if !RETRO_USE_ORIGINAL_CODE
inline void freeMusInfo()
{
    if (musInfo.loaded) {
#if RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2_AUDIO
        SDL_LockAudio();
#endif

        if (musInfo.buffer)
            delete[] musInfo.buffer;
#if RETRO_USING_SDL2_AUDIO
        if (musInfo.stream)
            SDL_FreeAudioStream(musInfo.stream);
#endif
        ov_clear(&musInfo.vorbisFile);
        musInfo.buffer = nullptr;
#if RETRO_USING_SDL2_AUDIO
        musInfo.stream = nullptr;
#endif
        musInfo.trackLoop = false;
        musInfo.loopPoint = 0;
        musInfo.loaded    = false;

#if RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2_AUDIO
        SDL_UnlockAudio();
#endif
    }
}
#endif

#elif RETRO_PLATFORM == RETRO_3DS && !RETRO_USE_SDLMIXER
void ProcessMusicStream(Sint32 *stream, size_t bytes_wanted);
void ProcessAudioPlayback(void *data, Uint8 *stream, int len);
void ProcessAudioMixing(Sint32 *dst, const Sint16 *src, int len, int volume, sbyte pan);

inline void freeMusInfo()
{
    if (musInfo.loaded) {
        if (musInfo.buffer)
            delete[] musInfo.buffer;
        ov_clear(&musInfo.vorbisFile);
        musInfo.buffer = nullptr;
        musInfo.trackLoop = false;
        musInfo.loopPoint = 0;
        musInfo.loaded    = false;
    }
}

#else
void ProcessMusicStream();
void ProcessAudioPlayback();
void ProcessAudioMixing();

#if !RETRO_USE_ORIGINAL_CODE
inline void freeMusInfo()
{
#if RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2_AUDIO
    if (musInfo.loaded) {
        if (musInfo.musicFile)
            delete[] musInfo.musicFile;
        musInfo.musicFile    = nullptr;
        musInfo.buffer       = nullptr;
        musInfo.stream       = nullptr;
        musInfo.pos          = 0;
        musInfo.len          = 0;
        musInfo.currentTrack = nullptr;
        musInfo.loaded       = false;
    }
#endif
}
#endif
#endif

void LoadMusic(void *userdata);
void SetMusicTrack(const char *filePath, byte trackID, bool loop, uint loopPoint);
void SwapMusicTrack(const char *filePath, byte trackID, uint loopPoint, uint ratio);
bool PlayMusic(int track, int musStartPos);
inline void StopMusic(bool setStatus)
{
    if (setStatus)
        musicStatus = MUSIC_STOPPED;
#if !RETRO_USE_ORIGINAL_CODE
  #if RETRO_USING_SDL
    SDL_LockAudio();
  #endif
    freeMusInfo();
  #if RETRO_USING_SDL
    SDL_UnlockAudio();
  #endif
#endif
}

void LoadSfx(char *filePath, byte sfxID);
void PlaySfx(int sfx, bool loop);
inline void StopSfx(int sfx)
{
#if RETRO_USE_SDLMIXER
    if (sfxList[sfx].channelPlaying != -1)
        Mix_HaltChannel(sfxList[sfx].channelPlaying);
    sfxList[sfx].channelPlaying = -1;
#elif RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2_AUDIO
    for (int i = 0; i < CHANNEL_COUNT; ++i) {
        if (sfxChannels[i].sfxID == sfx) {
            MEM_ZERO(sfxChannels[i]);
            sfxChannels[i].sfxID = -1;
        }
    }
#endif
}
void SetSfxAttributes(int sfx, int loopCount, sbyte pan);

void SetSfxName(const char *sfxName, int sfxID);

#if !RETRO_USE_ORIGINAL_CODE
// Helper Funcs
inline bool PlaySFXByName(const char *sfx, sbyte loopCnt)
{
    for (int s = 0; s < globalSFXCount + stageSFXCount; ++s) {
        if (StrComp(sfxNames[s], sfx)) {
            PlaySfx(s, loopCnt);
            return true;
        }
    }
    return false;
}
inline bool StopSFXByName(const char *sfx)
{
    for (int s = 0; s < globalSFXCount + stageSFXCount; ++s) {
        if (StrComp(sfxNames[s], sfx)) {
            StopSfx(s);
            return true;
        }
    }
    return false;
}
#endif

inline void SetMusicVolume(int volume)
{
    if (volume < 0)
        volume = 0;
    if (volume > MAX_VOLUME)
        volume = MAX_VOLUME;
    masterVolume = volume;
}

inline void SetGameVolumes(int bgmVolume, int sfxVolume)
{
    // musicVolumeSetting = bgmVolume;
    SetMusicVolume(masterVolume);
    // sfxVolumeSetting = ((sfxVolume << 7) / 100);
}

inline void PauseSound()
{
    if (musicStatus == MUSIC_PLAYING)
        musicStatus = MUSIC_PAUSED;
}

inline void ResumeSound()
{
    if (musicStatus == MUSIC_PAUSED)
        musicStatus = MUSIC_PLAYING;
}

inline void StopAllSfx()
{
#if !RETRO_USE_ORIGINAL_CODE
#if RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2_AUDIO
    SDL_LockAudio();
#endif
#endif
    for (int i = 0; i < CHANNEL_COUNT; ++i) sfxChannels[i].sfxID = -1;
#if !RETRO_USE_ORIGINAL_CODE
#if RETRO_USING_SDL1_AUDIO || RETRO_USING_SDL2_AUDIO
    SDL_UnlockAudio();
#endif
#endif
}
inline void ReleaseGlobalSfx()
{
    for (int i = globalSFXCount - 1; i >= 0; --i) {
        if (sfxList[i].loaded) {
#if RETRO_USE_SDLMIXER
           if (sfxList[i].chunk) {
                Mix_FreeChunk(sfxList[i].chunk);
	        sfxList[i].chunk = NULL;
		sfxRwops[i] = NULL;
	    }
#endif
            StrCopy(sfxList[i].name, "");
            StrCopy(sfxNames[i], "");
            if (sfxList[i].buffer)
                sys_LinearFree(sfxList[i].buffer);
            sfxList[i].buffer = NULL;
            sfxList[i].length = 0;
            sfxList[i].loaded = false;
        }
    }
    globalSFXCount = 0;
}
inline void ReleaseStageSfx()
{
    for (int i = (stageSFXCount + globalSFXCount) - 1; i >= globalSFXCount; --i) {
        if (sfxList[i].loaded) {
#if RETRO_USE_SDLMIXER
           if (sfxList[i].chunk) {
                Mix_FreeChunk(sfxList[i].chunk);
	        sfxList[i].chunk = NULL;
		sfxRwops[i] = NULL;
	    }
#endif

            StrCopy(sfxList[i].name, "");
            StrCopy(sfxNames[i], "");
            if (sfxList[i].buffer)
                sys_LinearFree(sfxList[i].buffer);
            sfxList[i].buffer = NULL;
            sfxList[i].length = 0;
            sfxList[i].loaded = false;
        }
    }
    stageSFXCount = 0;
}

inline void ReleaseAudioDevice()
{
    StopMusic(true);
    StopAllSfx();
    ReleaseStageSfx();
    ReleaseGlobalSfx();
}

#endif // !AUDIO_H
