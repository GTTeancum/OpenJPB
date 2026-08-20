#define WIN32_LEAN_AND_MEAN
#ifdef UNICODE
#undef UNICODE
#endif
#ifdef _UNICODE
#undef _UNICODE
#endif
#include <windows.h>
#include <tlhelp32.h>
#include <mmsystem.h>

#include "jpb/camera.h"
#include "jpb/physics.h"
#include "jpb/player.h"
#include "jpb/scene.h"
#include "jpb/world.h"

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum {
    RETAIL_RVA_GP_WORLD = 0x4afda0,
    RETAIL_RVA_GLOBAL_FRAME_RATE = 0x4cc0a0,
    RETAIL_RVA_NEW_CAMERA_FLAG = 0x4f1a98,
    RETAIL_RVA_LEAD = 0x4f1ac8,
    RETAIL_RVA_CAMERA_LEAD = 0x4f1ad0,
    RETAIL_RVA_LEVEL_SELECT = 0x537dea,
    RETAIL_RVA_TOTAL_FRAMES = 0x547b48,
    RETAIL_RVA_CURRENT_CAMERA_TYPE = 0x547c10,
    RETAIL_RVA_GLOBAL_TIMER = 0x581fc0,
    RETAIL_RVA_PLAYER_ONSCREEN = 0x547b40,
    RETAIL_RVA_P1_Y = 0x932c88,
    RETAIL_RVA_P1_X = 0x932c8c,
    RETAIL_RVA_GAME_STRUCT = 0x10da140,
    RETAIL_RVA_CAMERA = 0x10de6a0,
    RETAIL_RVA_CAMERA_LOCATION = 0x10de678,
    RETAIL_RVA_CAMERA_POSITION = 0x10de680
};

typedef struct RetailSample {
    int8_t level;
    int32_t totalFrames;
    uint32_t globalTimer;
    int32_t globalFrameRate;
    int32_t currentCameraType;
    int32_t newCameraFlag;
    int32_t playerOnscreen[2];
    int8_t numPlayers;
    WorldData *worldAddress;
    WorldData world;
    Camera camera;
    _svector cameraLocation;
    VECTOR cameraPosition;
    _svector lead;
    int32_t cameraLead;
    float inputX;
    float inputY;
    playerObject player[2];
    sceneObject scene[2];
    physicsObject physics[2];
    int playerValid[2];
} RetailSample;

static int read_remote(
    HANDLE process,
    uintptr_t address,
    void *destination,
    size_t size)
{
    SIZE_T bytes_read = 0;

    return ReadProcessMemory(
               process,
               (const void *)address,
               destination,
               size,
               &bytes_read) != 0 &&
        bytes_read == size;
}

static DWORD find_process(const char *name)
{
    PROCESSENTRY32 entry;
    HANDLE snapshot;
    DWORD pid = 0;

    memset(&entry, 0, sizeof(entry));
    entry.dwSize = sizeof(entry);
    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    if (Process32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szExeFile, name) == 0) {
                pid = entry.th32ProcessID;
                break;
            }
        } while (Process32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return pid;
}

static uintptr_t find_module_base(DWORD pid, const char *name)
{
    MODULEENTRY32 entry;
    HANDLE snapshot;
    uintptr_t base = 0;

    memset(&entry, 0, sizeof(entry));
    entry.dwSize = sizeof(entry);
    snapshot = CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, pid);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return 0;
    }
    if (Module32First(snapshot, &entry)) {
        do {
            if (_stricmp(entry.szModule, name) == 0) {
                base = (uintptr_t)entry.modBaseAddr;
                break;
            }
        } while (Module32Next(snapshot, &entry));
    }
    CloseHandle(snapshot);
    return base;
}

static int read_player(
    HANDLE process,
    playerObject *remote_player,
    playerObject *player,
    sceneObject *scene,
    physicsObject *physics)
{
    sceneObject *remote_scene;
    physicsObject *remote_physics;

    if (remote_player == NULL ||
        !read_remote(
            process, (uintptr_t)remote_player, player, sizeof(*player))) {
        return 0;
    }
    remote_scene = (sceneObject *)player->playerRoot.pParent;
    if (remote_scene == NULL ||
        !read_remote(
            process, (uintptr_t)remote_scene, scene, sizeof(*scene))) {
        return 0;
    }
    remote_physics = (physicsObject *)scene->pPhysics;
    return remote_physics != NULL &&
        read_remote(
            process, (uintptr_t)remote_physics, physics, sizeof(*physics));
}

static int read_sample(
    HANDLE process,
    uintptr_t base,
    RetailSample *sample)
{
    int player;

    memset(sample, 0, sizeof(*sample));
    if (!read_remote(
            process,
            base + RETAIL_RVA_LEVEL_SELECT,
            &sample->level,
            sizeof(sample->level)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_TOTAL_FRAMES,
            &sample->totalFrames,
            sizeof(sample->totalFrames)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_GLOBAL_TIMER,
            &sample->globalTimer,
            sizeof(sample->globalTimer)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_GLOBAL_FRAME_RATE,
            &sample->globalFrameRate,
            sizeof(sample->globalFrameRate)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_CURRENT_CAMERA_TYPE,
            &sample->currentCameraType,
            sizeof(sample->currentCameraType)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_NEW_CAMERA_FLAG,
            &sample->newCameraFlag,
            sizeof(sample->newCameraFlag)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_PLAYER_ONSCREEN,
            sample->playerOnscreen,
            sizeof(sample->playerOnscreen)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_GAME_STRUCT + 526,
            &sample->numPlayers,
            sizeof(sample->numPlayers)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_GP_WORLD,
            &sample->worldAddress,
            sizeof(sample->worldAddress)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_CAMERA,
            &sample->camera,
            sizeof(sample->camera)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_CAMERA_LOCATION,
            &sample->cameraLocation,
            sizeof(sample->cameraLocation)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_CAMERA_POSITION,
            &sample->cameraPosition,
            sizeof(sample->cameraPosition)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_LEAD,
            &sample->lead,
            sizeof(sample->lead)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_CAMERA_LEAD,
            &sample->cameraLead,
            sizeof(sample->cameraLead)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_P1_X,
            &sample->inputX,
            sizeof(sample->inputX)) ||
        !read_remote(
            process,
            base + RETAIL_RVA_P1_Y,
            &sample->inputY,
            sizeof(sample->inputY))) {
        return 0;
    }
    if (sample->worldAddress == NULL ||
        !read_remote(
            process,
            (uintptr_t)sample->worldAddress,
            &sample->world,
            sizeof(sample->world))) {
        return 0;
    }
    for (player = 0; player < 2; ++player) {
        playerObject *remote_player = player == 0
            ? sample->world.player0
            : sample->world.player1;

        sample->playerValid[player] = read_player(
            process,
            remote_player,
            &sample->player[player],
            &sample->scene[player],
            &sample->physics[player]);
    }
    return 1;
}

static double vector_distance(const FVECTOR *a, const FVECTOR *b)
{
    double dx = (double)a->vx - (double)b->vx;
    double dy = (double)a->vy - (double)b->vy;
    double dz = (double)a->vz - (double)b->vz;

    return sqrt(dx * dx + dy * dy + dz * dz);
}

static void log_sample(
    FILE *output,
    const RetailSample *sample,
    const RetailSample *previous,
    unsigned pulse)
{
    const BAP_CAMERADOLLY *dolly =
        &sample->world.aDolly[(uint16_t)sample->world.currentDolly & 0xff];
    const BAP_CAMERADOLLY *backup_dolly =
        &sample->world.aBkDolly[
            (uint16_t)sample->world.currentDolly & 0xff];
    int dolly_changed =
        memcmp(dolly, backup_dolly, sizeof(*dolly)) != 0;
    double camera_distance = 0.0;
    float player_dx = 0.0f;
    float player_dy = 0.0f;
    float player_dz = 0.0f;
    int camera_dx = 0;
    int camera_dy = 0;
    int camera_dz = 0;

    if (sample->playerValid[0]) {
        FVECTOR camera_position = {
            (float)sample->camera.focus.vx,
            (float)sample->camera.focus.vy,
            (float)sample->camera.focus.vz
        };

        camera_distance = vector_distance(
            &camera_position, &sample->physics[0].pos);
    }
    if (previous != NULL && previous->playerValid[0] &&
        sample->playerValid[0]) {
        player_dx = sample->physics[0].pos.vx - previous->physics[0].pos.vx;
        player_dy = sample->physics[0].pos.vy - previous->physics[0].pos.vy;
        player_dz = sample->physics[0].pos.vz - previous->physics[0].pos.vz;
        camera_dx = sample->camera.focus.vx - previous->camera.focus.vx;
        camera_dy = sample->camera.focus.vy - previous->camera.focus.vy;
        camera_dz = sample->camera.focus.vz - previous->camera.focus.vz;
    }
    fprintf(
        output,
        "retail camera pulse=%u frame=%d timer=%u step=%d level=%d "
        "players=%d camera_type=%d new_camera=%d "
        "dolly=%d override=%d flags=%08x backup_flags=%08x "
        "dolly_changed=%d view=%08x "
        "p0_valid=%d p0=(%.1f,%.1f,%.1f) p0_vpos=(%d,%d,%d) "
        "p0_delta=(%.1f,%.1f,%.1f) p0_mov=(%.1f,%.1f,%.1f) "
        "p0_cpad=%08x/%08x p0_held=%08x "
        "p1_valid=%d p1=(%.1f,%.1f,%.1f) p1_mov=(%.1f,%.1f,%.1f) "
        "p1_cpad=%08x/%08x p1_held=%08x "
        "camera=(%d,%d,%d) eye=(%d,%d,%d) "
        "camera_location=(%d,%d,%d) camera_delta=(%d,%d,%d) "
        "destination=(%d,%d,%d) remaining=(%d,%d,%d) "
        "angle=%d,%d,%d destination_angle=%d,%d,%d "
        "distance=%.1f lead=%d/%d/%d dot=%d "
        "input=(%.4f,%.4f) "
        "target=(%d,%d,%d) onscreen=%d/%d "
        "dolly_offset=%d/%d/%d slack=%d/%d/%d off=%d/%d/%d\n",
        pulse,
        sample->totalFrames,
        (unsigned)sample->globalTimer,
        sample->globalFrameRate,
        (int)sample->level,
        sample->numPlayers,
        sample->currentCameraType,
        sample->newCameraFlag,
        (int)sample->world.currentDolly,
        (int)sample->world.overRideDolly,
        (unsigned)dolly->flags,
        (unsigned)backup_dolly->flags,
        dolly_changed,
        (unsigned)sample->camera.viewType,
        sample->playerValid[0],
        sample->physics[0].pos.vx,
        sample->physics[0].pos.vy,
        sample->physics[0].pos.vz,
        sample->physics[0].vpos.vx,
        sample->physics[0].vpos.vy,
        sample->physics[0].vpos.vz,
        player_dx,
        player_dy,
        player_dz,
        sample->physics[0].mov.vx,
        sample->physics[0].mov.vy,
        sample->physics[0].mov.vz,
        (unsigned)sample->player[0].playerPad.cpad[0],
        (unsigned)sample->player[0].playerPad.cpad[1],
        (unsigned)sample->player[0].heldMask,
        sample->playerValid[1],
        sample->physics[1].pos.vx,
        sample->physics[1].pos.vy,
        sample->physics[1].pos.vz,
        sample->physics[1].mov.vx,
        sample->physics[1].mov.vy,
        sample->physics[1].mov.vz,
        (unsigned)sample->player[1].playerPad.cpad[0],
        (unsigned)sample->player[1].playerPad.cpad[1],
        (unsigned)sample->player[1].heldMask,
        sample->camera.focus.vx,
        sample->camera.focus.vy,
        sample->camera.focus.vz,
        sample->cameraPosition.vx,
        sample->cameraPosition.vy,
        sample->cameraPosition.vz,
        sample->cameraLocation.vx,
        sample->cameraLocation.vy,
        sample->cameraLocation.vz,
        camera_dx,
        camera_dy,
        camera_dz,
        sample->camera.focusDest.vx,
        sample->camera.focusDest.vy,
        sample->camera.focusDest.vz,
        sample->camera.focusDest.vx - sample->camera.focus.vx,
        sample->camera.focusDest.vy - sample->camera.focus.vy,
        sample->camera.focusDest.vz - sample->camera.focus.vz,
        sample->camera.angle.vx,
        sample->camera.angle.vy,
        sample->camera.angle.vz,
        sample->camera.angleDest.vx,
        sample->camera.angleDest.vy,
        sample->camera.angleDest.vz,
        camera_distance,
        (int)sample->lead.vx,
        (int)sample->lead.vy,
        (int)sample->lead.vz,
        sample->cameraLead,
        sample->inputX,
        sample->inputY,
        sample->world.location.vx,
        sample->world.location.vy,
        sample->world.location.vz,
        sample->playerOnscreen[0],
        sample->playerOnscreen[1],
        dolly->offset.vx,
        dolly->offset.vy,
        dolly->offset.vz,
        (int)dolly->slackx,
        (int)dolly->slacky,
        (int)dolly->slackz,
        (int)dolly->offx,
        (int)dolly->offy,
        (int)dolly->offz);
    fflush(output);
}

static void log_trail_header(FILE *output)
{
    fputs(
        "frame,global_timer,global_frame_rate,level,num_players,"
        "camera_type,new_camera_flag,input_x,input_y,"
        "p0_pad0,p0_pad1,p0_held,p0_x,p0_y,p0_z,"
        "p0_vpos_x,p0_vpos_y,p0_vpos_z,p0_mov_x,p0_mov_y,p0_mov_z,"
        "camera_x,camera_y,camera_z,eye_x,eye_y,eye_z,"
        "camera_location_x,camera_location_y,camera_location_z,"
        "camera_dest_x,camera_dest_y,camera_dest_z,"
        "pitch,yaw,roll,dest_pitch,dest_yaw,dest_roll,"
        "target_x,target_y,target_z,dolly,override,flags,backup_flags,"
        "lead_x,lead_y,lead_z,lead_dot,"
        "dolly_offset_x,dolly_offset_y,dolly_offset_z,"
        "dolly_slack_x,dolly_slack_y,dolly_slack_z,"
        "dolly_off_x,dolly_off_y,dolly_off_z\n",
        output);
}

static void log_trail_sample(
    FILE *output,
    const RetailSample *sample)
{
    int dolly_index = (uint16_t)sample->world.currentDolly & 0xff;
    const BAP_CAMERADOLLY *dolly =
        &sample->world.aDolly[dolly_index];
    const BAP_CAMERADOLLY *backup_dolly =
        &sample->world.aBkDolly[dolly_index];

    fprintf(
        output,
        "%d,%u,%d,%d,%d,%d,%d,%.6f,%.6f,%08x,%08x,%08x,"
        "%.3f,%.3f,%.3f,%d,%d,%d,%.3f,%.3f,%.3f,"
        "%d,%d,%d,%d,%d,%d,%d,%d,%d,"
        "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
        "%d,%d,%08x,%08x,%d,%d,%d,%d,"
        "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
        sample->totalFrames,
        (unsigned)sample->globalTimer,
        sample->globalFrameRate,
        (int)sample->level,
        sample->numPlayers,
        sample->currentCameraType,
        sample->newCameraFlag,
        sample->inputX,
        sample->inputY,
        (unsigned)sample->player[0].playerPad.cpad[0],
        (unsigned)sample->player[0].playerPad.cpad[1],
        (unsigned)sample->player[0].heldMask,
        sample->physics[0].pos.vx,
        sample->physics[0].pos.vy,
        sample->physics[0].pos.vz,
        sample->physics[0].vpos.vx,
        sample->physics[0].vpos.vy,
        sample->physics[0].vpos.vz,
        sample->physics[0].mov.vx,
        sample->physics[0].mov.vy,
        sample->physics[0].mov.vz,
        sample->camera.focus.vx,
        sample->camera.focus.vy,
        sample->camera.focus.vz,
        sample->cameraPosition.vx,
        sample->cameraPosition.vy,
        sample->cameraPosition.vz,
        sample->cameraLocation.vx,
        sample->cameraLocation.vy,
        sample->cameraLocation.vz,
        sample->camera.focusDest.vx,
        sample->camera.focusDest.vy,
        sample->camera.focusDest.vz,
        sample->camera.angle.vx,
        sample->camera.angle.vy,
        sample->camera.angle.vz,
        sample->camera.angleDest.vx,
        sample->camera.angleDest.vy,
        sample->camera.angleDest.vz,
        sample->world.location.vx,
        sample->world.location.vy,
        sample->world.location.vz,
        (int)sample->world.currentDolly,
        (int)sample->world.overRideDolly,
        (unsigned)dolly->flags,
        (unsigned)backup_dolly->flags,
        (int)sample->lead.vx,
        (int)sample->lead.vy,
        (int)sample->lead.vz,
        sample->cameraLead,
        dolly->offset.vx,
        dolly->offset.vy,
        dolly->offset.vz,
        (int)dolly->slackx,
        (int)dolly->slacky,
        (int)dolly->slackz,
        (int)dolly->offx,
        (int)dolly->offy,
        (int)dolly->offz);
}

int main(int argument_count, char **arguments)
{
    const char *process_name = "game.exe";
    const char *output_path = "retail_camera_pulses.log";
    const char *trail_output_path = "retail_camera_trail.csv";
    RetailSample sample;
    RetailSample previous_pulse;
    HANDLE process;
    uintptr_t base;
    FILE *output;
    FILE *trail_output;
    DWORD pid;
    unsigned pulse = 0;
    int have_previous = 0;
    int wait_for_left_stick = 0;
    int recording_started = 1;
    int32_t last_observed_frame = INT32_MIN;
    int32_t last_pulse_frame = INT32_MIN;
    unsigned trail_rows = 0;
    unsigned missed_frames = 0;
    int timer_period_active = 0;
    int argument;

    for (argument = 1; argument < argument_count; ++argument) {
        if (strcmp(arguments[argument], "--process") == 0 &&
            argument + 1 < argument_count) {
            process_name = arguments[++argument];
        } else if (strcmp(arguments[argument], "--output") == 0 &&
                   argument + 1 < argument_count) {
            output_path = arguments[++argument];
        } else if (strcmp(arguments[argument], "--trail-output") == 0 &&
                   argument + 1 < argument_count) {
            trail_output_path = arguments[++argument];
        } else if (strcmp(
                       arguments[argument],
                       "--wait-for-left-stick") == 0) {
            wait_for_left_stick = 1;
            recording_started = 0;
        } else {
            fprintf(
                stderr,
                "usage: %s [--process game.exe] [--output path] "
                "[--trail-output path.csv] [--wait-for-left-stick]\n",
                arguments[0]);
            return 2;
        }
    }

    printf("waiting for %s...\n", process_name);
    do {
        pid = find_process(process_name);
        if (pid == 0) Sleep(250);
    } while (pid == 0);

    process = OpenProcess(
        PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (process == NULL) {
        fprintf(stderr, "OpenProcess failed: %lu\n", GetLastError());
        return 1;
    }
    base = find_module_base(pid, process_name);
    if (base == 0) {
        fprintf(stderr, "could not resolve module base\n");
        CloseHandle(process);
        return 1;
    }
    output = fopen(output_path, "wb");
    if (output == NULL) {
        fprintf(stderr, "could not open %s\n", output_path);
        CloseHandle(process);
        return 1;
    }
    trail_output = fopen(trail_output_path, "wb");
    if (trail_output == NULL) {
        fprintf(stderr, "could not open %s\n", trail_output_path);
        fclose(output);
        CloseHandle(process);
        return 1;
    }
    log_trail_header(trail_output);
    fprintf(
        output,
        "retail camera watcher pid=%lu base=%p pdb_rvas=matched "
        "wait_for_left_stick=%d\n",
        (unsigned long)pid,
        (void *)base,
        wait_for_left_stick);
    fflush(output);
    printf(
        "attached pid=%lu base=%p output=%s trail=%s\n",
        (unsigned long)pid,
        (void *)base,
        output_path,
        trail_output_path);
    timer_period_active = timeBeginPeriod(1) == TIMERR_NOERROR;
    fprintf(
        output,
        "retail camera watcher timer_period_1ms=%d\n",
        timer_period_active);
    fflush(output);

    for (;;) {
        DWORD exit_code;

        if (!GetExitCodeProcess(process, &exit_code) ||
            exit_code != STILL_ACTIVE) {
            break;
        }
        if (!recording_started) {
            if (read_sample(process, base, &sample) &&
                (fabsf(sample.inputX) >= 0.05f ||
                 fabsf(sample.inputY) >= 0.05f)) {
                fprintf(
                    output,
                    "retail camera input start frame=%d "
                    "input=(%.4f,%.4f)\n",
                    sample.totalFrames,
                    sample.inputX,
                    sample.inputY);
                log_sample(output, &sample, NULL, pulse);
                previous_pulse = sample;
                have_previous = 1;
                recording_started = 1;
                last_observed_frame = sample.totalFrames;
                last_pulse_frame = sample.totalFrames;
                log_trail_sample(trail_output, &sample);
                ++trail_rows;
                fflush(trail_output);
            }
        } else {
            int32_t observed_frame;

            if (read_remote(
                    process,
                    base + RETAIL_RVA_TOTAL_FRAMES,
                    &observed_frame,
                    sizeof(observed_frame)) &&
                observed_frame != last_observed_frame &&
                read_sample(process, base, &sample)) {
                int32_t frame_delta =
                    sample.totalFrames - last_observed_frame;
                int32_t pulse_delta =
                    sample.totalFrames - last_pulse_frame;

                if (last_observed_frame != INT32_MIN &&
                    frame_delta > 1) {
                    missed_frames += (unsigned)(frame_delta - 1);
                    fprintf(
                        output,
                        "retail camera watcher frame gap previous=%d "
                        "current=%d missed=%d total_missed=%u\n",
                        last_observed_frame,
                        sample.totalFrames,
                        frame_delta - 1,
                        missed_frames);
                }
                last_observed_frame = sample.totalFrames;
                log_trail_sample(trail_output, &sample);
                ++trail_rows;
                if (trail_rows % 60U == 0) {
                    fflush(trail_output);
                }
                if (last_pulse_frame == INT32_MIN ||
                    pulse_delta < 0 || pulse_delta >= 60) {
                    log_sample(
                        output,
                        &sample,
                        have_previous ? &previous_pulse : NULL,
                        ++pulse);
                    previous_pulse = sample;
                    have_previous = 1;
                    last_pulse_frame = sample.totalFrames;
                }
            }
        }
        Sleep(recording_started ? 2 : 10);
    }

    fprintf(
        output,
        "retail camera watcher process exited recording_started=%d "
        "trail_rows=%u missed_frames=%u\n",
        recording_started,
        trail_rows,
        missed_frames);
    if (timer_period_active) {
        timeEndPeriod(1);
    }
    fflush(trail_output);
    fclose(trail_output);
    fclose(output);
    CloseHandle(process);
    return 0;
}
