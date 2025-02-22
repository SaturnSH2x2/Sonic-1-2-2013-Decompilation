#include "RetroEngine.hpp"

int globalVariablesCount;
int globalVariables[GLOBALVAR_COUNT];
char globalVariableNames[GLOBALVAR_COUNT][0x20];

int (*nativeFunction[16])(int, void *);
int nativeFunctionCount = 0;

char gamePath[0x100];
char modsPath[0x100];
int saveRAM[SAVEDATA_MAX];
Achievement achievements[ACHIEVEMENT_MAX];
LeaderboardEntry leaderboard[LEADERBOARD_MAX];

MultiplayerData multiplayerDataIN  = MultiplayerData();
MultiplayerData multiplayerDataOUT = MultiplayerData();
int matchValueData[0x100];
int matchValueReadPos  = 0;
int matchValueWritePos = 0;

int sendDataMethod = 0;
int sendCounter    = 0;

#if RETRO_USE_MOD_LOADER
std::vector<ModInfo> modList;

#include <filesystem>
#endif

#if RETRO_PLATFORM == RETRO_OSX
#include <sys/stat.h>
#include <sys/types.h>
#endif

#if !RETRO_USE_ORIGINAL_CODE
bool forceUseScripts          = false;
bool skipStartMenu            = false;
bool skipStartMenu_Config     = false;
bool disableFocusPause        = false;
bool disableFocusPause_Config = false;


void InitUserdata()
{
    // userdata files are loaded from this directory
    sprintf(gamePath, "%s", BASE_PATH);
    sprintf(modsPath, "%s", BASE_PATH);

#if RETRO_PLATFORM == RETRO_OSX
    sprintf(gamePath, "%s/RSDKv4", getResourcesPath());
    sprintf(modsPath, "%s/RSDKv4/", getResourcesPath());

    mkdir(gamePath, 0777);
#elif RETRO_PLATFORM == RETRO_ANDROID
    {
        char buffer[0x200];

        JNIEnv *env      = (JNIEnv *)SDL_AndroidGetJNIEnv();
        jobject activity = (jobject)SDL_AndroidGetActivity();
        jclass cls(env->GetObjectClass(activity));
        jmethodID method = env->GetMethodID(cls, "getBasePath", "()Ljava/lang/String;");
        auto ret         = env->CallObjectMethod(activity, method);

        strcpy(buffer, env->GetStringUTFChars((jstring)ret, NULL));

        sprintf(gamePath, "%s", buffer);
        sprintf(modsPath, "%s", buffer);

        env->DeleteLocalRef(activity);
        env->DeleteLocalRef(cls);
    }
#endif

    char buffer[0x100];
#if RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/settings.ini", getResourcesPath());
    else
        sprintf(buffer, "%ssettings.ini", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX || RETRO_PLATFORM == RETRO_ANDROID
    sprintf(buffer, "%s/settings.ini", gamePath);
#else
    sprintf(buffer, BASE_PATH "settings.ini");
#endif
    FileIO *file = fOpen(buffer, "rb");
    if (!file) {
        IniParser ini;

        ini.SetBool("Dev", "DevMenu", Engine.devMenu = false);
        ini.SetBool("Dev", "EngineDebugMode", engineDebugMode = false);
        ini.SetInteger("Dev", "StartingCategory", Engine.startList = 255);
        ini.SetInteger("Dev", "StartingScene", Engine.startStage = 255);
        ini.SetInteger("Dev", "StartingPlayer", Engine.startPlayer = 255);
        ini.SetInteger("Dev", "StartingSaveFile", Engine.startSave = 255);
        ini.SetInteger("Dev", "FastForwardSpeed", Engine.fastForwardSpeed = 8);
        ini.SetBool("Dev", "UseHQModes", Engine.useHQModes = DEFAULT_USE_HQ_MODES);
        ini.SetString("Dev", "DataFile", (char *)"Data.rsdk");
        StrCopy(Engine.dataFile[0], "Data.rsdk");
        if (!StrComp(Engine.dataFile[1], "")) {
            ini.SetString("Dev", "DataFile2", (char *)"Data2.rsdk");
            StrCopy(Engine.dataFile[2], "Data2.rsdk");
        }
        if (!StrComp(Engine.dataFile[2], "")) {
            ini.SetString("Dev", "DataFile3", (char *)"Data3.rsdk");
            StrCopy(Engine.dataFile[2], "Data3.rsdk");
        }
        if (!StrComp(Engine.dataFile[3], "")) {
            ini.SetString("Dev", "DataFile4", (char *)"Data4.rsdk");
            StrCopy(Engine.dataFile[3], "Data4.rsdk");
        }

        ini.SetInteger("Game", "Language", Engine.language = RETRO_EN);
        ini.SetBool("Game", "SkipStartMenu", skipStartMenu = false);
        skipStartMenu_Config = skipStartMenu;
        ini.SetBool("Game", "DisableFocusPause", disableFocusPause = false);
        disableFocusPause_Config = disableFocusPause;

        ini.SetBool("Window", "FullScreen", Engine.startFullScreen = DEFAULT_FULLSCREEN);
        ini.SetBool("Window", "Borderless", Engine.borderless = false);
        ini.SetBool("Window", "VSync", Engine.vsync = DEFAULT_VSYNC);
        ini.SetInteger("Window", "ScalingMode", Engine.scalingMode = RETRO_DEFAULTSCALINGMODE);
        ini.SetInteger("Window", "WindowScale", Engine.windowScale = RETRO_DEFAULTWINDOWSCALE);
        ini.SetInteger("Window", "ScreenWidth", SCREEN_XSIZE = DEFAULT_SCREEN_XSIZE);
        ini.SetInteger("Window", "RefreshRate", Engine.refreshRate = 60);
        ini.SetInteger("Window", "DimLimit", Engine.dimLimit = 300);
        Engine.dimLimit *= Engine.refreshRate;

        ini.SetFloat("Audio", "BGMVolume", bgmVolume / (float)MAX_VOLUME);
        ini.SetFloat("Audio", "SFXVolume", sfxVolume / (float)MAX_VOLUME);

#if RETRO_USING_SDL2
        ini.SetInteger("Keyboard 1", "Up", inputDevice[INPUT_UP].keyMappings = SDL_SCANCODE_UP);
        ini.SetInteger("Keyboard 1", "Down", inputDevice[INPUT_DOWN].keyMappings = SDL_SCANCODE_DOWN);
        ini.SetInteger("Keyboard 1", "Left", inputDevice[INPUT_LEFT].keyMappings = SDL_SCANCODE_LEFT);
        ini.SetInteger("Keyboard 1", "Right", inputDevice[INPUT_RIGHT].keyMappings = SDL_SCANCODE_RIGHT);
        ini.SetInteger("Keyboard 1", "A", inputDevice[INPUT_BUTTONA].keyMappings = SDL_SCANCODE_Z);
        ini.SetInteger("Keyboard 1", "B", inputDevice[INPUT_BUTTONB].keyMappings = SDL_SCANCODE_X);
        ini.SetInteger("Keyboard 1", "C", inputDevice[INPUT_BUTTONC].keyMappings = SDL_SCANCODE_C);
        ini.SetInteger("Keyboard 1", "X", inputDevice[INPUT_BUTTONX].keyMappings = SDL_SCANCODE_A);
        ini.SetInteger("Keyboard 1", "Y", inputDevice[INPUT_BUTTONY].keyMappings = SDL_SCANCODE_S);
        ini.SetInteger("Keyboard 1", "Z", inputDevice[INPUT_BUTTONZ].keyMappings = SDL_SCANCODE_D);
        ini.SetInteger("Keyboard 1", "L", inputDevice[INPUT_BUTTONL].keyMappings = SDL_SCANCODE_Q);
        ini.SetInteger("Keyboard 1", "R", inputDevice[INPUT_BUTTONR].keyMappings = SDL_SCANCODE_E);
        ini.SetInteger("Keyboard 1", "Start", inputDevice[INPUT_START].keyMappings = SDL_SCANCODE_RETURN);
        ini.SetInteger("Keyboard 1", "Select", inputDevice[INPUT_SELECT].keyMappings = SDL_SCANCODE_TAB);

        ini.SetInteger("Controller 1", "Up", inputDevice[INPUT_UP].contMappings = SDL_CONTROLLER_BUTTON_DPAD_UP);
        ini.SetInteger("Controller 1", "Down", inputDevice[INPUT_DOWN].contMappings = SDL_CONTROLLER_BUTTON_DPAD_DOWN);
        ini.SetInteger("Controller 1", "Left", inputDevice[INPUT_LEFT].contMappings = SDL_CONTROLLER_BUTTON_DPAD_LEFT);
        ini.SetInteger("Controller 1", "Right", inputDevice[INPUT_RIGHT].contMappings = SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
        ini.SetInteger("Controller 1", "A", inputDevice[INPUT_BUTTONA].contMappings = SDL_CONTROLLER_BUTTON_A);
        ini.SetInteger("Controller 1", "B", inputDevice[INPUT_BUTTONB].contMappings = SDL_CONTROLLER_BUTTON_B);
        ini.SetInteger("Controller 1", "C", inputDevice[INPUT_BUTTONC].contMappings = SDL_CONTROLLER_BUTTON_X);
        ini.SetInteger("Controller 1", "X", inputDevice[INPUT_BUTTONX].contMappings = SDL_CONTROLLER_BUTTON_Y);
        ini.SetInteger("Controller 1", "Y", inputDevice[INPUT_BUTTONY].contMappings = SDL_CONTROLLER_BUTTON_ZL);
        ini.SetInteger("Controller 1", "Z", inputDevice[INPUT_BUTTONZ].contMappings = SDL_CONTROLLER_BUTTON_ZR);
        ini.SetInteger("Controller 1", "L", inputDevice[INPUT_BUTTONL].contMappings = SDL_CONTROLLER_BUTTON_LEFTSHOULDER);
        ini.SetInteger("Controller 1", "R", inputDevice[INPUT_BUTTONR].contMappings = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
        ini.SetInteger("Controller 1", "Start", inputDevice[INPUT_START].contMappings = SDL_CONTROLLER_BUTTON_START);
        ini.SetInteger("Controller 1", "Select", inputDevice[INPUT_SELECT].contMappings = SDL_CONTROLLER_BUTTON_GUIDE);

        ini.SetFloat("Controller 1", "LStickDeadzone", LSTICK_DEADZONE = 0.3);
        ini.SetFloat("Controller 1", "RStickDeadzone", RSTICK_DEADZONE = 0.3);
        ini.SetFloat("Controller 1", "LTriggerDeadzone", LTRIGGER_DEADZONE = 0.3);
        ini.SetFloat("Controller 1", "RTriggerDeadzone", RTRIGGER_DEADZONE = 0.3);
#endif

#if RETRO_USING_SDL1
        ini.SetInteger("Keyboard 1", "Up", inputDevice[INPUT_UP].keyMappings = SDLK_UP);
        ini.SetInteger("Keyboard 1", "Down", inputDevice[INPUT_DOWN].keyMappings = SDLK_DOWN);
        ini.SetInteger("Keyboard 1", "Left", inputDevice[INPUT_LEFT].keyMappings = SDLK_LEFT);
        ini.SetInteger("Keyboard 1", "Right", inputDevice[INPUT_RIGHT].keyMappings = SDLK_RIGHT);
        ini.SetInteger("Keyboard 1", "A", inputDevice[INPUT_BUTTONA].keyMappings = SDLK_z);
        ini.SetInteger("Keyboard 1", "B", inputDevice[INPUT_BUTTONB].keyMappings = SDLK_x);
        ini.SetInteger("Keyboard 1", "C", inputDevice[INPUT_BUTTONC].keyMappings = SDLK_c);
        ini.SetInteger("Keyboard 1", "X", inputDevice[INPUT_BUTTONX].keyMappings = SDLK_a);
        ini.SetInteger("Keyboard 1", "Y", inputDevice[INPUT_BUTTONY].keyMappings = SDLK_s);
        ini.SetInteger("Keyboard 1", "Z", inputDevice[INPUT_BUTTONZ].keyMappings = SDLK_d);
        ini.SetInteger("Keyboard 1", "L", inputDevice[INPUT_BUTTONL].keyMappings = SDLK_q);
        ini.SetInteger("Keyboard 1", "R", inputDevice[INPUT_BUTTONR].keyMappings = SDLK_e);
        ini.SetInteger("Keyboard 1", "Start", inputDevice[INPUT_START].keyMappings = SDLK_RETURN);
        ini.SetInteger("Keyboard 1", "Select", inputDevice[INPUT_SELECT].keyMappings = SDLK_TAB);

        ini.SetInteger("Controller 1", "Up", inputDevice[INPUT_UP].contMappings = 1);
        ini.SetInteger("Controller 1", "Down", inputDevice[INPUT_DOWN].contMappings = 2);
        ini.SetInteger("Controller 1", "Left", inputDevice[INPUT_LEFT].contMappings = 3);
        ini.SetInteger("Controller 1", "Right", inputDevice[INPUT_RIGHT].contMappings = 4);
        ini.SetInteger("Controller 1", "A", inputDevice[INPUT_BUTTONA].contMappings = 5);
        ini.SetInteger("Controller 1", "B", inputDevice[INPUT_BUTTONB].contMappings = 6);
        ini.SetInteger("Controller 1", "C", inputDevice[INPUT_BUTTONC].contMappings = 7);
        ini.SetInteger("Controller 1", "X", inputDevice[INPUT_BUTTONX].contMappings = 9);
        ini.SetInteger("Controller 1", "Y", inputDevice[INPUT_BUTTONY].contMappings = 10);
        ini.SetInteger("Controller 1", "Z", inputDevice[INPUT_BUTTONZ].contMappings = 11);
        ini.SetInteger("Controller 1", "L", inputDevice[INPUT_BUTTONL].contMappings = 12);
        ini.SetInteger("Controller 1", "R", inputDevice[INPUT_BUTTONR].contMappings = 13);
        ini.SetInteger("Controller 1", "Start", inputDevice[INPUT_START].contMappings = 8);
        ini.SetInteger("Controller 1", "Select", inputDevice[INPUT_SELECT].contMappings = 14);

        ini.SetFloat("Controller 1", "LStickDeadzone", LSTICK_DEADZONE = 0.3);
        ini.SetFloat("Controller 1", "RStickDeadzone", RSTICK_DEADZONE = 0.3);
        ini.SetFloat("Controller 1", "LTriggerDeadzone", LTRIGGER_DEADZONE = 0.3);
        ini.SetFloat("Controller 1", "RTriggerDeadzone", RTRIGGER_DEADZONE = 0.3);
#endif

#if RETRO_PLATFORM == RETRO_3DS
        ini.SetInteger("Controller 1", "Up", inputDevice[INPUT_UP].contMappings = 11);
        ini.SetInteger("Controller 1", "Down", inputDevice[INPUT_DOWN].contMappings = 12);
        ini.SetInteger("Controller 1", "Left", inputDevice[INPUT_LEFT].contMappings = 13);
        ini.SetInteger("Controller 1", "Right", inputDevice[INPUT_RIGHT].contMappings = 14);
        ini.SetInteger("Controller 1", "A", inputDevice[INPUT_BUTTONA].contMappings = 0);
        ini.SetInteger("Controller 1", "B", inputDevice[INPUT_BUTTONB].contMappings = 1);
        ini.SetInteger("Controller 1", "C", inputDevice[INPUT_BUTTONC].contMappings = 2);
        ini.SetInteger("Controller 1", "X", inputDevice[INPUT_BUTTONX].contMappings = 3);
        ini.SetInteger("Controller 1", "Y", inputDevice[INPUT_BUTTONY].contMappings = 16);
        ini.SetInteger("Controller 1", "Z", inputDevice[INPUT_BUTTONZ].contMappings = 17);
        ini.SetInteger("Controller 1", "L", inputDevice[INPUT_BUTTONL].contMappings = 9);
        ini.SetInteger("Controller 1", "R", inputDevice[INPUT_BUTTONR].contMappings = 10);
        ini.SetInteger("Controller 1", "Start", inputDevice[INPUT_START].contMappings = 6);
        ini.SetInteger("Controller 1", "Select", inputDevice[INPUT_SELECT].contMappings = 4);
#endif

        ini.Write(buffer);
    }
    else {
        fClose(file);
        IniParser ini(buffer);

        if (!ini.GetBool("Dev", "DevMenu", &Engine.devMenu))
            Engine.devMenu = false;
        if (!ini.GetBool("Dev", "EngineDebugMode", &engineDebugMode))
            engineDebugMode = false;
        if (!ini.GetInteger("Dev", "StartingCategory", &Engine.startList))
            Engine.startList = 255;
        if (!ini.GetInteger("Dev", "StartingScene", &Engine.startStage))
            Engine.startStage = 255;
        if (!ini.GetInteger("Dev", "StartingPlayer", &Engine.startPlayer))
            Engine.startPlayer = 255;
        if (!ini.GetInteger("Dev", "StartingSaveFile", &Engine.startSave))
            Engine.startSave = 255;
        if (!ini.GetInteger("Dev", "FastForwardSpeed", &Engine.fastForwardSpeed))
            Engine.fastForwardSpeed = 8;
        if (!ini.GetBool("Dev", "UseHQModes", &Engine.useHQModes))
            Engine.useHQModes = true;

        if (!ini.GetString("Dev", "DataFile", Engine.dataFile[0]))
            StrCopy(Engine.dataFile[0], "Data.rsdk");
        if (!StrComp(Engine.dataFile[1], "")) {
            ini.SetString("Dev", "DataFile2", (char *)"Data2.rsdk");
            StrCopy(Engine.dataFile[2], "Data2.rsdk");
        }
        if (!StrComp(Engine.dataFile[2], "")) {
            if (!ini.GetString("Dev", "DataFile3", Engine.dataFile[2]))
                StrCopy(Engine.dataFile[2], "");
        }
        if (!StrComp(Engine.dataFile[3], "")) {
            if (!ini.GetString("Dev", "DataFile4", Engine.dataFile[3]))
                StrCopy(Engine.dataFile[3], "");
        }

        if (!ini.GetInteger("Game", "Language", &Engine.language))
            Engine.language = RETRO_EN;
        if (!ini.GetBool("Game", "SkipStartMenu", &skipStartMenu))
            skipStartMenu = false;
        skipStartMenu_Config = skipStartMenu;
        if (!ini.GetBool("Game", "DisableFocusPause", &disableFocusPause))
            disableFocusPause = false;
        disableFocusPause_Config = disableFocusPause;

        if (!ini.GetBool("Window", "FullScreen", &Engine.startFullScreen))
            Engine.startFullScreen = DEFAULT_FULLSCREEN;
        if (!ini.GetBool("Window", "Borderless", &Engine.borderless))
            Engine.borderless = false;
        if (!ini.GetBool("Window", "VSync", &Engine.vsync))
            Engine.vsync = false;
        if (!ini.GetInteger("Window", "ScalingMode", &Engine.scalingMode))
            Engine.scalingMode = RETRO_DEFAULTSCALINGMODE;
        if (!ini.GetInteger("Window", "WindowScale", &Engine.windowScale))
            Engine.windowScale = 2;
        if (!ini.GetInteger("Window", "ScreenWidth", &SCREEN_XSIZE))
            SCREEN_XSIZE = DEFAULT_SCREEN_XSIZE;
        if (!ini.GetInteger("Window", "RefreshRate", &Engine.refreshRate))
            Engine.refreshRate = 60;
        if (!ini.GetInteger("Window", "DimLimit", &Engine.dimLimit))
            Engine.dimLimit = 300; // 5 mins
        if (Engine.dimLimit >= 0)
            Engine.dimLimit *= Engine.refreshRate;

        float bv = 0, sv = 0;
        if (!ini.GetFloat("Audio", "BGMVolume", &bv))
            bv = 1.0f;
        if (!ini.GetFloat("Audio", "SFXVolume", &sv))
            sv = 1.0f;

        bgmVolume = bv * MAX_VOLUME;
        sfxVolume = sv * MAX_VOLUME;

        if (bgmVolume > MAX_VOLUME)
            bgmVolume = MAX_VOLUME;
        if (bgmVolume < 0)
            bgmVolume = 0;

        if (sfxVolume > MAX_VOLUME)
            sfxVolume = MAX_VOLUME;
        if (sfxVolume < 0)
            sfxVolume = 0;

#if RETRO_USING_SDL2
        if (!ini.GetInteger("Keyboard 1", "Up", &inputDevice[INPUT_UP].keyMappings))
            inputDevice[INPUT_UP].keyMappings = SDL_SCANCODE_UP;
        if (!ini.GetInteger("Keyboard 1", "Down", &inputDevice[INPUT_DOWN].keyMappings))
            inputDevice[INPUT_DOWN].keyMappings = SDL_SCANCODE_DOWN;
        if (!ini.GetInteger("Keyboard 1", "Left", &inputDevice[INPUT_LEFT].keyMappings))
            inputDevice[INPUT_LEFT].keyMappings = SDL_SCANCODE_LEFT;
        if (!ini.GetInteger("Keyboard 1", "Right", &inputDevice[INPUT_RIGHT].keyMappings))
            inputDevice[INPUT_RIGHT].keyMappings = SDL_SCANCODE_RIGHT;
        if (!ini.GetInteger("Keyboard 1", "A", &inputDevice[INPUT_BUTTONA].keyMappings))
            inputDevice[INPUT_BUTTONA].keyMappings = SDL_SCANCODE_Z;
        if (!ini.GetInteger("Keyboard 1", "B", &inputDevice[INPUT_BUTTONB].keyMappings))
            inputDevice[INPUT_BUTTONB].keyMappings = SDL_SCANCODE_X;
        if (!ini.GetInteger("Keyboard 1", "C", &inputDevice[INPUT_BUTTONC].keyMappings))
            inputDevice[INPUT_BUTTONC].keyMappings = SDL_SCANCODE_C;
        if (!ini.GetInteger("Keyboard 1", "X", &inputDevice[INPUT_BUTTONX].keyMappings))
            inputDevice[INPUT_BUTTONX].keyMappings = SDL_SCANCODE_A;
        if (!ini.GetInteger("Keyboard 1", "Y", &inputDevice[INPUT_BUTTONY].keyMappings))
            inputDevice[INPUT_BUTTONY].keyMappings = SDL_SCANCODE_S;
        if (!ini.GetInteger("Keyboard 1", "Z", &inputDevice[INPUT_BUTTONZ].keyMappings))
            inputDevice[INPUT_BUTTONZ].keyMappings = SDL_SCANCODE_D;
        if (!ini.GetInteger("Keyboard 1", "L", &inputDevice[INPUT_BUTTONL].keyMappings))
            inputDevice[INPUT_BUTTONL].keyMappings = SDL_SCANCODE_Q;
        if (!ini.GetInteger("Keyboard 1", "R", &inputDevice[INPUT_BUTTONR].keyMappings))
            inputDevice[INPUT_BUTTONR].keyMappings = SDL_SCANCODE_E;
        if (!ini.GetInteger("Keyboard 1", "Start", &inputDevice[INPUT_START].keyMappings))
            inputDevice[INPUT_START].keyMappings = SDL_SCANCODE_RETURN;
        if (!ini.GetInteger("Keyboard 1", "Select", &inputDevice[INPUT_SELECT].keyMappings))
            inputDevice[INPUT_SELECT].keyMappings = SDL_SCANCODE_TAB;

        if (!ini.GetInteger("Controller 1", "Up", &inputDevice[INPUT_UP].contMappings))
            inputDevice[INPUT_UP].contMappings = SDL_CONTROLLER_BUTTON_DPAD_UP;
        if (!ini.GetInteger("Controller 1", "Down", &inputDevice[INPUT_DOWN].contMappings))
            inputDevice[INPUT_DOWN].contMappings = SDL_CONTROLLER_BUTTON_DPAD_DOWN;
        if (!ini.GetInteger("Controller 1", "Left", &inputDevice[INPUT_LEFT].contMappings))
            inputDevice[INPUT_LEFT].contMappings = SDL_CONTROLLER_BUTTON_DPAD_LEFT;
        if (!ini.GetInteger("Controller 1", "Right", &inputDevice[INPUT_RIGHT].contMappings))
            inputDevice[INPUT_RIGHT].contMappings = SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
        if (!ini.GetInteger("Controller 1", "A", &inputDevice[INPUT_BUTTONA].contMappings))
            inputDevice[INPUT_BUTTONA].contMappings = SDL_CONTROLLER_BUTTON_A;
        if (!ini.GetInteger("Controller 1", "B", &inputDevice[INPUT_BUTTONB].contMappings))
            inputDevice[INPUT_BUTTONB].contMappings = SDL_CONTROLLER_BUTTON_B;
        if (!ini.GetInteger("Controller 1", "C", &inputDevice[INPUT_BUTTONC].contMappings))
            inputDevice[INPUT_BUTTONC].contMappings = SDL_CONTROLLER_BUTTON_X;
        if (!ini.GetInteger("Controller 1", "X", &inputDevice[INPUT_BUTTONX].contMappings))
            inputDevice[INPUT_BUTTONX].contMappings = SDL_CONTROLLER_BUTTON_Y;
        if (!ini.GetInteger("Controller 1", "Y", &inputDevice[INPUT_BUTTONY].contMappings))
            inputDevice[INPUT_BUTTONY].contMappings = SDL_CONTROLLER_BUTTON_ZL;
        if (!ini.GetInteger("Controller 1", "Z", &inputDevice[INPUT_BUTTONZ].contMappings))
            inputDevice[INPUT_BUTTONZ].contMappings = SDL_CONTROLLER_BUTTON_ZR;
        if (!ini.GetInteger("Controller 1", "L", &inputDevice[INPUT_BUTTONL].contMappings))
            inputDevice[INPUT_BUTTONL].contMappings = SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
        if (!ini.GetInteger("Controller 1", "R", &inputDevice[INPUT_BUTTONR].contMappings))
            inputDevice[INPUT_BUTTONR].contMappings = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
        if (!ini.GetInteger("Controller 1", "Start", &inputDevice[INPUT_START].contMappings))
            inputDevice[INPUT_START].contMappings = SDL_CONTROLLER_BUTTON_START;
        if (!ini.GetInteger("Controller 1", "Select", &inputDevice[INPUT_SELECT].contMappings))
            inputDevice[INPUT_SELECT].contMappings = SDL_CONTROLLER_BUTTON_GUIDE;

        if (!ini.GetFloat("Controller 1", "LStickDeadzone", &LSTICK_DEADZONE))
            LSTICK_DEADZONE = 0.3;
        if (!ini.GetFloat("Controller 1", "RStickDeadzone", &RSTICK_DEADZONE))
            RSTICK_DEADZONE = 0.3;
        if (!ini.GetFloat("Controller 1", "LTriggerDeadzone", &LTRIGGER_DEADZONE))
            LTRIGGER_DEADZONE = 0.3;
        if (!ini.GetFloat("Controller 1", "RTriggerDeadzone", &RTRIGGER_DEADZONE))
            RTRIGGER_DEADZONE = 0.3;
#endif

#if RETRO_USING_SDL1
        if (!ini.GetInteger("Keyboard 1", "Up", &inputDevice[INPUT_UP].keyMappings))
            inputDevice[INPUT_UP].keyMappings = SDLK_UP;
        if (!ini.GetInteger("Keyboard 1", "Down", &inputDevice[INPUT_DOWN].keyMappings))
            inputDevice[INPUT_DOWN].keyMappings = SDLK_DOWN;
        if (!ini.GetInteger("Keyboard 1", "Left", &inputDevice[INPUT_LEFT].keyMappings))
            inputDevice[INPUT_LEFT].keyMappings = SDLK_LEFT;
        if (!ini.GetInteger("Keyboard 1", "Right", &inputDevice[INPUT_RIGHT].keyMappings))
            inputDevice[INPUT_RIGHT].keyMappings = SDLK_RIGHT;
        if (!ini.GetInteger("Keyboard 1", "A", &inputDevice[INPUT_BUTTONA].keyMappings))
            inputDevice[INPUT_BUTTONA].keyMappings = SDLK_z;
        if (!ini.GetInteger("Keyboard 1", "B", &inputDevice[INPUT_BUTTONB].keyMappings))
            inputDevice[INPUT_BUTTONB].keyMappings = SDLK_x;
        if (!ini.GetInteger("Keyboard 1", "C", &inputDevice[INPUT_BUTTONC].keyMappings))
            inputDevice[INPUT_BUTTONC].keyMappings = SDLK_c;
        if (!ini.GetInteger("Controller 1", "X", &inputDevice[INPUT_BUTTONX].contMappings))
            inputDevice[INPUT_BUTTONX].contMappings = SDLK_a;
        if (!ini.GetInteger("Controller 1", "Y", &inputDevice[INPUT_BUTTONY].contMappings))
            inputDevice[INPUT_BUTTONY].contMappings = SDLK_s;
        if (!ini.GetInteger("Controller 1", "Z", &inputDevice[INPUT_BUTTONZ].contMappings))
            inputDevice[INPUT_BUTTONZ].contMappings = SDLK_d;
        if (!ini.GetInteger("Controller 1", "L", &inputDevice[INPUT_BUTTONL].contMappings))
            inputDevice[INPUT_BUTTONL].contMappings = SDLK_q;
        if (!ini.GetInteger("Controller 1", "R", &inputDevice[INPUT_BUTTONR].contMappings))
            inputDevice[INPUT_BUTTONR].contMappings = SDLK_e;
        if (!ini.GetInteger("Keyboard 1", "Start", &inputDevice[INPUT_START].keyMappings))
            inputDevice[INPUT_START].keyMappings = SDLK_RETURN;
        if (!ini.GetInteger("Keyboard 1", "Select", &inputDevice[INPUT_SELECT].keyMappings))
            inputDevice[INPUT_SELECT].keyMappings = SDLK_TAB;

        if (!ini.GetInteger("Controller 1", "Up", &inputDevice[INPUT_UP].contMappings))
            inputDevice[INPUT_UP].contMappings = 1;
        if (!ini.GetInteger("Controller 1", "Down", &inputDevice[INPUT_DOWN].contMappings))
            inputDevice[INPUT_DOWN].contMappings = 2;
        if (!ini.GetInteger("Controller 1", "Left", &inputDevice[INPUT_LEFT].contMappings))
            inputDevice[INPUT_LEFT].contMappings = 3;
        if (!ini.GetInteger("Controller 1", "Right", &inputDevice[INPUT_RIGHT].contMappings))
            inputDevice[INPUT_RIGHT].contMappings = 4;
        if (!ini.GetInteger("Controller 1", "A", &inputDevice[INPUT_BUTTONA].contMappings))
            inputDevice[INPUT_BUTTONA].contMappings = 5;
        if (!ini.GetInteger("Controller 1", "B", &inputDevice[INPUT_BUTTONB].contMappings))
            inputDevice[INPUT_BUTTONB].contMappings = 6;
        if (!ini.GetInteger("Controller 1", "C", &inputDevice[INPUT_BUTTONC].contMappings))
            inputDevice[INPUT_BUTTONC].contMappings = 7;
        if (!ini.GetInteger("Controller 1", "X", &inputDevice[INPUT_BUTTONX].contMappings))
            inputDevice[INPUT_BUTTONX].contMappings = 8;
        if (!ini.GetInteger("Controller 1", "Y", &inputDevice[INPUT_BUTTONY].contMappings))
            inputDevice[INPUT_BUTTONY].contMappings = 9;
        if (!ini.GetInteger("Controller 1", "Z", &inputDevice[INPUT_BUTTONZ].contMappings))
            inputDevice[INPUT_BUTTONZ].contMappings = 10;
        if (!ini.GetInteger("Controller 1", "L", &inputDevice[INPUT_BUTTONL].contMappings))
            inputDevice[INPUT_BUTTONL].contMappings = 11;
        if (!ini.GetInteger("Controller 1", "R", &inputDevice[INPUT_BUTTONR].contMappings))
            inputDevice[INPUT_BUTTONR].contMappings = 12;
        if (!ini.GetInteger("Controller 1", "Start", &inputDevice[INPUT_START].contMappings))
            inputDevice[INPUT_START].contMappings = 13;
        if (!ini.GetInteger("Controller 1", "Select", &inputDevice[INPUT_SELECT].contMappings))
            inputDevice[INPUT_SELECT].contMappings = 14;

        if (!ini.GetFloat("Controller 1", "LStickDeadzone", &LSTICK_DEADZONE))
            LSTICK_DEADZONE = 0.3;
        if (!ini.GetFloat("Controller 1", "RStickDeadzone", &RSTICK_DEADZONE))
            RSTICK_DEADZONE = 0.3;
        if (!ini.GetFloat("Controller 1", "LTriggerDeadzone", &LTRIGGER_DEADZONE))
            LTRIGGER_DEADZONE = 0.3;
        if (!ini.GetFloat("Controller 1", "RTriggerDeadzone", &RTRIGGER_DEADZONE))
            RTRIGGER_DEADZONE = 0.3;
#endif

#if RETRO_PLATFORM == RETRO_3DS
        if (!ini.GetInteger("Controller 1", "Up", &inputDevice[INPUT_UP].contMappings))
            inputDevice[INPUT_UP].contMappings = 11;
        if (!ini.GetInteger("Controller 1", "Down", &inputDevice[INPUT_DOWN].contMappings))
            inputDevice[INPUT_DOWN].contMappings = 12;
        if (!ini.GetInteger("Controller 1", "Left", &inputDevice[INPUT_LEFT].contMappings))
            inputDevice[INPUT_LEFT].contMappings = 13;
        if (!ini.GetInteger("Controller 1", "Right", &inputDevice[INPUT_RIGHT].contMappings))
            inputDevice[INPUT_RIGHT].contMappings = 14;
        if (!ini.GetInteger("Controller 1", "A", &inputDevice[INPUT_BUTTONA].contMappings))
            inputDevice[INPUT_BUTTONA].contMappings = 0;
        if (!ini.GetInteger("Controller 1", "B", &inputDevice[INPUT_BUTTONB].contMappings))
            inputDevice[INPUT_BUTTONB].contMappings = 1;
        if (!ini.GetInteger("Controller 1", "C", &inputDevice[INPUT_BUTTONC].contMappings))
            inputDevice[INPUT_BUTTONC].contMappings = 2;
        if (!ini.GetInteger("Controller 1", "X", &inputDevice[INPUT_BUTTONX].contMappings))
            inputDevice[INPUT_BUTTONX].contMappings = 3;
        if (!ini.GetInteger("Controller 1", "Y", &inputDevice[INPUT_BUTTONY].contMappings))
            inputDevice[INPUT_BUTTONY].contMappings = 16;
        if (!ini.GetInteger("Controller 1", "Z", &inputDevice[INPUT_BUTTONZ].contMappings))
            inputDevice[INPUT_BUTTONZ].contMappings = 17;
        if (!ini.GetInteger("Controller 1", "L", &inputDevice[INPUT_BUTTONL].contMappings))
            inputDevice[INPUT_BUTTONL].contMappings = 9;
        if (!ini.GetInteger("Controller 1", "R", &inputDevice[INPUT_BUTTONR].contMappings))
            inputDevice[INPUT_BUTTONR].contMappings = 10;
        if (!ini.GetInteger("Controller 1", "Start", &inputDevice[INPUT_START].contMappings))
            inputDevice[INPUT_START].contMappings = 6;
        if (!ini.GetInteger("Controller 1", "Select", &inputDevice[INPUT_SELECT].contMappings))
            inputDevice[INPUT_SELECT].contMappings = 4;
#endif
    }

#if RETRO_PLATFORM == RETRO_3DS
    if (SCREEN_XSIZE >= 800)
        SCREEN_XSIZE = 800;
    else if (SCREEN_XSIZE >= 400)
        SCREEN_XSIZE = 400;
    else if (SCREEN_XSIZE <= 320)
        SCREEN_XSIZE = 320;
#endif

    SetScreenSize(SCREEN_XSIZE, SCREEN_YSIZE);

#if RETRO_USING_SDL2
    // Support for extra controller types SDL doesn't recognise
#if RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/controllerdb.txt", getResourcesPath());
    else
        sprintf(buffer, "%scontrollerdb.txt", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX || RETRO_PLATFORM == RETRO_ANDROID
    sprintf(buffer, "%s/controllerdb.txt", gamePath);
#else
    sprintf(buffer, BASE_PATH "controllerdb.txt");
#endif
    file = fOpen(buffer, "rb");
    if (file) {
        fClose(file);

        int nummaps = SDL_GameControllerAddMappingsFromFile(buffer);
        if (nummaps >= 0)
            printLog("loaded %d controller mappings from '%s'\n", buffer, nummaps);
    }
#endif

#if RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/UData.bin", getResourcesPath());
    else
        sprintf(buffer, "%sUData.bin", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX
    sprintf(buffer, "%s/UData.bin", gamePath);
#else
    sprintf(buffer, "%sUData.bin", gamePath);
#endif
    file = fOpen(buffer, "rb");
    if (file) {
        fClose(file);
        ReadUserdata();
    }
    else {
        WriteUserdata();
    }
}

void writeSettings()
{
    IniParser ini;

    ini.SetComment("Dev", "DevMenuComment", "Enable this flag to activate dev menu via the ESC key");
    ini.SetBool("Dev", "DevMenu", Engine.devMenu);
    ini.SetComment("Dev", "DebugModeComment",
                   "Enable this flag to activate features used for debugging the engine (may result in slightly slower game speed)");
    ini.SetBool("Dev", "EngineDebugMode", engineDebugMode);
    ini.SetComment("Dev", "SCComment", "Sets the starting category ID");
    ini.SetInteger("Dev", "StartingCategory", Engine.startList);
    ini.SetComment("Dev", "SSComment", "Sets the starting scene ID");
    ini.SetInteger("Dev", "StartingScene", Engine.startStage);
    ini.SetComment("Dev", "SPComment", "Sets the starting player ID");
    ini.SetInteger("Dev", "StartingPlayer", Engine.startPlayer);
    ini.SetComment("Dev", "SSaveComment", "Sets the starting save file ID");
    ini.SetInteger("Dev", "StartingSaveFile", Engine.startSave);
    ini.SetComment("Dev", "FFComment", "Determines how fast the game will be when fastforwarding is active");
    ini.SetInteger("Dev", "FastForwardSpeed", Engine.fastForwardSpeed);
    ini.SetComment(
        "Dev", "UseHQComment",
        "Determines if applicable rendering modes (such as 3D floor from special stages) will render in \"High Quality\" mode or standard mode");
    ini.SetBool("Dev", "UseHQModes", Engine.useHQModes);

    ini.SetComment("Dev", "DataFileComment", "Determines where the first RSDK file will be loaded from");
    ini.SetString("Dev", "DataFile", Engine.dataFile[0]);
    if (!StrComp(Engine.dataFile[1], "")) {
        ini.SetComment("Dev", "DataFileComment2", "Determines where the second RSDK file will be loaded from");
        ini.SetString("Dev", "DataFile2", Engine.dataFile[1]);
    }
    if (!StrComp(Engine.dataFile[2], "")) {
        ini.SetComment("Dev", "DataFileComment3", "Determines where the third RSDK file will be loaded from (normally unused)");
        ini.SetString("Dev", "DataFile3", Engine.dataFile[2]);
    }
    if (!StrComp(Engine.dataFile[3], "")) {
        ini.SetComment("Dev", "DataFileComment4", "Determines where the fourth RSDK file will be loaded from (normally unused)");
        ini.SetString("Dev", "DataFile4", Engine.dataFile[3]);
    }

    ini.SetComment("Game", "LangComment",
                   "Sets the game language (0 = EN, 1 = FR, 2 = IT, 3 = DE, 4 = ES, 5 = JP, 6 = PT, 7 = RU, 8 = KO, 9 = ZH, 10 = ZS)");
    ini.SetInteger("Game", "Language", Engine.language);
    ini.SetComment("Game", "SSMenuComment", "if set to true, disables the start menu");
    ini.SetBool("Game", "SkipStartMenu", skipStartMenu_Config);
    ini.SetComment("Game", "DFPMenuComment", "if set to true, disables the game pausing when focus is lost");
    ini.SetBool("Game", "DisableFocusPause", disableFocusPause);

    ini.SetComment("Window", "FSComment", "Determines if the window will be fullscreen or not");
    ini.SetBool("Window", "FullScreen", Engine.startFullScreen);
    ini.SetComment("Window", "BLComment", "Determines if the window will be borderless or not");
    ini.SetBool("Window", "Borderless", Engine.borderless);
    ini.SetComment("Window", "VSComment", "Determines if VSync will be active or not");
    ini.SetBool("Window", "VSync", Engine.vsync);
    ini.SetComment("Window", "SMComment",
                   "Determines what scaling is used. 0 is nearest neighbour, 1 is integer scale, 2 is sharp bilinear, and 3 is regular bilinear.");
    ini.SetComment("Window", "SMWarning",
                   "Note: Not all scaling options work correctly on certain platforms, as they don't support bilinear filtering.");
    ini.SetInteger("Window", "ScalingMode", Engine.scalingMode);
    ini.SetComment("Window", "WSComment", "How big the window will be");
    ini.SetInteger("Window", "WindowScale", Engine.windowScale);
    ini.SetComment("Window", "SWComment", "How wide the base screen will be in pixels");
    ini.SetInteger("Window", "ScreenWidth", SCREEN_XSIZE);
    ini.SetComment("Window", "RRComment", "Determines the target FPS");
    ini.SetInteger("Window", "RefreshRate", Engine.refreshRate);
    ini.SetComment("Window", "DLComment", "Determines the dim timer in seconds, set to -1 to disable dimming");
    ini.SetInteger("Window", "DimLimit", Engine.dimLimit >= 0 ? Engine.dimLimit / Engine.refreshRate : -1);

    ini.SetFloat("Audio", "BGMVolume", bgmVolume / (float)MAX_VOLUME);
    ini.SetFloat("Audio", "SFXVolume", sfxVolume / (float)MAX_VOLUME);

#if RETRO_PLATFORM != RETRO_3DS
  #if RETRO_USING_SDL2
    ini.SetComment("Keyboard 1", "IK1Comment", "Keyboard Mappings for P1 (Based on: https://wiki.libsdl.org/SDL_Scancode)");
  #endif
  #if RETRO_USING_SDL1
    ini.SetComment("Keyboard 1", "IK1Comment", "Keyboard Mappings for P1 (Based on: https://wiki.libsdl.org/SDLKeycodeLookup)");
  #endif
    ini.SetInteger("Keyboard 1", "Up", inputDevice[INPUT_UP].keyMappings);
    ini.SetInteger("Keyboard 1", "Down", inputDevice[INPUT_DOWN].keyMappings);
    ini.SetInteger("Keyboard 1", "Left", inputDevice[INPUT_LEFT].keyMappings);
    ini.SetInteger("Keyboard 1", "Right", inputDevice[INPUT_RIGHT].keyMappings);
    ini.SetInteger("Keyboard 1", "A", inputDevice[INPUT_BUTTONA].keyMappings);
    ini.SetInteger("Keyboard 1", "B", inputDevice[INPUT_BUTTONB].keyMappings);
    ini.SetInteger("Keyboard 1", "C", inputDevice[INPUT_BUTTONC].keyMappings);
    ini.SetInteger("Keyboard 1", "X", inputDevice[INPUT_BUTTONX].keyMappings);
    ini.SetInteger("Keyboard 1", "Y", inputDevice[INPUT_BUTTONY].keyMappings);
    ini.SetInteger("Keyboard 1", "Z", inputDevice[INPUT_BUTTONZ].keyMappings);
    ini.SetInteger("Keyboard 1", "L", inputDevice[INPUT_BUTTONL].keyMappings);
    ini.SetInteger("Keyboard 1", "R", inputDevice[INPUT_BUTTONR].keyMappings);
    ini.SetInteger("Keyboard 1", "Start", inputDevice[INPUT_START].keyMappings);
    ini.SetInteger("Keyboard 1", "Select", inputDevice[INPUT_SELECT].keyMappings);
#endif

#if RETRO_USING_SDL2
    ini.SetComment("Controller 1", "IC1Comment", "Controller Mappings for P1 (Based on: https://wiki.libsdl.org/SDL_GameControllerButton)");
    ini.SetComment("Controller 1", "IC1Comment2", "Extra buttons can be mapped with the following IDs:");
    ini.SetComment("Controller 1", "IC1Comment3", "CONTROLLER_BUTTON_ZL             = 16");
    ini.SetComment("Controller 1", "IC1Comment4", "CONTROLLER_BUTTON_ZR             = 17");
    ini.SetComment("Controller 1", "IC1Comment5", "CONTROLLER_BUTTON_LSTICK_UP      = 18");
    ini.SetComment("Controller 1", "IC1Comment6", "CONTROLLER_BUTTON_LSTICK_DOWN    = 19");
    ini.SetComment("Controller 1", "IC1Comment7", "CONTROLLER_BUTTON_LSTICK_LEFT    = 20");
    ini.SetComment("Controller 1", "IC1Comment8", "CONTROLLER_BUTTON_LSTICK_RIGHT   = 21");
    ini.SetComment("Controller 1", "IC1Comment9", "CONTROLLER_BUTTON_RSTICK_UP      = 22");
    ini.SetComment("Controller 1", "IC1Comment10", "CONTROLLER_BUTTON_RSTICK_DOWN    = 23");
    ini.SetComment("Controller 1", "IC1Comment11", "CONTROLLER_BUTTON_RSTICK_LEFT    = 24");
    ini.SetComment("Controller 1", "IC1Comment12", "CONTROLLER_BUTTON_RSTICK_RIGHT   = 25");
#endif

#if RETRO_PLATFORM == RETRO_3DS
    ini.SetComment("Controller 1", "IC1Comment", "Controller Mappings for P1 (Based on: https://wiki.libsdl.org/SDL_GameControllerButton)");
    ini.SetComment("Controller 1", "IC1Comment2", "(0 = A, 1 = B, 2 = Y, 3 = X, 4 = Select, 5 = Touch, 6 = Start, 9 = L, 10 = R, 11 = Up, 12 = Down, 13 = Left, 14 = Right, 16 = ZL, 17 = ZR)");
#endif
    ini.SetInteger("Controller 1", "Up", inputDevice[INPUT_UP].contMappings);
    ini.SetInteger("Controller 1", "Down", inputDevice[INPUT_DOWN].contMappings);
    ini.SetInteger("Controller 1", "Left", inputDevice[INPUT_LEFT].contMappings);
    ini.SetInteger("Controller 1", "Right", inputDevice[INPUT_RIGHT].contMappings);
    ini.SetInteger("Controller 1", "A", inputDevice[INPUT_BUTTONA].contMappings);
    ini.SetInteger("Controller 1", "B", inputDevice[INPUT_BUTTONB].contMappings);
    ini.SetInteger("Controller 1", "C", inputDevice[INPUT_BUTTONC].contMappings);
    ini.SetInteger("Controller 1", "X", inputDevice[INPUT_BUTTONX].contMappings);
    ini.SetInteger("Controller 1", "Y", inputDevice[INPUT_BUTTONY].contMappings);
    ini.SetInteger("Controller 1", "Z", inputDevice[INPUT_BUTTONZ].contMappings);
    ini.SetInteger("Controller 1", "L", inputDevice[INPUT_BUTTONL].contMappings);
    ini.SetInteger("Controller 1", "R", inputDevice[INPUT_BUTTONR].contMappings);
    ini.SetInteger("Controller 1", "Start", inputDevice[INPUT_START].contMappings);
    ini.SetInteger("Controller 1", "Select", inputDevice[INPUT_SELECT].contMappings);

#if RETRO_PLATFORM != RETRO_3DS
    ini.SetComment("Controller 1", "DeadZoneComment", "Deadzones, 0.0-1.0");
    ini.SetFloat("Controller 1", "LStickDeadzone", LSTICK_DEADZONE);
    ini.SetFloat("Controller 1", "RStickDeadzone", RSTICK_DEADZONE);
    ini.SetFloat("Controller 1", "LTriggerDeadzone", LTRIGGER_DEADZONE);
    ini.SetFloat("Controller 1", "RTriggerDeadzone", RTRIGGER_DEADZONE);
#endif

    char buffer[0x100];

#if RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/settings.ini", getResourcesPath());
    else
        sprintf(buffer, "%ssettings.ini", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX
    sprintf(buffer, "%s/settings.ini", gamePath);
#else
    sprintf(buffer, "%ssettings.ini", gamePath);
#endif

    ini.Write(buffer);
}

void ReadUserdata()
{
    char buffer[0x100];
#if RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/UData.bin", getResourcesPath());
    else
        sprintf(buffer, "%sUData.bin", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX
    sprintf(buffer, "%s/UData.bin", gamePath);
#else
    sprintf(buffer, "%sUData.bin", gamePath);
#endif
    FileIO *userFile = fOpen(buffer, "rb");
    if (!userFile)
        return;

    int buf = 0;
    for (int a = 0; a < ACHIEVEMENT_MAX; ++a) {
        fRead(&buf, 4, 1, userFile);
        achievements[a].status = buf;
    }
    for (int l = 0; l < LEADERBOARD_MAX; ++l) {
        fRead(&buf, 4, 1, userFile);
        leaderboard[l].status = buf;
    }

    fClose(userFile);

    if (Engine.onlineActive) {
        // Load from online
    }
}

void WriteUserdata()
{
    char buffer[0x100];
#if RETRO_PLATFORM == RETRO_UWP
    if (!usingCWD)
        sprintf(buffer, "%s/UData.bin", getResourcesPath());
    else
        sprintf(buffer, "%sUData.bin", gamePath);
#elif RETRO_PLATFORM == RETRO_OSX
    sprintf(buffer, "%s/UData.bin", gamePath);
#else
    sprintf(buffer, "%sUData.bin", gamePath);
#endif
    FileIO *userFile = fOpen(buffer, "wb");
    if (!userFile)
        return;

    for (int a = 0; a < ACHIEVEMENT_MAX; ++a) fWrite(&achievements[a].status, 4, 1, userFile);
    for (int l = 0; l < LEADERBOARD_MAX; ++l) fWrite(&leaderboard[l].status, 4, 1, userFile);

    fClose(userFile);

    if (Engine.onlineActive) {
        // Load from online
    }
}
#endif

void AwardAchievement(int id, int status)
{
    if (id < 0 || id >= ACHIEVEMENT_MAX)
        return;

    if (status == 100 && status != achievements[id].status)
        printLog("Achieved achievement: %s (%d)!", achievements[id].name, status);

    achievements[id].status = status;

    if (Engine.onlineActive) {
        // Set Achievement online
    }
#if !RETRO_USE_ORIGINAL_CODE
    WriteUserdata();
#endif
}

int SetAchievement(int achievementID, void *achDone)
{
    int achievementDone = static_cast<int>(reinterpret_cast<intptr_t>(achDone));
    if (!Engine.trialMode && !debugMode) {
        AwardAchievement(achievementID, achievementDone);
        return 1;
    }
    return 0;
}
int SetLeaderboard(int leaderboardID, void *res)
{
    int result = static_cast<int>(reinterpret_cast<intptr_t>(res));
    if (!Engine.trialMode && !debugMode) {
        printLog("Set leaderboard (%d) value to %d", leaderboard, result);
        switch (leaderboardID) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
            case 6:
            case 7:
            case 8:
            case 9:
            case 10:
            case 11:
            case 12:
            case 13:
            case 14:
            case 15:
            case 16:
            case 17:
            case 18:
            case 19:
            case 20:
            case 21: leaderboard[leaderboardID].status = result;
#if !RETRO_USE_ORIGINAL_CODE
                WriteUserdata();
#endif
                return 1;
        }
    }
    return 0;
}

int Connect2PVS(int gameLength, void *itemMode)
{
    printLog("Attempting to connect to 2P game (%d) (%p)", gameLength, itemMode);

    multiplayerDataIN.type = 0;
    matchValueData[0]      = 0;
    matchValueData[1]      = 0;
    matchValueReadPos      = 0;
    matchValueWritePos     = 0;
    Engine.gameMode        = ENGINE_CONNECT2PVS;
    PauseSound();

    // actual connection code
    if (Engine.onlineActive) {
        // Do online code
        return 1;
    }
    return 0;
}
int Disconnect2PVS(int a1, void *a2)
{
    printLog("Attempting to disconnect from 2P game (%d) (%p)", a1, a2);

    if (Engine.onlineActive) {
#if RETRO_USE_NETWORKING
        sendData(0, sizeof(multiplayerDataOUT), &multiplayerDataOUT);
#endif
        return 1;
    }
    return 0;
}
int SendEntity(int dataSlot, void *entityID)
{
    printLog("Attempting to send entity (%d) (%p)", dataSlot, entityID);

    if (!sendCounter) {
        multiplayerDataOUT.type = 1;
        memcpy(multiplayerDataOUT.data, &objectEntityList[static_cast<int>(reinterpret_cast<intptr_t>(entityID))], sizeof(Entity));
        if (Engine.onlineActive) {
#if RETRO_USE_NETWORKING
            sendData(0, sizeof(multiplayerDataOUT), &multiplayerDataOUT);
#endif
            return 1;
        }
    }
    sendCounter += 1;
    sendCounter %= 2;
    return 0;
}
int SendValue(int a1, void *value)
{
    printLog("Attempting to send value (%d) (%p)", a1, value);

    multiplayerDataOUT.type    = 0;
    multiplayerDataOUT.data[0] = static_cast<int>(reinterpret_cast<intptr_t>(value));
    if (Engine.onlineActive && sendDataMethod) {
#if RETRO_USE_NETWORKING
        sendData(0, sizeof(multiplayerDataOUT), &multiplayerDataOUT);
#endif
        return 1;
    }
    return 0;
}
int ReceiveEntity(int dataSlotID, void *entityID)
{
    printLog("Attempting to receive entity (%d) (%p)", dataSlotID, entityID);

    if (Engine.onlineActive) {
        // Do online code
        int entitySlot = static_cast<int>(reinterpret_cast<intptr_t>(entityID));
        if (dataSlotID == 1) {
            if (multiplayerDataIN.type == 1) {
                memcpy(&objectEntityList[entitySlot], multiplayerDataIN.data, sizeof(Entity));
            }
            multiplayerDataIN.type = 0;
        }
        else {
            memcpy(&objectEntityList[entitySlot], multiplayerDataIN.data, sizeof(Entity));
        }
    }
    return 0;
}
int ReceiveValue(int dataSlot, void *value)
{
    printLog("Attempting to receive value (%d) (%p)", dataSlot, value);

    if (Engine.onlineActive) {
        // Do online code
        int *val = (int *)value;

        if (dataSlot == 1) {
            if (matchValueReadPos != matchValueWritePos) {
                *val = matchValueData[matchValueReadPos];
                matchValueReadPos++;
            }
        }
        else {
            *val = matchValueData[matchValueReadPos];
        }
        return 1;
    }
    return 0;
}
int TransmitGlobal(int globalValue, void *globalName)
{
    printLog("Attempting to transmit global (%s) (%d)", (char *)globalName, globalValue);

    multiplayerDataOUT.type    = 2;
    multiplayerDataOUT.data[0] = GetGlobalVariableID((char *)globalName);
    multiplayerDataOUT.data[1] = globalValue;
    if (Engine.onlineActive && sendDataMethod) {
#if RETRO_USE_NETWORKING
        sendData(0, sizeof(multiplayerDataOUT), &multiplayerDataOUT);
#endif
        return 1;
    }
    return 0;
}

void receive2PVSData(MultiplayerData *data)
{
    switch (data->type) {
        case 0: matchValueData[matchValueWritePos++] = data->data[0]; break;
        case 1:
            multiplayerDataIN.type = 1;
            memcpy(multiplayerDataIN.data, data->data, sizeof(Entity));
            break;
        case 2: globalVariables[data->data[0]] = data->data[1]; break;
    }
}

void receive2PVSMatchCode(int code)
{
    matchValueData[matchValueWritePos++] = code;
    ResumeSound();
    Engine.gameMode = ENGINE_MAINGAME;
}

int ShowPromoPopup(int a1, void *a2)
{
    printLog("Attempting to show promo popup (%d) (%p)", a1, a2);
    if (Engine.onlineActive) {
        // Do online code
        return 1;
    }
    return 0;
}

int ExitGame(int val, void *name)
{
    Engine.running = false;
    return 1;
}

int OpenModMenu(int val, void *name)
{
#if RETRO_USE_MOD_LOADER
    Engine.gameMode = ENGINE_INITMODMENU;
    Engine.modMenuCalled = true;
#endif
    return 1;
}

#if RETRO_USE_MOD_LOADER
#include <string>

#if RETRO_PLATFORM == RETRO_ANDROID
namespace fs = std::__fs::filesystem;
#else
namespace fs = std::filesystem;
#endif

void initMods()
{
    modList.clear();
    forceUseScripts = false;

    char modBuf[0x100];
    sprintf(modBuf, "%smods/", modsPath);
    fs::path modPath(modBuf);

    if (fs::exists(modPath) && fs::is_directory(modPath)) {
        std::string mod_config = modPath.string() + "/modconfig.ini";
        FileIO *configFile     = fOpen(mod_config.c_str(), "r");
        if (configFile) {
            fClose(configFile);
            IniParser modConfig(mod_config.c_str(), false);

            for (int m = 0; m < modConfig.items.size(); ++m) {
                bool active = false;
                ModInfo info;
                modConfig.GetBool("mods", modConfig.items[m].key, &active);
                if (loadMod(&info, modPath.string(), modConfig.items[m].key, active))
                    modList.push_back(info);
            }
        }


        try {
            auto rdi = fs::directory_iterator(modPath);
            for (auto de : rdi) {
                //printf("%s\n", de.path().string().c_str());
                if (de.is_directory()) {
                    fs::path modDirPath = de.path();

                    ModInfo info;

                    std::string modDir            = modDirPath.string().c_str();
                    const std::string mod_inifile = modDir + "/mod.ini";
                    std::string folder            = modDirPath.filename().string();

                    bool flag = true;
                    for (int m = 0; m < modList.size(); ++m) {
                        if (modList[m].folder == folder) {
                            flag = false;
                            break;
                        }
                    }

                    if (flag) {
                        if (loadMod(&info, modPath.string(), modDirPath.filename().string(), false))
                            modList.push_back(info);
                    }
                }
            }
        } catch (fs::filesystem_error fe) {
            printLog("Mods Folder Scanning Error: ");
            printLog(fe.what());
        }
    }
}
bool loadMod(ModInfo *info, std::string modsPath, std::string folder, bool active)
{
    if (!info)
        return false;

    info->fileMap.clear();
    info->name                = "";
    info->desc                = "";
    info->author              = "";
    info->version             = "";
    info->folder              = "";
    info->active              = false;

    const std::string modDir = modsPath + folder;
    //printf("%s\n", modDir.c_str());
    FileIO *f = fOpen((modDir + "/mod.ini").c_str(), "r");
    if (f) {
        fClose(f);
        IniParser modSettings((modDir + "/mod.ini").c_str(), false);

        info->name    = "Unnamed Mod";
        info->desc    = "";
        info->author  = "Unknown Author";
        info->version = "1.0.0";
        info->folder  = folder;

        char infoBuf[0x100];
        // Name
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Name", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->name = infoBuf;
        // Desc
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Description", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->desc = infoBuf;
        // Author
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Author", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->author = infoBuf;
        // Version
        StrCopy(infoBuf, "");
        modSettings.GetString("", "Version", infoBuf);
        if (!StrComp(infoBuf, ""))
            info->version = infoBuf;

        info->active = active;

        // Check for Data/ replacements
        fs::path dataPath(modDir + "/Data");

        if (fs::exists(dataPath) && fs::is_directory(dataPath)) {
            try {
                auto data_rdi = fs::recursive_directory_iterator(dataPath);
                for (auto data_de : data_rdi) {
                    //printf("%s\n", data_de.path().string().c_str());
                    if (data_de.is_regular_file()) {
                        char modBuf[0x100];
                        StrCopy(modBuf, data_de.path().string().c_str());
                        char folderTest[4][0x10] = {
                            "Data/",
                            "Data\\",
                            "data/",
                            "data\\",
                        };
                        int tokenPos = -1;
                        for (int i = 0; i < 4; ++i) {
                            tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                            if (tokenPos >= 0)
                                break;
                        }

                        if (tokenPos >= 0) {
                            char buffer[0x80];
                            for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                                buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                            }

                            // printLog(modBuf);
                            std::string path(buffer);
                            std::string modPath(modBuf);
                            char pathLower[0x100];
                            memset(pathLower, 0, sizeof(char) * 0x100);
                            for (int c = 0; c < path.size(); ++c) {
                                pathLower[c] = tolower(path.c_str()[c]);
                            }

                            info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                        }
                    }
                }
            } catch (fs::filesystem_error fe) {
                printLog("Data Folder Scanning Error: ");
                printLog(fe.what());
            }
        }

        // Check for Scripts/ replacements
        fs::path scriptPath(modDir + "/Scripts");

        if (fs::exists(scriptPath) && fs::is_directory(scriptPath)) {
            try {
                auto data_rdi = fs::recursive_directory_iterator(scriptPath);
                for (auto data_de : data_rdi) {
                    if (data_de.is_regular_file()) {
                        char modBuf[0x100];
                        StrCopy(modBuf, data_de.path().string().c_str());
                        char folderTest[4][0x10] = {
                            "Scripts/",
                            "Scripts\\",
                            "scripts/",
                            "scripts\\",
                        };
                        int tokenPos = -1;
                        for (int i = 0; i < 4; ++i) {
                            tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                            if (tokenPos >= 0)
                                break;
                        }

                        if (tokenPos >= 0) {
                            char buffer[0x80];
                            for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                                buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                            }

                            // printLog(modBuf);
                            std::string path(buffer);
                            std::string modPath(modBuf);
                            char pathLower[0x100];
                            memset(pathLower, 0, sizeof(char) * 0x100);
                            for (int c = 0; c < path.size(); ++c) {
                                pathLower[c] = tolower(path.c_str()[c]);
                            }

                            info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                        }
                    }
                }
            } catch (fs::filesystem_error fe) {
                printLog("Script Folder Scanning Error: ");
                printLog(fe.what());
            }
        }

        // Check for Bytecode/ replacements
        fs::path bytecodePath(modDir + "/Bytecode");

        if (fs::exists(bytecodePath) && fs::is_directory(bytecodePath)) {
            try {
                auto data_rdi = fs::recursive_directory_iterator(bytecodePath);
                for (auto data_de : data_rdi) {
                    if (data_de.is_regular_file()) {
                        char modBuf[0x100];
                        StrCopy(modBuf, data_de.path().string().c_str());
                        char folderTest[4][0x10] = {
                            "Bytecode/",
                            "Bytecode\\",
                            "bytecode/",
                            "bytecode\\",
                        };
                        int tokenPos = -1;
                        for (int i = 0; i < 4; ++i) {
                            tokenPos = FindStringToken(modBuf, folderTest[i], 1);
                            if (tokenPos >= 0)
                                break;
                        }

                        if (tokenPos >= 0) {
                            char buffer[0x80];
                            for (int i = StrLength(modBuf); i >= tokenPos; --i) {
                                buffer[i - tokenPos] = modBuf[i] == '\\' ? '/' : modBuf[i];
                            }

                            // printLog(modBuf);
                            std::string path(buffer);
                            std::string modPath(modBuf);
                            char pathLower[0x100];
                            memset(pathLower, 0, sizeof(char) * 0x100);
                            for (int c = 0; c < path.size(); ++c) {
                                pathLower[c] = tolower(path.c_str()[c]);
                            }

                            info->fileMap.insert(std::pair<std::string, std::string>(pathLower, modBuf));
                        }
                    }
                }
            } catch (fs::filesystem_error fe) {
                printLog("Bytecode Folder Scanning Error: ");
                printLog(fe.what());
            }
        }

        info->useScripts = false;
        modSettings.GetBool("", "TxtScripts", &info->useScripts);
        if (info->useScripts && info->active)
            forceUseScripts = true;

        info->skipStartMenu = false;
        modSettings.GetBool("", "SkipStartMenu", &info->skipStartMenu);
        if (info->skipStartMenu && info->active)
            skipStartMenu = true;
        info->disableFocusPause = false;
        modSettings.GetBool("", "DisableFocusPause", &info->disableFocusPause);
        if (info->disableFocusPause && info->active)
            disableFocusPause = true;
        return true;
    }
    return false;
}

void saveMods()
{
    char modBuf[0x100];
    sprintf(modBuf, "%smods/", modsPath);
    fs::path modPath(modBuf);

    if (fs::exists(modPath) && fs::is_directory(modPath)) {
        std::string mod_config = modPath.string() + "modconfig.ini";
        IniParser modConfig;
        printf("%s\n", mod_config.c_str());
        for (int m = 0; m < modList.size(); ++m) {
            ModInfo *info                 = &modList[m];

            modConfig.SetBool("mods", info->folder.c_str(), info->active);
        }

        modConfig.Write(mod_config.c_str(), false);
    }
}
#endif
