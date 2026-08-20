#include "jpb/pc_audio_win32.h"
#include "jpb/sound_bank_data.h"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <stdio.h>
#include <string.h>

static int failures;

#define CHECK(condition)                                                   \
    do {                                                                   \
        if (!(condition)) {                                                \
            fprintf(stderr, "FAIL %s:%d: %s\n",                         \
                    __FILE__, __LINE__, #condition);                       \
            ++failures;                                                    \
        }                                                                  \
    } while (0)

static void test_pcm_wav_parser(void)
{
    static const unsigned char wav[] = {
        'R','I','F','F', 40,0,0,0, 'W','A','V','E',
        'f','m','t',' ', 16,0,0,0,
        1,0, 1,0, 0x44,0xac,0,0, 0x88,0x58,1,0,
        2,0, 16,0,
        'd','a','t','a', 4,0,0,0, 0,0,0,0
    };
    JPBPCAudioWavInfo info;

    CHECK(jpb_PCAudioInspectWavMemory(wav, sizeof(wav), &info));
    CHECK(info.formatTag == 1);
    CHECK(info.channels == 1);
    CHECK(info.sampleRate == 44100);
    CHECK(info.averageBytesPerSecond == 88200);
    CHECK(info.blockAlign == 2);
    CHECK(info.bitsPerSample == 16);
    CHECK(info.dataOffset == 44);
    CHECK(info.dataSize == 4);
    CHECK(!jpb_PCAudioInspectWavMemory(wav, sizeof(wav) - 1, &info));
}

static void test_bank_resolution(void)
{
    JPBPCAudio *audio = jpb_PCAudioCreate(
        "C:\\Game\\res\\level\\jpx\\fed\\fed.jpx",
        "C:\\Game\\res\\animation\\obi_wan.cad",
        NULL,
        1,
        0);
    char path[1024];

    CHECK(audio != NULL);
    CHECK(jpb_PCAudioResolveSound(
        audio, 0, "sabrsw01", path, sizeof(path)));
    CHECK(strcmp(
        path,
        "C:\\Game\\res\\sound\\sfx\\final\\resident\\sabrsw01.wav") == 0);
    CHECK(jpb_PCAudioResolveSound(
        audio, 1, "-!vjatk1", path, sizeof(path)));
    CHECK(strcmp(
        path,
        "C:\\Game\\res\\sound\\sfx\\final\\obi_wan\\vjatk1.wav") == 0);
    CHECK(jpb_PCAudioResolveSound(
        audio, 3, "dstroll1", path, sizeof(path)));
    CHECK(strcmp(
        path,
        "C:\\Game\\res\\sound\\sfx\\final\\fed\\dstroll1.wav") == 0);
    CHECK(!jpb_PCAudioResolveSound(
        audio, 2, "vjatk1", path, sizeof(path)));
    CHECK(!jpb_PCAudioResolveSound(
        audio, 4, "vjatk1", path, sizeof(path)));
    CHECK(jpb_PCAudioResolveStream(
        audio, "01_FedAmbient1.wav", path, sizeof(path)));
    CHECK(strcmp(
        path,
        "C:\\Game\\res\\sound\\streams\\01_FedAmbient1.wav") == 0);
    CHECK(!jpb_PCAudioResolveStream(
        audio, "..\\outside.wav", path, sizeof(path)));
    jpb_PCAudioDestroy(audio);
}

static void test_exact_alias_banks(void)
{
    JPBPCAudio *audio = jpb_PCAudioCreate(
        "C:\\Game\\res\\level\\jpx\\train1\\train1.jpx",
        "C:\\Game\\res\\animation\\jar_jar_playable.cad",
        NULL,
        16,
        0);
    char path[1024];

    CHECK(audio != NULL);
    CHECK(jpb_PCAudioResolveSound(
        audio, 1, "vggdie", path, sizeof(path)));
    CHECK(strstr(path, "\\gungan_2\\vggdie.wav") != NULL);
    CHECK(jpb_PCAudioResolveSound(
        audio, 3, "probmove", path, sizeof(path)));
    CHECK(strstr(path, "\\tato\\probmove.wav") != NULL);
    CHECK(jpb_PCAudioResolveSound(
        audio, 3, "vbdhit2", path, sizeof(path)));
    CHECK(strstr(path, "\\mini1\\vbdhit2.wav") != NULL);
    CHECK(!jpb_PCAudioResolveSound(
        audio, 3, "not_in_training_bank", path, sizeof(path)));
    jpb_PCAudioDestroy(audio);

    audio = jpb_PCAudioCreate(
        "C:\\Game\\res\\level\\jpx\\corus2\\corus2.jpx",
        NULL,
        NULL,
        15,
        0);
    CHECK(audio != NULL);
    CHECK(jpb_PCAudioResolveSound(
        audio, 3, "taxiloop", path, sizeof(path)));
    CHECK(strstr(path, "\\corus1\\taxiloop.wav") != NULL);
    jpb_PCAudioDestroy(audio);
}

static void test_two_player_banks(void)
{
    JPBPCAudio *audio = jpb_PCAudioCreate(
        "C:\\Game\\res\\level\\jpx\\fed\\fed.jpx",
        "C:\\Game\\res\\animation\\obi_wan.cad",
        "C:\\Game\\res\\animation\\qui_gon.cad",
        1,
        0);
    char path[1024];

    CHECK(audio != NULL);
    CHECK(jpb_PCAudioResolveSound(
        audio, 1, "vjatk1", path, sizeof(path)));
    CHECK(strstr(path, "\\obi_wan\\vjatk1.wav") != NULL);
    CHECK(jpb_PCAudioResolveSound(
        audio, 2, "vjdie", path, sizeof(path)));
    CHECK(strstr(path, "\\qui_gon\\vjdie.wav") != NULL);
    jpb_PCAudioDestroy(audio);
}

static void test_real_wav(int argc, char **argv)
{
    JPBPCAudioWavInfo info;

    if (argc != 2) {
        return;
    }
    CHECK(jpb_PCAudioInspectWavFile(argv[1], &info));
    CHECK(info.channels >= 1 && info.channels <= 2);
    CHECK(info.sampleRate > 0);
    CHECK(info.dataSize > 0);
}

static size_t test_wav_corpus_directory(const char *directory)
{
    WIN32_FIND_DATAA entry;
    HANDLE find;
    char pattern[2048];
    size_t count = 0;
    int written;

    written = snprintf(
        pattern, sizeof(pattern), "%s\\*", directory);
    if (written < 0 || (size_t)written >= sizeof(pattern)) {
        ++failures;
        return 0;
    }
    find = FindFirstFileA(pattern, &entry);
    if (find == INVALID_HANDLE_VALUE) {
        fprintf(stderr, "could not scan WAV corpus: %s\n", directory);
        ++failures;
        return 0;
    }
    do {
        char path[2048];
        const char *extension;

        if (strcmp(entry.cFileName, ".") == 0 ||
            strcmp(entry.cFileName, "..") == 0) {
            continue;
        }
        written = snprintf(
            path,
            sizeof(path),
            "%s\\%s",
            directory,
            entry.cFileName);
        if (written < 0 || (size_t)written >= sizeof(path)) {
            ++failures;
            continue;
        }
        if ((entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0 &&
            (entry.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT) == 0) {
            count += test_wav_corpus_directory(path);
            continue;
        }
        extension = strrchr(entry.cFileName, '.');
        if (extension != NULL && _stricmp(extension, ".wav") == 0) {
            JPBPCAudioWavInfo info;

            if (!jpb_PCAudioInspectWavFile(path, &info)) {
                fprintf(stderr, "invalid WAV in corpus: %s\n", path);
                ++failures;
            } else {
                ++count;
            }
        }
    } while (FindNextFileA(find, &entry));
    FindClose(find);
    return count;
}

static size_t test_exact_bank_path_corpus(const char *sound_directory)
{
    size_t bank_index;
    size_t count = 0;

    for (bank_index = 0;
         bank_index < JPB_SOUND_BANK_TABLE_COUNT;
         ++bank_index) {
        const JPBSoundBankData *bank =
            &jpb_soundBankTable[bank_index];
        int path_index;

        for (path_index = 0; path_index < bank->count; ++path_index) {
            JPBPCAudioWavInfo info;
            char path[2048];
            char *cursor;
            int written = snprintf(
                path,
                sizeof(path),
                "%s\\sfx\\final\\%s",
                sound_directory,
                bank->paths[path_index]);

            if (written < 0 || (size_t)written >= sizeof(path)) {
                ++failures;
                continue;
            }
            for (cursor = path; *cursor != '\0'; ++cursor) {
                if (*cursor == '/') {
                    *cursor = '\\';
                }
            }
            if (!jpb_PCAudioInspectWavFile(path, &info)) {
                fprintf(
                    stderr,
                    "missing/invalid exact bank WAV: %s (%s)\n",
                    path,
                    bank->directory);
                ++failures;
            }
            ++count;
        }
    }
    return count;
}

int main(int argc, char **argv)
{
    test_pcm_wav_parser();
    test_bank_resolution();
    test_exact_alias_banks();
    test_two_player_banks();
    if (argc == 3 && strcmp(argv[1], "--corpus") == 0) {
        size_t count = test_wav_corpus_directory(argv[2]);
        size_t bank_path_count = test_exact_bank_path_corpus(argv[2]);

        CHECK(count > 0);
        /*
         * The executable owns 696 unique pointer-array slots. Its 43 bank
         * descriptors deliberately reuse arrays and longer prefixes, which
         * expands to 1,242 descriptor-visible path references.
         */
        CHECK(bank_path_count == 1242);
        printf("validated %zu WAV files\n", count);
        printf(
            "validated %zu exact sound-bank descriptor references\n",
            bank_path_count);
    } else {
        test_real_wav(argc, argv);
    }
    if (failures != 0) {
        fprintf(stderr, "%d PC audio test(s) failed\n", failures);
        return 1;
    }
    puts("PC audio tests passed");
    return 0;
}
