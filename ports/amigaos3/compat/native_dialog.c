/* Allegro 5's native dialogs on classic AmigaOS 3.x, for Liberation Circuit.
 * File dialogs are asl.library file requesters; message boxes are Intuition
 * EasyRequests; the text log writes to the console.
 * Copyright (c) 2026 AmiCradle Team. GPL-3.0 with the game (see LICENSE.md).
 */
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <exec/types.h>
#include <libraries/asl.h>
#include <intuition/intuition.h>
#include <proto/exec.h>
#include <proto/asl.h>
#include <proto/intuition.h>
#include <proto/dos.h>
#include <intuition/intuitionbase.h>

#include <allegro5/allegro.h>
#include <allegro5/allegro_native_dialog.h>

struct ALLEGRO_FILECHOOSER {
    char *initial_path;
    char *title;
    char pattern[128];              /* AmigaDOS pattern, e.g. #?.c */
    int mode;
    char *result;                   /* the chosen file, full path */
};

struct ALLEGRO_TEXTLOG {
    int unused;
};

static bool native_dialog_initialized;

bool al_init_native_dialog_addon(void) { native_dialog_initialized = true; return true; }
bool al_is_native_dialog_addon_initialized(void) { return native_dialog_initialized; }
void al_shutdown_native_dialog_addon(void) { native_dialog_initialized = false; }

static char *dup_or_null(const char *s)
{
    char *d;
    if (!s) return NULL;
    d = malloc(strlen(s) + 1);
    if (d) strcpy(d, s);
    return d;
}

/* "*.c" or "*.c;*.h" -> "#?.c" or "(#?.c|#?.h)"; empty -> "#?" */
static void to_amiga_pattern(char *out, size_t size, const char *patterns)
{
    size_t n = 0;
    int many = patterns && strpbrk(patterns, "; ") != NULL;
    const char *p = patterns;
    out[0] = 0;
    if (!p || !*p) { snprintf(out, size, "#?"); return; }
    if (many && n < size - 1) out[n++] = '(';
    for (; *p && n < size - 4; p++) {
        if (*p == '*') { out[n++] = '#'; out[n++] = '?'; }
        else if (*p == ';' || *p == ' ') { if (n && out[n - 1] != '|' && out[n - 1] != '(') out[n++] = '|'; }
        else out[n++] = *p;
    }
    if (many && n < size - 1) out[n++] = ')';
    out[n] = 0;
}

ALLEGRO_FILECHOOSER *al_create_native_file_dialog(
    const char *initial_path, const char *title, const char *patterns, int mode)
{
    ALLEGRO_FILECHOOSER *fc = calloc(1, sizeof(*fc));
    if (!fc) return NULL;
    fc->initial_path = dup_or_null(initial_path);
    fc->title = dup_or_null(title ? title : "Choose file");
    fc->mode = mode;
    to_amiga_pattern(fc->pattern, sizeof(fc->pattern), patterns);
    return fc;
}

bool al_show_native_file_dialog(ALLEGRO_DISPLAY *display, ALLEGRO_FILECHOOSER *fc)
{
    struct Library *AslBase;
    struct FileRequester *fr;
    char drawer[256] = "", file[108] = "";
    bool ok = false;
    (void)display;
    if (!fc) return false;
    free(fc->result);
    fc->result = NULL;
    AslBase = OpenLibrary((CONST_STRPTR)"asl.library", 39);
    if (!AslBase) return false;
    if (fc->initial_path && *fc->initial_path) {
        /* a drawer, or a drawer and a file */
        const char *slash = strrchr(fc->initial_path, '/');
        const char *colon = strrchr(fc->initial_path, ':');
        const char *cut = slash > colon ? slash : colon;
        if (cut) {
            size_t dl = (size_t)(cut - fc->initial_path) + (cut == colon ? 1 : 0);
            if (dl >= sizeof(drawer)) dl = sizeof(drawer) - 1;
            memcpy(drawer, fc->initial_path, dl);
            drawer[dl] = 0;
            snprintf(file, sizeof(file), "%s", cut + 1);
        } else {
            snprintf(drawer, sizeof(drawer), "%s", fc->initial_path);
        }
    }
    fr = (struct FileRequester *)AllocAslRequestTags(ASL_FileRequest,
        ASLFR_TitleText, (ULONG)fc->title,
        ASLFR_Screen, (ULONG)IntuitionBase->FirstScreen,   /* the game's, in front */
        ASLFR_InitialDrawer, (ULONG)drawer,
        ASLFR_InitialFile, (ULONG)file,
        ASLFR_InitialPattern, (ULONG)fc->pattern,
        ASLFR_DoPatterns, TRUE,
        ASLFR_DoSaveMode, (fc->mode & ALLEGRO_FILECHOOSER_SAVE) ? TRUE : FALSE,
        ASLFR_DrawersOnly, (fc->mode & ALLEGRO_FILECHOOSER_FOLDER) ? TRUE : FALSE,
        ASLFR_RejectIcons, TRUE,
        TAG_DONE);
    if (fr) {
        if (AslRequest(fr, NULL)) {
            char path[512];
            snprintf(path, sizeof(path), "%s", (const char *)fr->fr_Drawer);
            if (!(fc->mode & ALLEGRO_FILECHOOSER_FOLDER))
                AddPart((STRPTR)path, fr->fr_File, sizeof(path));
            if (path[0] && !((fc->mode & ALLEGRO_FILECHOOSER_FILE_MUST_EXIST) && !fr->fr_File[0])) {
                fc->result = dup_or_null(path);
                ok = fc->result != NULL;
            }
        }
        FreeAslRequest(fr);
    }
    CloseLibrary(AslBase);
    return ok;
}

int al_get_native_file_dialog_count(const ALLEGRO_FILECHOOSER *fc) { return fc && fc->result ? 1 : 0; }

const char *al_get_native_file_dialog_path(const ALLEGRO_FILECHOOSER *fc, size_t index)
{
    return fc && fc->result && index == 0 ? fc->result : NULL;
}

void al_destroy_native_file_dialog(ALLEGRO_FILECHOOSER *fc)
{
    if (!fc) return;
    free(fc->initial_path);
    free(fc->title);
    free(fc->result);
    free(fc);
}

/* An EasyRequest: buttons "Yes|No" etc. Returns Allegro's 1 for the first
 * button, 2 for the second, 0 for the last (cancel) or none. */
int al_show_native_message_box(ALLEGRO_DISPLAY *display, const char *title, const char *heading,
                               const char *text, const char *buttons, int flags)
{
    struct EasyStruct es;
    char body[1024], gadgets[256];
    LONG r;
    (void)display;
    snprintf(body, sizeof(body), "%s%s%s", heading ? heading : "", heading && *heading ? "\n\n" : "", text ? text : "");
    /* % is a format character for EasyRequest */
    {
        char safe[1024];
        size_t i, n = 0;
        for (i = 0; body[i] && n < sizeof(safe) - 2; i++) { if (body[i] == '%') safe[n++] = '%'; safe[n++] = body[i]; }
        safe[n] = 0;
        snprintf(body, sizeof(body), "%s", safe);
    }
    if (buttons && *buttons) snprintf(gadgets, sizeof(gadgets), "%s", buttons);
    else if (flags & ALLEGRO_MESSAGEBOX_YES_NO) snprintf(gadgets, sizeof(gadgets), "Yes|No");
    else if (flags & ALLEGRO_MESSAGEBOX_OK_CANCEL) snprintf(gadgets, sizeof(gadgets), "OK|Cancel");
    else snprintf(gadgets, sizeof(gadgets), "OK");
    es.es_StructSize = sizeof(es);
    es.es_Flags = 0;
    es.es_Title = (UBYTE *)(title ? title : "Liberation Circuit");
    es.es_TextFormat = (UBYTE *)body;
    es.es_GadgetFormat = (UBYTE *)gadgets;
    r = EasyRequestArgs(NULL, &es, NULL, NULL);
    /* EasyRequest gives 1, 2, ... for the first buttons and 0 for the last;
     * Allegro wants 1, 2, ... for every button */
    if (r == 0) {
        const char *g;
        int count = 1;
        for (g = gadgets; *g; g++) if (*g == '|') count++;
        return count;
    }
    return (int)r;
}

ALLEGRO_TEXTLOG *al_open_native_text_log(const char *title, int flags)
{
    (void)title; (void)flags;
    return calloc(1, sizeof(ALLEGRO_TEXTLOG));
}

void al_close_native_text_log(ALLEGRO_TEXTLOG *textlog) { free(textlog); }

void al_append_native_text_log(ALLEGRO_TEXTLOG *textlog, const char *format, ...)
{
    va_list ap;
    (void)textlog;
    va_start(ap, format);
    vprintf(format, ap);
    va_end(ap);
}
