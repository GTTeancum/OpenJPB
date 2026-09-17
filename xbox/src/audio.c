/* nxdk SDL/AC97 adapter. The shared scheduler owns banks, spatial gain,
   looping names and gameplay triggers. No XDK APIs or host audio capture. */
#include "jpb/sound.h"
#include "jpb/audio_stream.h"
#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pbkit/pbkit.h>

extern FILE *jpb_XboxFopen(const char *,const char *);
enum { RATE=48000, VOICES=64, CACHE_BYTES=1280*1024,
       MUSIC_RING_FRAMES=16384, MUSIC_READ_FRAMES=4096 };
typedef struct Sample {
    struct Sample *next;
    char path[512];
    int16_t *pcm;
    unsigned frames,bytes,used;
} Sample;
typedef struct Voice {
    Sample *sample;
    unsigned frame,fade,total_fade;
    int loops,left,right,distance,volume,paused;
} Voice;
static Sample *samples;
static Voice voices[VOICES];
static int16_t sample_cache[CACHE_BYTES/sizeof(int16_t)];
static SDL_AudioDeviceID device;
static unsigned cached,clock_value,plays,failures;
static unsigned decode_stage;
static unsigned decode_errno;
enum { SFX_FAILURE_RECORDS=32, SFX_FAILURE_PATH_BYTES=192 };
char jpb_XboxSfxFailurePaths[SFX_FAILURE_RECORDS][SFX_FAILURE_PATH_BYTES];
volatile unsigned jpb_XboxSfxFailureReasons[SFX_FAILURE_RECORDS];
volatile unsigned jpb_XboxSfxFailureErrnos[SFX_FAILURE_RECORDS];
volatile unsigned jpb_XboxSfxFailureFreePages[SFX_FAILURE_RECORDS];
volatile unsigned jpb_XboxSfxFailureTinyAlloc[SFX_FAILURE_RECORDS];
volatile unsigned jpb_XboxSfxFailurePathCount;
/* Bounded read-only trace for matching in-game requests to native AC97 output. */
enum { SFX_PLAY_RECORDS=64, SFX_PLAY_PATH_BYTES=96 };
char jpb_XboxSfxPlayedPaths[SFX_PLAY_RECORDS][SFX_PLAY_PATH_BYTES];
volatile unsigned jpb_XboxSfxPlayedFrames[SFX_PLAY_RECORDS];
volatile unsigned jpb_XboxSfxPlayedCount;
extern volatile unsigned jpb_XboxSmokeFrame;
static volatile unsigned callbacks;
static volatile unsigned mixed_frames,mixed_peak;
static SDL_Thread *ac97_service_thread;
#ifdef JPB_XBOX_CAPTURE_MIX
enum { MIX_CAPTURE_FRAMES=3*RATE };
int16_t jpb_XboxMixCapture[MIX_CAPTURE_FRAMES*2];
volatile unsigned jpb_XboxMixCaptureFrames;
#endif
typedef struct Music {
    FILE *file;                  /* Main thread only. The callback reads ring. */
    int16_t ring[MUSIC_RING_FRAMES*2];
    unsigned read_frame,write_frame;
    unsigned rate,data_start,data_bytes,remaining;
    unsigned requests,started,failures,underruns,completed;
    float phase;
    int volume,active,paused,loop,eof;
} Music;
static Music music={.volume=128};

static int music_sample(int channel)
{
    if(!music.active || music.paused)return 0;
    unsigned available=music.write_frame-music.read_frame;
    if(available<2) {
        if(music.eof && available==1) {
            int value=music.ring[(music.read_frame&(MUSIC_RING_FRAMES-1))*2+channel];
            if(channel==1) {++music.read_frame;music.active=0;++music.completed;}
            return value*music.volume/128;
        }
        if(music.eof && !available) {music.active=0;++music.completed;}
        else if(!music.eof)++music.underruns;
        return 0;
    }
    unsigned at=(music.read_frame&(MUSIC_RING_FRAMES-1))*2+channel;
    unsigned next=((music.read_frame+1)&(MUSIC_RING_FRAMES-1))*2+channel;
    int a=music.ring[at],b=music.ring[next];
    int value=(int)(a+(b-a)*music.phase);
    if(channel==1) {
        music.phase+=(float)music.rate/RATE;
        if(music.phase>=1) {music.phase-=1;++music.read_frame;}
    }
    return value*music.volume/128;
}

static void mix(void *unused,Uint8 *output,int bytes)
{
    (void)unused;
    int16_t *out=(int16_t *)output;
    for(int f=0;f<bytes/4;++f) {
        int left=music_sample(0),right=music_sample(1);
        for(unsigned i=0;i<VOICES;++i) {
            Voice *v=&voices[i];
            if(!v->sample || v->paused)continue;
            if(v->frame>=v->sample->frames) {
                if(!v->loops) {v->sample=NULL;continue;}
                if(v->loops>0)--v->loops;
                v->frame=0;
            }
            int gain=v->volume*(255-v->distance)/255;
            if(v->total_fade) {
                if(!v->fade) {v->sample=NULL;continue;}
                gain=(int)((uint64_t)gain*v->fade/v->total_fade);
                --v->fade;
            }
            left+=v->sample->pcm[v->frame*2]*gain/128*v->left/255;
            right+=v->sample->pcm[v->frame*2+1]*gain/128*v->right/255;
            ++v->frame;
        }
        out[f*2]=(int16_t)(left>32767?32767:left< -32768?-32768:left);
        out[f*2+1]=(int16_t)(right>32767?32767:right< -32768?-32768:right);
        int peak=left<0?-left:left;
        if((right<0?-right:right)>peak)peak=right<0?-right:right;
        if(peak)++mixed_frames;
        if((unsigned)peak>mixed_peak)mixed_peak=(unsigned)peak;
    }
#ifdef JPB_XBOX_CAPTURE_MIX
    if(music.started && jpb_XboxMixCaptureFrames<MIX_CAPTURE_FRAMES) {
        unsigned frames=(unsigned)bytes/4;
        unsigned room=MIX_CAPTURE_FRAMES-jpb_XboxMixCaptureFrames;
        if(frames>room)frames=room;
        memcpy(jpb_XboxMixCapture+jpb_XboxMixCaptureFrames*2,out,frames*4);
        jpb_XboxMixCaptureFrames+=frames;
    }
#endif
    ++callbacks;
}

static unsigned le16(FILE *file)
{
    unsigned char b[2];
    return fread(b,1,2,file)==2?(unsigned)b[0]|((unsigned)b[1]<<8):UINT32_MAX;
}
static unsigned le32(FILE *file)
{
    unsigned char b[4];
    return fread(b,1,4,file)==4?(unsigned)b[0]|((unsigned)b[1]<<8)|
        ((unsigned)b[2]<<16)|((unsigned)b[3]<<24):UINT32_MAX;
}

static int music_wave_header(FILE *file,unsigned *rate,unsigned *start,unsigned *bytes)
{
    char tag[4];
    if(fread(tag,1,4,file)!=4 || memcmp(tag,"RIFF",4) || le32(file)==UINT32_MAX ||
       fread(tag,1,4,file)!=4 || memcmp(tag,"WAVE",4))return 0;
    int format_found=0,data_found=0;
    for(unsigned chunk=0;chunk<256 && (!format_found || !data_found);++chunk) {
        if(fread(tag,1,4,file)!=4)return 0;
        unsigned size=le32(file);
        if(size==UINT32_MAX || size>INT32_MAX)return 0;
        long position=ftell(file);
        if(position<0)return 0;
        if(!memcmp(tag,"fmt ",4)) {
            if(size<16 || le16(file)!=1 || le16(file)!=2)return 0;
            *rate=le32(file);
            if(le32(file)!=*rate*4 || le16(file)!=4 || le16(file)!=16 ||
               *rate<8000 || *rate>RATE)return 0;
            format_found=1;
        } else if(!memcmp(tag,"data",4)) {
            *start=(unsigned)position;*bytes=size;
            data_found=1;
        }
        if(fseek(file,position+(long)size+(size&1),SEEK_SET))return 0;
    }
    if(!format_found || !data_found || !*bytes || (*bytes&3))return 0;
    if(fseek(file,0,SEEK_END))return 0;
    long end=ftell(file);
    return end>=0 && (uint64_t)*start+*bytes<=(uint64_t)end &&
        !fseek(file,*start,SEEK_SET);
}

static void music_stop(void)
{
    SDL_LockAudioDevice(device);
    FILE *file=music.file;
    music.file=NULL;music.active=0;music.paused=0;music.eof=0;
    music.read_frame=music.write_frame=0;music.phase=0;
    SDL_UnlockAudioDevice(device);
    if(file)fclose(file);
}

static void music_play(int index,const char *name,int volume,int loop,void *unused)
{
    (void)index;(void)volume;(void)unused; /* Retail playXA ignores volume. */
    ++music.requests;
    music_stop();
    if(!name || strchr(name,'/') || strchr(name,'\\') || strlen(name)>200) {
        ++music.failures;return;
    }
    char path[256];
    snprintf(path,sizeof(path),"D:\\res\\sound\\streams\\%s",name);
    FILE *file=jpb_XboxFopen(path,"rb");
    unsigned rate=0,start=0,bytes=0;
    if(!file || !music_wave_header(file,&rate,&start,&bytes)) {
        if(file)fclose(file);
        ++music.failures;return;
    }
    SDL_LockAudioDevice(device);
    music.file=file;music.rate=rate;music.data_start=start;
    music.data_bytes=music.remaining=bytes;
    music.loop=loop!=0;music.eof=0;music.active=1;
    ++music.started;
    SDL_UnlockAudioDevice(device);
}

static int music_control(JPBAudioStreamControl control,int value,void *unused)
{
    (void)unused;
    switch(control) {
    case JPB_AUDIO_STREAM_START_UP:return 1;
    case JPB_AUDIO_STREAM_SHUT_DOWN:case JPB_AUDIO_STREAM_STOP:
        music_stop();return 1;
    case JPB_AUDIO_STREAM_PAUSE:case JPB_AUDIO_STREAM_RESUME:
        SDL_LockAudioDevice(device);
        music.paused=control==JPB_AUDIO_STREAM_PAUSE;
        SDL_UnlockAudioDevice(device);
        return 1;
    case JPB_AUDIO_STREAM_SET_VOLUME:
        SDL_LockAudioDevice(device);
        music.volume=value<0?0:value>128?128:value;
        SDL_UnlockAudioDevice(device);
        return 1;
    case JPB_AUDIO_STREAM_SET_CHANNEL_TYPE:return 1;
    }
    return 0;
}

void jpb_XboxAudioPump(void)
{
    static int16_t buffer[MUSIC_READ_FRAMES*2];
    if(!music.file)return;
    if(!music.active) {music_stop();return;}
    if(music.eof)return;
    SDL_LockAudioDevice(device);
    unsigned free_frames=MUSIC_RING_FRAMES-(music.write_frame-music.read_frame);
    SDL_UnlockAudioDevice(device);
    if(free_frames<MUSIC_READ_FRAMES)return;
    if(!music.remaining) {
        if(music.loop && !fseek(music.file,music.data_start,SEEK_SET))
            music.remaining=music.data_bytes;
        else music.eof=1;
        return;
    }
    unsigned frames=music.remaining/4;
    if(frames>MUSIC_READ_FRAMES)frames=MUSIC_READ_FRAMES;
    unsigned got=(unsigned)fread(buffer,4,frames,music.file);
    if(!got) {music.remaining=0;return;}
    music.remaining-=got*4;
    if(got<frames)music.remaining=0;
    SDL_LockAudioDevice(device);
    for(unsigned i=0;i<got;++i) {
        unsigned at=((music.write_frame+i)&(MUSIC_RING_FRAMES-1))*2;
        music.ring[at]=buffer[i*2];music.ring[at+1]=buffer[i*2+1];
    }
    music.write_frame+=got;
    SDL_UnlockAudioDevice(device);
}

static int service_ac97(void *unused)
{
    extern void jpb_XboxAudioService(void);
    (void)unused;
    for (;;) {
        jpb_XboxAudioService();
        SDL_Delay(5);
    }
    return 0;
}

static void sound_control(JPBSoundControl control,void *unused)
{
    (void)unused;
    if(control==JPB_SOUND_CONTROL_PAUSE_MUSIC)
        music_control(JPB_AUDIO_STREAM_PAUSE,0,NULL);
    else if(control==JPB_SOUND_CONTROL_HALT_MUSIC)
        music_control(JPB_AUDIO_STREAM_STOP,0,NULL);
}

static void *load_chunk(const char *path,void *unused)
{
    (void)unused;
    if(!path || strlen(path)>=sizeof(((Sample *)0)->path))return NULL;
    Sample *s=calloc(1,sizeof(*s));
    if(!s)return NULL;
    strcpy(s->path,path);s->next=samples;samples=s;
    return s;
}

static int active(Sample *s)
{
    for(unsigned i=0;i<VOICES;++i)if(voices[i].sample==s)return 1;
    return 0;
}

static void discard_sample(Sample *s)
{
    if(!s->pcm)return;
    cached-=s->bytes;
    s->pcm=NULL;s->frames=s->bytes=0;
}

static int cache_region(unsigned bytes)
{
    unsigned start=0;
    while(start<=CACHE_BYTES-bytes) {
        unsigned next=CACHE_BYTES,next_end=CACHE_BYTES;
        int overlap=0;
        for(Sample *s=samples;s;s=s->next)if(s->pcm) {
            unsigned offset=(unsigned)((uint8_t *)s->pcm-(uint8_t *)sample_cache);
            if(offset<=start && offset+s->bytes>start) {
                start=offset+s->bytes;
                overlap=1;break;
            }
            if(offset>=start && offset<next) {
                next=offset;next_end=offset+s->bytes;
            }
        }
        if(overlap)continue;
        if(next-start>=bytes)return (int)start;
        start=next_end;
    }
    return -1;
}

static int decode(Sample *s)
{
    if(s->pcm)return 1;
    decode_stage=10;
    decode_errno=0;
    FILE *file=jpb_XboxFopen(s->path,"rb");
    if(!file) {decode_errno=errno;return 0;}
    decode_stage=12;
    unsigned rate=0,start=0,bytes=0;
    if(!music_wave_header(file,&rate,&start,&bytes) || rate!=RATE ||
       bytes>CACHE_BYTES) {fclose(file);return 0;}
    SDL_LockAudioDevice(device);
    int offset=cache_region(bytes);
    while(offset<0) {
        Sample *old=NULL;
        for(Sample *p=samples;p;p=p->next)
            if(p->pcm && !active(p) && (!old || p->used<old->used))old=p;
        if(!old)break;
        discard_sample(old);
        offset=cache_region(bytes);
    }
    if(offset>=0) {
        s->pcm=(int16_t *)((uint8_t *)sample_cache+offset);
        s->frames=bytes/4;s->bytes=bytes;cached+=bytes;
    }
    SDL_UnlockAudioDevice(device);
    decode_stage=15;
    if(offset<0) {fclose(file);return 0;}
    decode_stage=16;
    if(fread(s->pcm,1,bytes,file)!=bytes) {
        SDL_LockAudioDevice(device);
        discard_sample(s);
        SDL_UnlockAudioDevice(device);
        fclose(file);return 0;
    }
    fclose(file);
    return 1;
}

static void record_failure(const Sample *sample,unsigned reason)
{
    const char *path=sample?sample->path:"(no sample)";
    for(unsigned i=0;i<jpb_XboxSfxFailurePathCount;++i)
        if(!strcmp(jpb_XboxSfxFailurePaths[i],path))return;
    unsigned index=jpb_XboxSfxFailurePathCount;
    if(index>=SFX_FAILURE_RECORDS)return;
    snprintf(jpb_XboxSfxFailurePaths[index],SFX_FAILURE_PATH_BYTES,"%s",path);
    jpb_XboxSfxFailureReasons[index]=reason;
    jpb_XboxSfxFailureErrnos[index]=decode_errno;
    MM_STATISTICS memory={0};
    memory.Length=sizeof(memory);
    MmQueryStatistics(&memory);
    jpb_XboxSfxFailureFreePages[index]=memory.AvailablePages;
    void *probe=malloc(16);
    jpb_XboxSfxFailureTinyAlloc[index]=probe!=NULL;
    free(probe);
    jpb_XboxSfxFailurePathCount=index+1;
}

static uint16_t play(void *chunk,int loops,VECTOR *position,int bank,char *name,uint32_t flag,void *unused)
{
    (void)position;(void)bank;(void)name;(void)flag;(void)unused;
    Sample *s=chunk;
    if(!device || !s || !decode(s)) {
        record_failure(s,!device?3:decode_stage);
        ++failures;return UINT16_MAX;
    }
    SDL_LockAudioDevice(device);
    uint16_t result=UINT16_MAX;
    for(unsigned i=0;i<VOICES;++i)if(!voices[i].sample) {
        voices[i]=(Voice){.sample=s,.loops=loops,.left=255,.right=255,.volume=128};
        result=(uint16_t)i;s->used=++clock_value;++plays;
        {
            unsigned record=jpb_XboxSfxPlayedCount++%SFX_PLAY_RECORDS;
            snprintf(jpb_XboxSfxPlayedPaths[record],SFX_PLAY_PATH_BYTES,"%s",s->path);
            jpb_XboxSfxPlayedFrames[record]=jpb_XboxSmokeFrame;
        }
        break;
    }
    SDL_UnlockAudioDevice(device);
    if(result==UINT16_MAX) {record_failure(s,2);++failures;}
    return result;
}

static void stop(uint16_t handle,void *unused)
{
    (void)unused;
    SDL_LockAudioDevice(device);
    if(handle==UINT16_MAX)memset(voices,0,sizeof(voices));
    else if(handle<VOICES)voices[handle].sample=NULL;
    SDL_UnlockAudioDevice(device);
}
static void fade(uint16_t handle,uint32_t ms,void *unused)
{
    (void)unused;
    if(!ms) {stop(handle,NULL);return;}
    SDL_LockAudioDevice(device);
    for(unsigned i=0;i<VOICES;++i)if(i==handle || handle==UINT16_MAX) {
        voices[i].total_fade=voices[i].fade=ms>UINT32_MAX/48?UINT32_MAX:ms*48;
    }
    SDL_UnlockAudioDevice(device);
}
static void channel(JPBSoundChannelOperation op,int index,int a,int b,void *unused)
{
    (void)unused;
    SDL_LockAudioDevice(device);
    for(int i=0;i<VOICES;++i)if(i==index || index==-1) {
        Voice *v=&voices[i];
        switch(op) {
        case JPB_SOUND_CHANNEL_PANNING:v->left=a;v->right=b;break;
        case JPB_SOUND_CHANNEL_DISTANCE:v->distance=a;break;
        case JPB_SOUND_CHANNEL_VOLUME:v->volume=a<0?v->volume:a>128?128:a;break;
        case JPB_SOUND_CHANNEL_PAUSE:v->paused=1;break;
        case JPB_SOUND_CHANNEL_RESUME:v->paused=0;break;
        }
    }
    SDL_UnlockAudioDevice(device);
}
static void free_chunk(void *chunk,void *unused)
{
    (void)unused;
    Sample *s=chunk;if(!s)return;
    SDL_LockAudioDevice(device);
    for(unsigned i=0;i<VOICES;++i)if(voices[i].sample==s)voices[i].sample=NULL;
    Sample **p=&samples;while(*p && *p!=s)p=&(*p)->next;
    if(*p)*p=s->next;
    discard_sample(s);free(s);
    SDL_UnlockAudioDevice(device);
}
static int setup(JPBSoundSetupOperation op,int a,int b,int c,int d,void *unused)
{
    (void)a;(void)b;(void)c;(void)d;(void)unused;
    return device?(op==JPB_SOUND_SETUP_ALLOCATE_CHANNELS?VOICES:0):-1;
}
int jpb_XboxAudioInit(void)
{
    if(SDL_InitSubSystem(SDL_INIT_AUDIO)<0)return 0;
    SDL_AudioSpec desired={0},obtained;
    desired.freq=RATE;desired.format=AUDIO_S16LSB;desired.channels=2;desired.samples=1024;desired.callback=mix;
    device=SDL_OpenAudioDevice(NULL,0,&desired,&obtained,0);
    if(!device)return 0;
    ac97_service_thread=SDL_CreateThread(service_ac97,"OpenJPB AC97 service",NULL);
    if(!ac97_service_thread)return 0;
    jpb_SoundSetSetupHook(setup,NULL);
    jpb_SoundSetChunkHooks(load_chunk,free_chunk,NULL);
    jpb_SoundSetPlaySfxHook(play,NULL);
    jpb_SoundSetStopHook(stop,NULL);jpb_SoundSetFadeHook(fade,NULL);
    jpb_SoundSetChannelHook(channel,NULL);
    jpb_SoundSetControlHook(sound_control,NULL);
    jpb_AudioStreamSetPlayHook(music_play,NULL);
    jpb_AudioStreamSetControlHook(music_control,NULL);
    sound_Init();
    SDL_PauseAudioDevice(device,0);
    return 1;
}
void jpb_XboxAudioStats(void)
{
    extern void jpb_XboxAudioRecoveryStats(int *,int *);
    int halts=0,reclaimed=0;
    jpb_XboxAudioRecoveryStats(&halts,&reclaimed);
    pb_print("Audio callbacks %u plays %u failures %u cache %u KiB\n",callbacks,plays,failures,cached/1024);
    pb_print("AC97 halts %d buffers reclaimed %d\n",halts,reclaimed);
    pb_print("Music request %u start %u fail %u underrun %u done %u\n",
        music.requests,music.started,music.failures,music.underruns,music.completed);
    pb_print("Music active %d gain %d buffered %u read %u mix %u peak %u\n",
        music.active,music.volume,music.write_frame-music.read_frame,
        music.read_frame,mixed_frames,mixed_peak);
}
