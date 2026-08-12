#include "jpb/audio_stream.h"

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

typedef struct AudioStreamTrace {
    int playCalls;
    int streamIndex;
    const char *streamName;
    int volume;
    int loop;
    int controlCalls;
    JPBAudioStreamControl controls[8];
    int values[8];
} AudioStreamTrace;

static void trace_play(
    int stream_index,
    const char *stream_name,
    int volume,
    int loop,
    void *user_data)
{
    AudioStreamTrace *trace = (AudioStreamTrace *)user_data;

    ++trace->playCalls;
    trace->streamIndex = stream_index;
    trace->streamName = stream_name;
    trace->volume = volume;
    trace->loop = loop;
}

static int trace_control(
    JPBAudioStreamControl control,
    int value,
    void *user_data)
{
    AudioStreamTrace *trace = (AudioStreamTrace *)user_data;
    int call = trace->controlCalls++;

    trace->controls[call] = control;
    trace->values[call] = value;
    return 1;
}

static int test_stream_table(void)
{
    AudioStreamTrace trace = {0};

    jpb_AudioStreamSetPlayHook(trace_play, &trace);
    playXA(6, 91, 1);
    CHECK(trace.playCalls == 1);
    CHECK(trace.streamIndex == 6);
    CHECK(strcmp(trace.streamName, "01_FedAmbient1.wav") == 0);
    CHECK(trace.volume == 91);
    CHECK(trace.loop == 1);

    playXA(30, 1, 0);
    playXA(39, 1, 0);
    playXA(102, 1, 0);
    playXA(-1, 1, 0);
    playXA(103, 1, 0);
    CHECK(trace.playCalls == 1);

    playXA(94, 64, 0);
    CHECK(trace.playCalls == 2);
    CHECK(trace.streamIndex == 94);
    CHECK(strcmp(trace.streamName, "QueenArrested.wav") == 0);
    CHECK(trace.volume == 64);
    CHECK(trace.loop == 0);
    jpb_AudioStreamSetPlayHook(NULL, NULL);
    return 0;
}

static int test_stream_controls(void)
{
    AudioStreamTrace trace = {0};

    CHECK(kmAudioStream_StartUp() == 1);
    jpb_AudioStreamSetControlHook(trace_control, &trace);
    CHECK(kmAudioStream_StartUp() == 1);
    pauseXA();
    unpauseXA();
    stopXA();
    setMusicVol(73);
    setChannelType(2);
    kmAudioStream_ShutDown();
    updateXA();
    CHECK(trace.controlCalls == 7);
    CHECK(trace.controls[0] == JPB_AUDIO_STREAM_START_UP);
    CHECK(trace.controls[1] == JPB_AUDIO_STREAM_PAUSE);
    CHECK(trace.controls[2] == JPB_AUDIO_STREAM_RESUME);
    CHECK(trace.controls[3] == JPB_AUDIO_STREAM_STOP);
    CHECK(trace.controls[4] == JPB_AUDIO_STREAM_SET_VOLUME);
    CHECK(trace.values[4] == 73);
    CHECK(trace.controls[5] == JPB_AUDIO_STREAM_SET_CHANNEL_TYPE);
    CHECK(trace.values[5] == 2);
    CHECK(trace.controls[6] == JPB_AUDIO_STREAM_SHUT_DOWN);
    jpb_AudioStreamSetControlHook(NULL, NULL);
    return 0;
}

int main(void)
{
    CHECK(test_stream_table() == 0);
    CHECK(test_stream_controls() == 0);
    puts("audio stream tests passed");
    return 0;
}
