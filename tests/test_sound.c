#include "jpb/camera.h"
#include "jpb/game.h"
#include "jpb/sound.h"

#include <limits.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition)                                                     \
    do {                                                                     \
        if (!(condition)) {                                                  \
            fprintf(                                                         \
                stderr,                                                      \
                "CHECK failed at %s:%d: %s\n",                             \
                __FILE__,                                                    \
                __LINE__,                                                    \
                #condition);                                                 \
            return 1;                                                        \
        }                                                                    \
    } while (0)

typedef struct SoundTrace {
    int calls;
    int banks[8];
    uint16_t results[8];
    VECTOR *expected_position;
    VECTOR expected_position_value;
    int compare_position_by_value;
    char *expected_sound;
    uint32_t expected_flag;
    int arguments_match;
} SoundTrace;

typedef struct SoundControlTrace {
    int stop_calls;
    int fade_calls;
    uint16_t stop_handle;
    uint16_t fade_handle;
    uint32_t fade_time;
    JPBSoundControl controls[16];
    int control_calls;
} SoundControlTrace;

typedef struct SoundBankTrace {
    int calls;
    int load_calls;
    int unload_calls;
    int last_bank_id;
    const char *last_directory;
    const char *const *last_paths;
    int last_count;
    int accept_load;
} SoundBankTrace;

static uint16_t trace_play_sfx(
    VECTOR *position,
    int bankId,
    char *sound,
    uint32_t flag,
    void *user_data)
{
    SoundTrace *trace = (SoundTrace *)user_data;
    int call = trace->calls++;

    if (call >= (int)(sizeof(trace->banks) / sizeof(trace->banks[0]))) {
        trace->arguments_match = 0;
        return 0;
    }
    trace->banks[call] = bankId;
    if ((!trace->compare_position_by_value &&
         position != trace->expected_position) ||
        (trace->compare_position_by_value &&
         (position->vx != trace->expected_position_value.vx ||
          position->vy != trace->expected_position_value.vy ||
          position->vz != trace->expected_position_value.vz ||
          position->pad != trace->expected_position_value.pad)) ||
        sound != trace->expected_sound ||
        flag != trace->expected_flag) {
        trace->arguments_match = 0;
    }
    return trace->results[call];
}

static void reset_trace(
    SoundTrace *trace, VECTOR *position, char *sound, uint32_t flag)
{
    memset(trace, 0, sizeof(*trace));
    trace->expected_position = position;
    trace->expected_sound = sound;
    trace->expected_flag = flag;
    trace->arguments_match = 1;
    jpb_SoundSetPlaySfxHook(trace_play_sfx, trace);
}

static void trace_stop(uint16_t handle, void *user_data)
{
    SoundControlTrace *trace = (SoundControlTrace *)user_data;

    ++trace->stop_calls;
    trace->stop_handle = handle;
}

static void trace_fade(
    uint16_t handle,
    uint32_t fade_time,
    void *user_data)
{
    SoundControlTrace *trace = (SoundControlTrace *)user_data;

    ++trace->fade_calls;
    trace->fade_handle = handle;
    trace->fade_time = fade_time;
}

static void trace_control(
    JPBSoundControl control, void *user_data)
{
    SoundControlTrace *trace = (SoundControlTrace *)user_data;

    if (trace->control_calls <
        (int)(sizeof(trace->controls) / sizeof(trace->controls[0]))) {
        trace->controls[trace->control_calls] = control;
    }
    ++trace->control_calls;
}

static int trace_bank(
    int bank_id,
    const char *directory,
    const char *const *paths,
    int count,
    int load,
    void *user_data)
{
    SoundBankTrace *trace = (SoundBankTrace *)user_data;

    ++trace->calls;
    trace->last_bank_id = bank_id;
    trace->last_directory = directory;
    trace->last_paths = paths;
    trace->last_count = count;
    if (load != 0) {
        ++trace->load_calls;
        return trace->accept_load;
    }
    ++trace->unload_calls;
    return 1;
}

static int test_helpers_and_default_options(void)
{
    _svector short_vector = {-7, 12, 32767, 99};
    VECTOR vector = convert_svector_to_vector(short_vector);

    CHECK(strcmp(
        ExtractFileNameFromPath("a\\mixed/path/sound.wav"),
        "sound.wav") == 0);
    CHECK(strcmp(ExtractFileNameFromPath("plain.wav"), "plain.wav") == 0);
    CHECK(ExtractFileNameFromPath(NULL) == NULL);
    CHECK(vector.vx == -7);
    CHECK(vector.vy == 12);
    CHECK(vector.vz == 32767);
    CHECK(vector.pad == 0);

    memset(&OptionStruct, 0, sizeof(OptionStruct));
    game_setDefaultOptions();
    CHECK(memcmp(
        &OptionStruct,
        &defaultOptionStruct,
        sizeof(OptionStruct)) == 0);
    CHECK(OptionStruct.Stereo == 1);
    CHECK(OptionStruct.Music == 1);
    CHECK(OptionStruct.SFX == 1);
    CHECK(OptionStruct.musicVolume == 30);
    CHECK(OptionStruct.SFXVolume == 30);
    CHECK(OptionStruct.ControllerConfig[0] == 1);
    CHECK(OptionStruct.WalkLimit[0] == 2);
    CHECK(OptionStruct.RunLimit[0] == 8);
    CHECK(OptionStruct.ScreenWidth == 1920);
    CHECK(OptionStruct.ScreenHeight == 1080);

    OptionStruct.Stereo = 0;
    OptionStruct.Music = 0;
    OptionStruct.musicVolume = 0;
    OptionStruct.SFXVolume = 0;
    OptionStruct.PadAudioEnabled = 0;
    OptionStruct.SFX = 9;
    game_setAudioOptions();
    CHECK(OptionStruct.Stereo == 1);
    CHECK(OptionStruct.Music == 1);
    CHECK(OptionStruct.musicVolume == 30);
    CHECK(OptionStruct.SFXVolume == 30);
    CHECK(OptionStruct.PadAudioEnabled == 1);
    CHECK(OptionStruct.SFX == 9);

    OptionStruct.ControllerConfig[1] = 0;
    OptionStruct.WalkLimit[1] = 0;
    OptionStruct.RunLimit[1] = 0;
    OptionStruct.ShockFlag[1] = 0;
    game_setControlsOptions(1);
    CHECK(OptionStruct.ControllerConfig[1] == 1);
    CHECK(OptionStruct.WalkLimit[1] == 2);
    CHECK(OptionStruct.RunLimit[1] == 8);
    CHECK(OptionStruct.ShockFlag[1] == 1);
    return 0;
}

static int test_bank_lifecycle(void)
{
    SoundBankTrace trace = {0};
    char unknown[] = "does_not_exist";

    trace.accept_load = 1;
    jpb_SoundSetBankHook(trace_bank, &trace);
    sound_Init();
    CHECK(trace.calls == 1);
    CHECK(trace.load_calls == 1);
    CHECK(trace.last_bank_id == 0);
    CHECK(strcmp(trace.last_directory, "resident/") == 0);
    CHECK(trace.last_count == 41);
    CHECK(sound_NumInBank(0) == 41);
    CHECK(strcmp(sound_GetSoundName(0, 0), "resident/sabrhit7.wav") == 0);
    CHECK(strcmp(sound_GetSoundName(0, 22), "resident/sabrsw01.wav") == 0);
    CHECK(sound_GetSoundName(0, 41) == NULL);

    CHECK(sound_LoadBank("jar_jar_playable", 1) == 0);
    CHECK(sound_NumInBank(1) == 8);
    CHECK(strcmp(
        sound_GetSoundName(1, 0),
        "gungan_2/gungstaf.wav") == 0);
    CHECK(strcmp(trace.last_directory, "gungan_2/") == 0);
    CHECK(trace.last_bank_id == 1);

    CHECK(sound_LoadBank("FeD", 3) == 0);
    CHECK(sound_NumInBank(3) == 40);
    CHECK(strcmp(sound_GetSoundName(3, 0), "fed/dfgetht.wav") == 0);
    CHECK(sound_LoadBank("training_level", 3) == 0);
    CHECK(sound_NumInBank(3) == 7);
    CHECK(strcmp(sound_GetSoundName(3, 0), "tato/probmove.wav") == 0);
    CHECK(strcmp(sound_GetSoundName(3, 5), "mini1/vbdhit2.wav") == 0);
    CHECK(strcmp(sound_GetSoundName(3, 6), "mini4/xtimerbp.wav") == 0);
    CHECK(sound_LoadBank(unknown, 2) == -1);
    CHECK(sound_LoadBank("fed", -1) == -1);
    CHECK(sound_LoadBank("fed", 5) == -1);

    sound_FreeBank(1);
    CHECK(sound_NumInBank(1) == 0);
    CHECK(trace.unload_calls == 1);
    CHECK(trace.last_bank_id == 1);

    trace.accept_load = 0;
    CHECK(sound_LoadBank("theed", 2) == -1);
    CHECK(sound_NumInBank(2) == 0);
    CHECK(trace.last_paths != NULL);
    jpb_SoundSetBankHook(NULL, NULL);
    sound_FreeBank(0);
    sound_FreeBank(3);
    return 0;
}

static int test_sound_play_fallback_order(void)
{
    VECTOR position = {1, 2, 3, 0};
    char sound[] = "jedihit";
    SoundTrace trace;

    reset_trace(&trace, &position, sound, 7);
    trace.results[0] = 11;
    CHECK(sound_Play(&position, 2, sound, 7) == 11);
    CHECK(trace.calls == 1);
    CHECK(trace.banks[0] == 2);
    CHECK(trace.arguments_match == 1);

    reset_trace(&trace, &position, sound, 7);
    trace.results[1] = 12;
    CHECK(sound_Play(&position, 2, sound, 7) == 12);
    CHECK(trace.calls == 2);
    CHECK(trace.banks[0] == 2);
    CHECK(trace.banks[1] == 3);

    reset_trace(&trace, &position, sound, 7);
    CHECK(sound_Play(&position, 0, sound, 7) == 0);
    CHECK(trace.calls == 2);
    CHECK(trace.banks[0] == 0);
    CHECK(trace.banks[1] == 3);

    reset_trace(&trace, &position, sound, 7);
    trace.results[1] = 13;
    CHECK(sound_Play(&position, 3, sound, 7) == 13);
    CHECK(trace.calls == 2);
    CHECK(trace.banks[0] == 3);
    CHECK(trace.banks[1] == 0);

    reset_trace(&trace, &position, sound, 7);
    trace.results[3] = 14;
    CHECK(sound_Play(&position, 2, sound, 7) == 14);
    CHECK(trace.calls == 4);
    CHECK(trace.banks[0] == 2);
    CHECK(trace.banks[1] == 3);
    CHECK(trace.banks[2] == 0);
    CHECK(trace.banks[3] == 0);

    reset_trace(&trace, &position, sound, 7);
    trace.results[0] = 15;
    CHECK(sound_PlayController(&position, 2, sound, 7) == 15);
    CHECK(trace.calls == 1);
    CHECK(trace.arguments_match == 1);

    jpb_SoundSetPlaySfxHook(NULL, NULL);
    CHECK(sound_Play(&position, 3, sound, 7) == 0);
    return 0;
}

static int test_sound_position_wrappers(void)
{
    FVECTOR float_position = {1.9f, -2.9f, NAN};
    _svector short_position = {1, 2, 3, 0};
    char float_sound[] = "splash";
    char short_sound[] = "probmove";
    SoundTrace trace;

    reset_trace(&trace, NULL, float_sound, 4);
    trace.compare_position_by_value = 1;
    trace.expected_position_value.vx = 1;
    trace.expected_position_value.vy = -2;
    trace.expected_position_value.vz = INT32_MIN;
    trace.expected_position_value.pad = 0;
    trace.results[0] = 15;
    CHECK(sound_PlayFV(&float_position, 3, float_sound, 4) == 15);
    CHECK(trace.calls == 1);
    CHECK(trace.banks[0] == 3);
    CHECK(trace.arguments_match == 1);

    reset_trace(
        &trace, (VECTOR *)&short_position, short_sound, 8);
    trace.results[0] = 16;
    CHECK(sound_PlaySV(&short_position, 3, short_sound, 8) == 0);
    CHECK(trace.calls == 1);
    CHECK(trace.arguments_match == 1);
    return 0;
}

static int test_volume_and_retail_stubs(void)
{
    VECTOR listener = {0, 0, 0, 0};
    VECTOR sound = {0, 0, 0, 0};
    uint8_t distance = 255;
    int left = 0;
    int right = 0;
    int bank = 99;
    char name[] = "anything";

    OptionStruct.Stereo = 0;
    get_sound_volume(listener, sound, &distance, &left, &right);
    CHECK(distance == 0);
    CHECK(left == 255);
    CHECK(right == 255);

    sound.vx = 2560;
    get_sound_volume(listener, sound, &distance, &left, &right);
    CHECK(distance == 171);
    OptionStruct.Stereo = 1;
    cameraFacing.vx = 0;
    cameraFacing.vy = 0;
    cameraFacing.vz = 4096;
    cameraFacing.pad = 0;
    sound = listener;
    get_sound_volume(listener, sound, &distance, &left, &right);
    CHECK(left == 127);
    CHECK(right == 127);

    CHECK(sound_GetIndex(&bank, name) == -1);
    CHECK(sound_GetSoundIndex(&bank, name) == 0);
    CHECK(sound_IsPlaying(7) == 0);
    CHECK(sound_Resume() == 0);
    CHECK(sound_UnLoadBank(NULL) == 0);
    sound_SetFrequency(7, 44100);
    sound_SetPosition(7, &sound);
    sound_UpdateAll(16);
    setCDXAvol(100, 100);
    return 0;
}

static int test_sound_control_and_loop_lifecycle(void)
{
    SoundControlTrace control = {0};
    SoundTrace play;
    VECTOR position = {10, 20, 30, 0};
    char loop_name[] = "fan_big";
    char ordinary_name[] = "splash";

    sound_Init();
    jpb_SoundSetStopHook(trace_stop, &control);
    jpb_SoundSetFadeHook(trace_fade, &control);
    jpb_SoundSetControlHook(trace_control, &control);

    sound_SetLoopingFadeTime(UINT16_C(19), UINT32_C(750));
    CHECK(control.fade_calls == 1);
    CHECK(control.fade_handle == 19);
    CHECK(control.fade_time == 750);

    reset_trace(&play, &position, loop_name, 0);
    play.results[0] = 37;
    CHECK(sound_playSfx(&position, 3, loop_name, 0) == 37);
    reset_trace(&play, &position, ordinary_name, 0);
    play.results[0] = 38;
    CHECK(sound_playSfx(&position, 3, ordinary_name, 0) == 38);
    stop_all_looped_sounds();
    CHECK(control.stop_calls == 1);
    CHECK(control.stop_handle == 37);

    sound_Init();
    reset_trace(&play, &position, loop_name, 0);
    play.results[0] = 41;
    CHECK(sound_playSfx(&position, 3, loop_name, 0) == 41);
    sound_StopSound(41);
    CHECK(control.stop_calls == 2);
    stop_all_looped_sounds();
    CHECK(control.stop_calls == 2);

    sound_Pause();
    sound_StopAll();
    mute_looped_sounds();
    CHECK(loopedSoundMuted == 1);
    unmute_looped_sounds();
    CHECK(loopedSoundMuted == 0);
    CHECK(control.control_calls == 6);
    CHECK(control.controls[0] == JPB_SOUND_CONTROL_PAUSE_MUSIC);
    CHECK(control.controls[1] == JPB_SOUND_CONTROL_HALT_MUSIC);
    CHECK(control.controls[2] == JPB_SOUND_CONTROL_UPDATE_LOOPED);
    CHECK(control.controls[3] == JPB_SOUND_CONTROL_MUTE_LOOPED);
    CHECK(control.controls[4] == JPB_SOUND_CONTROL_UPDATE_LOOPED);
    CHECK(control.controls[5] == JPB_SOUND_CONTROL_UNMUTE_LOOPED);

    jpb_SoundSetStopHook(NULL, NULL);
    jpb_SoundSetFadeHook(NULL, NULL);
    jpb_SoundSetControlHook(NULL, NULL);
    return 0;
}

static int test_paused_playback_gate(void)
{
    SoundTrace trace;
    VECTOR position = {0, 0, 0, 0};
    char sound[] = "splash";

    sound_Paused = 1;
    reset_trace(&trace, &position, sound, 0);
    trace.results[0] = 50;
    CHECK(sound_playSfx(&position, 3, sound, 0) == 0);
    CHECK(trace.calls == 0);

    reset_trace(&trace, &position, sound, 8);
    trace.results[0] = 51;
    CHECK(sound_playSfx(&position, 3, sound, 8) == 51);
    CHECK(trace.calls == 1);
    sound_Paused = 0;
    jpb_SoundSetPlaySfxHook(NULL, NULL);
    return 0;
}

int main(void)
{
    CHECK(test_helpers_and_default_options() == 0);
    CHECK(test_bank_lifecycle() == 0);
    CHECK(test_sound_play_fallback_order() == 0);
    CHECK(test_sound_position_wrappers() == 0);
    CHECK(test_volume_and_retail_stubs() == 0);
    CHECK(test_sound_control_and_loop_lifecycle() == 0);
    CHECK(test_paused_playback_gate() == 0);
    puts("sound tests passed");
    return 0;
}
