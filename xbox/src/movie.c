#include "movie.h"
#include "audio.h"
#include "gpu.h"
#include "presentation.h"
#include "jpb/theoraplay.h"
#include "jpb/input.h"
#include "jpb/game.h"
#include "jpb/menu.h"
#include <hal/video.h>
#include <windows.h>
#include <pbkit/pbkit.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

extern void jpb_XboxControllerPoll(JPBControllerState *state);
extern volatile unsigned jpb_XboxSmokeFrame;

volatile unsigned jpb_XboxMovieState;
volatile unsigned jpb_XboxMovieIndex;
volatile unsigned jpb_XboxMovieFramesDecoded;
volatile unsigned jpb_XboxMovieFramesPresented;
volatile unsigned jpb_XboxMovieAudioFrames;
volatile unsigned jpb_XboxMovieWidth;
volatile unsigned jpb_XboxMovieHeight;
volatile unsigned jpb_XboxMovieDecodeErrors;
volatile unsigned jpb_XboxMovieResults[10][4];
volatile unsigned jpb_XboxMovieVideoMs;
volatile unsigned jpb_XboxMovieAudioMs;

static const char *movie_name(unsigned movie)
{
    static const char *const names[]={
        "1080\\flipped\\IntroFlippedVertical_converted.ogg",
        "1080\\flipped\\English1920Vertical_converted.ogg",
        "1080\\flipped\\HorizontalFlippedQui_converted.ogg",
        "1080\\flipped\\HorizontalFlippedObi_converted.ogg",
        "1080\\flipped\\HorizontalFlippedMace_converted.ogg",
        "1080\\flipped\\HorizontalFlippedPlo_converted.ogg",
        "1080\\flipped\\HorizontalFlippedAdi_converted.ogg",
        "1080\\flipped\\End1080Flipped_converted.ogg",
        "1080\\flipped\\Aspyr_Logo_1080_Flipped.ogg",
        "1080\\flipped\\photo_warning_English_1080_Flipped.ogg"
    };
    return movie<sizeof(names)/sizeof(names[0])?names[movie]:NULL;
}

/* IYUV (theoraplay planar: Y, then U and V at half size) to ARGB with the
   colour formula of theoraplay's RGBA converter, in 16.16 fixed point. Like
   that converter, rows come out bottom-up: staged movies are stored
   vertically flipped. */
static uint8_t movie_clamp(int value)
{
    value=(value+32768)>>16;
    return (uint8_t)(value<0?0:value>255?255:value);
}

static void convert_iyuv(const THEORAPLAY_VideoFrame *video,uint32_t *out,
    int stride_pixels)
{
    const unsigned width=video->width,height=video->height;
    const uint8_t *luma=video->pixels;
    const uint8_t *cb_plane=luma+(size_t)width*height;
    const uint8_t *cr_plane=cb_plane+(size_t)(width/2u)*(height/2u);
    for(unsigned row=0;row<height;++row){
        const uint8_t *y_row=luma+(size_t)row*width;
        const uint8_t *cb_row=cb_plane+(size_t)(row>>1)*(width/2u);
        const uint8_t *cr_row=cr_plane+(size_t)(row>>1)*(width/2u);
        uint32_t *destination=out+(size_t)(height-1u-row)*stride_pixels;
        for(unsigned column=0;column<width;++column){
            int y=((int)y_row[column]-16)*76309;
            int cb=(int)cb_row[column>>1]-128;
            int cr=(int)cr_row[column>>1]-128;
            destination[column]=0xff000000u|
                ((uint32_t)movie_clamp(y+cr*104597)<<16)|
                ((uint32_t)movie_clamp(y-cb*25674-cr*53279)<<8)|
                movie_clamp(y+cb*132201);
        }
    }
}

static void blit_video(
    JPBSoftwareFramebuffer *framebuffer,
    const THEORAPLAY_VideoFrame *video)
{
    memset(framebuffer->pixels,0,
        (size_t)framebuffer->stridePixels*framebuffer->height*4u);
    if(!video || !video->pixels || !video->width || !video->height)return;
    int width=framebuffer->width;
    int height=(int)((int64_t)width*video->height/video->width);
    if(height>framebuffer->height){
        height=framebuffer->height;
        width=(int)((int64_t)height*video->width/video->height);
    }
    int left=(framebuffer->width-width)/2;
    int top=(framebuffer->height-height)/2;
    const unsigned w=video->width,h=video->height;
    const uint8_t *cb_plane=video->pixels+(size_t)w*h;
    const uint8_t *cr_plane=cb_plane+(size_t)(w/2u)*(h/2u);
    for(int y=0;y<height;++y){
        /* Flipped source, as in convert_iyuv. */
        unsigned sy=h-1u-(unsigned)((int64_t)y*h/height);
        uint32_t *destination=framebuffer->pixels+
            (size_t)(top+y)*framebuffer->stridePixels+left;
        for(int x=0;x<width;++x){
            unsigned sx=(unsigned)((int64_t)x*w/width);
            int luma=((int)video->pixels[(size_t)sy*w+sx]-16)*76309;
            int cb=(int)cb_plane[(size_t)(sy>>1)*(w/2u)+(sx>>1)]-128;
            int cr=(int)cr_plane[(size_t)(sy>>1)*(w/2u)+(sx>>1)]-128;
            destination[x]=UINT32_C(0xff000000)|
                ((uint32_t)movie_clamp(luma+cr*104597)<<16)|
                ((uint32_t)movie_clamp(luma-cb*25674-cr*53279)<<8)|
                movie_clamp(luma+cb*132201);
        }
    }
}

static void present_frame(
    JPBSoftwareFramebuffer *framebuffer,int gpu_initialized)
{
    if(gpu_initialized){
        jpb_XboxGpuBegin();
        if(jpb_XboxGpuPresentFrontend(framebuffer))jpb_XboxGpuPresent();
        return;
    }
    jpb_XboxPresentSoftware(framebuffer->pixels,framebuffer->width,
        framebuffer->height,framebuffer->stridePixels);
}

static int any_button_pressed(void)
{
    JPBControllerState state;
    jpb_XboxControllerPoll(&state);
    for(unsigned i=0;i<sizeof(state.button);++i)if(state.button[i])return 1;
    return 0;
}

typedef struct MovieAudioFeed {
    THEORAPLAY_Decoder *decoder;
    volatile int stop,active,started,done,failed;
} MovieAudioFeed;

static DWORD WINAPI feed_movie_audio(void *opaque)
{
    MovieAudioFeed *feed=(MovieAudioFeed *)opaque;
    const THEORAPLAY_AudioPacket *packet=NULL;
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
    while(!feed->stop){
        if(!packet)packet=THEORAPLAY_getAudio(feed->decoder);
        if(packet){
            if(!feed->active){
                /* Movie soundtracks peak well above full scale; the game
                   plays them at music volume times VideoVolume. */
                float gain=(float)OptionStruct.musicVolume*0.0078125f*VideoVolume;
                if(!jpb_XboxAudioMovieBegin(packet->freq,packet->channels,gain)){
                    feed->failed=1;break;
                }
                feed->active=1;
            }
            if(jpb_XboxAudioMovieQueue(packet->samples,packet->frames,packet->channels)){
                jpb_XboxMovieAudioFrames+=packet->frames;
                /* Never enter nxdk's heap spinlock above the priority of
                 * an allocating thread that might already own it. */
                SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);
                THEORAPLAY_freeAudio(packet);packet=NULL;
                SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_ABOVE_NORMAL);
            }
        }
        if(feed->active&&!feed->started &&
           (jpb_XboxAudioMovieBufferedFrames()>=12288u ||
            !THEORAPLAY_isDecoding(feed->decoder))){
            jpb_XboxAudioMovieStart();feed->started=1;
        }
        if(!THEORAPLAY_isDecoding(feed->decoder)&&!packet&&
           !THEORAPLAY_availableAudio(feed->decoder)&&
           !jpb_XboxAudioMovieBufferedFrames())break;
        Sleep(1);
    }
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_NORMAL);
    if(packet)THEORAPLAY_freeAudio(packet);
    feed->done=1;
    return 0;
}

int jpb_XboxMoviePlay(
    unsigned movie,
    JPBSoftwareFramebuffer *framebuffer,
    int gpu_initialized,
    int ignore_input)
{
    char path[256];
    const char *name=movie_name(movie);
    THEORAPLAY_Decoder *decoder=NULL;
    const THEORAPLAY_VideoFrame *current=NULL,*next=NULL;
    MovieAudioFeed feed={0};
    HANDLE feeder=NULL;
    DWORD wait_started,play_started=0;
    int has_audio=0;
    int success=0,input_armed=0;
    unsigned last_presented_ms=~0u;

    jpb_XboxMovieIndex=movie;
    jpb_XboxMovieFramesDecoded=0;
    jpb_XboxMovieFramesPresented=0;
    jpb_XboxMovieAudioFrames=0;
    jpb_XboxMovieWidth=jpb_XboxMovieHeight=0;
    jpb_XboxMovieState=1;
    if(!name || !framebuffer || !framebuffer->pixels)goto done;
    snprintf(path,sizeof(path),"D:\\res\\movies\\%s",name);
    /* Frames are decoded as planar IYUV and converted while uploading. An
       RGBA frame at 512x288 is a 590 KB allocation; with the menus resident
       only 2-4 MB stay free at boot, and queueing eight ran the intro film
       out of memory 1-12 s in. Fewer queued frames deadlock instead: the
       first audio sits behind them and the audio clock never starts. */
    decoder=THEORAPLAY_startDecodeFile(path,8,THEORAPLAY_VIDFMT_IYUV);
    if(!decoder)goto done;
    jpb_XboxMovieState=2;
    wait_started=GetTickCount();
    while(!THEORAPLAY_isInitialized(decoder)&&
          THEORAPLAY_isDecoding(decoder)&&
          GetTickCount()-wait_started<10000u)Sleep(2);
    if(!THEORAPLAY_isInitialized(decoder)||
       !THEORAPLAY_hasVideoStream(decoder))goto done;

    wait_started=GetTickCount();
    while(!next&&THEORAPLAY_isDecoding(decoder)&&
          GetTickCount()-wait_started<10000u){
        next=THEORAPLAY_getVideo(decoder);
        if(!next)Sleep(2);
    }
    if(!next)goto done;
    ++jpb_XboxMovieFramesDecoded;
    jpb_XboxMovieWidth=next->width;
    jpb_XboxMovieHeight=next->height;

    has_audio=THEORAPLAY_hasAudioStream(decoder);
    if(has_audio){
        feed.decoder=decoder;
        feeder=CreateThread(NULL,0,feed_movie_audio,&feed,0,NULL);
        if(!feeder)goto done;

    }

    current=next;next=NULL;
    play_started=GetTickCount();
    jpb_XboxMovieState=3;
    while(THEORAPLAY_isDecoding(decoder)||current||next||
          (has_audio&&!feed.done)){
        DWORD elapsed=GetTickCount()-play_started;
        if(!ignore_input){
            int pressed=any_button_pressed();
            if(!pressed)input_armed=1;
            else if(input_armed)break;
        }
        if(feed.failed)goto done;
        if(has_audio)
            elapsed=feed.started ? jpb_XboxAudioMoviePlayedFrames()/48u : 0;
        if(!next){
            next=THEORAPLAY_getVideo(decoder);
            if(next)++jpb_XboxMovieFramesDecoded;
        }
        while(next&&next->playms<=elapsed){
            if(current)THEORAPLAY_freeVideo(current);
            current=next;next=THEORAPLAY_getVideo(decoder);
            if(next)++jpb_XboxMovieFramesDecoded;
        }
        jpb_XboxMovieAudioMs=feed.started ? jpb_XboxAudioMoviePlayedFrames()/48u : 0;
        if(current && current->playms!=last_presented_ms){
            jpb_XboxMovieVideoMs=current->playms;
            if(gpu_initialized && current->width<=framebuffer->width &&
               current->height<=framebuffer->height){
                /* Upload at decode resolution; the GPU does the scaling.
                   Reusing the menu buffer avoids another allocation. */
                JPBSoftwareFramebuffer native={framebuffer->pixels,
                    (int)current->width,(int)current->height,(int)current->width};
                convert_iyuv(current,native.pixels,(int)current->width);
                present_frame(&native,1);
            }else{
                blit_video(framebuffer,current);
                present_frame(framebuffer,gpu_initialized);
            }
            last_presented_ms=current->playms;
            ++jpb_XboxMovieFramesPresented;
            jpb_XboxSmokeFrame=jpb_XboxMovieFramesPresented;
        }
        if(!THEORAPLAY_isDecoding(decoder)&&!next&&
           (!has_audio || feed.done))break;
        Sleep(1);
    }
    success=!THEORAPLAY_decodingError(decoder);

done:
    if(feeder){
        feed.stop=1;
        WaitForSingleObject(feeder,INFINITE);
        CloseHandle(feeder);
    }
    if(current)THEORAPLAY_freeVideo(current);
    if(next)THEORAPLAY_freeVideo(next);
    if(feed.active)jpb_XboxAudioMovieEnd();
    if(decoder){
        if(THEORAPLAY_decodingError(decoder))++jpb_XboxMovieDecodeErrors;
        THEORAPLAY_stopDecode(decoder);
    }
    jpb_XboxMovieState=success?4:0x80000000u;
    if(movie<10){
        jpb_XboxMovieResults[movie][0]=jpb_XboxMovieState;
        jpb_XboxMovieResults[movie][1]=jpb_XboxMovieFramesDecoded;
        jpb_XboxMovieResults[movie][2]=jpb_XboxMovieAudioFrames;
        jpb_XboxMovieResults[movie][3]=jpb_XboxMovieVideoMs;
    }
    return success;
}
