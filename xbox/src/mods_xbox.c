/* Xbox side of mod packages: conversion stamp check and package listing.
   Loading, overlays, characters and saves are the shared mods.cpp. */
#include "mods_xbox.h"
#include <windows.h>
#include <stdio.h>
#include <string.h>

void jpb_XboxModsForEachPackage(void (*visit)(const char *directory, void *user),
    void *user)
{
    WIN32_FIND_DATAA data;
    char directory[MAX_PATH], manifest[MAX_PATH];
    HANDLE find = FindFirstFileA("D:\\mods\\*", &data);
    if (find == INVALID_HANDLE_VALUE) return;
    do {
        if (!(data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            !strcmp(data.cFileName, ".") || !strcmp(data.cFileName, ".."))
            continue;
        snprintf(directory, sizeof(directory), "D:\\mods\\%s", data.cFileName);
        snprintf(manifest, sizeof(manifest), "%s\\mod.json", directory);
        if (GetFileAttributesA(manifest) != INVALID_FILE_ATTRIBUTES)
            visit(directory, user);
    } while (FindNextFileA(find, &data));
    FindClose(find);
}

typedef struct FormatCheck {
    char *error;
    size_t capacity;
    int ok;
} FormatCheck;

static void check_package(const char *directory, void *user)
{
    FormatCheck *check = user;
    char path[MAX_PATH], text[512];
    const char *name = strrchr(directory, '\\') + 1;
    const char *field;
    size_t length;
    int format = 0;
    FILE *file;
    if (!check->ok) return;
    snprintf(path, sizeof(path), "%s\\xbox-package.json", directory);
    file = fopen(path, "rb");
    if (!file) {
        snprintf(check->error, check->capacity,
            "%s is a PC package; convert it with xbox/tools/stage_mods.py", name);
        check->ok = 0;
        return;
    }
    length = fread(text, 1, sizeof(text) - 1, file);
    fclose(file);
    text[length] = 0;
    field = strstr(text, "\"xboxFormat\"");
    if (field) field = strchr(field, ':');
    if (!field || sscanf(field + 1, "%d", &format) != 1 ||
        format != JPB_XBOX_MOD_FORMAT) {
        snprintf(check->error, check->capacity,
            "%s was converted for Xbox package format %d; this build reads "
            "format %d. Convert it again with xbox/tools/stage_mods.py",
            name, format, JPB_XBOX_MOD_FORMAT);
        check->ok = 0;
    }
}

int jpb_XboxModsCheckFormat(char *error, size_t capacity)
{
    FormatCheck check = {error, capacity, 1};
    if (error && capacity) error[0] = 0;
    jpb_XboxModsForEachPackage(check_package, &check);
    return check.ok;
}
