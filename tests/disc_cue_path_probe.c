/* Resolve a cue sheet's relative FILE entry from another working directory.
 * The synthetic track is deliberately undersized so the error distinguishes
 * successful path resolution from a missing file. */
#include "musashi_disc_media.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define chdir _chdir
#define mkdir_one(p) _mkdir(p)
#else
#include <sys/stat.h>
#include <unistd.h>
#define mkdir_one(p) mkdir(p, 0700)
#endif

static int write_file(const char *path, const char *data, size_t size) {
    FILE *f = fopen(path, "wb");
    if (!f) return 0;
    if (fwrite(data, 1, size, f) != size) { fclose(f); return 0; }
    return fclose(f) == 0;
}

static const char wrong_size[] =
    "%s is not a complete image of Brave Fencer Musashi (USA, SLUS-00726): "
    "the size does not match.";
static const char malformed[] = "%s could not be read as a cue sheet.";
static const char missing[] = "The disc image file %s is missing.";

static int check(const char *cue, const char *label, const char *format, const char *subject) {
    char why[4096] = "", expected[4096];
    int length = snprintf(expected, sizeof expected, format, subject);
    MusashiDiscMedia *media = musashi_disc_media_open_image(cue, why, sizeof why);
    if (media) {
        musashi_disc_media_close(media);
        printf("FAIL %s: unexpected acceptance\n", label);
        return 1;
    }
    if (length < 0 || (size_t)length >= sizeof expected || strcmp(why, expected)) {
        printf("FAIL %s: expected [%s], got [%s]\n", label, expected, why);
        return 1;
    }
    printf("ok %s: %s\n", label, why);
    return 0;
}

/* The public fixture needs no disc. Eight distinct short files exercise the
 * accepted FILE count before the loader reports their incomplete total. */
static int check_track_count(void) {
    const char data[8] = {0};
    char name[32];
    FILE *cue = fopen("many.cue", "wb");
    unsigned i;
    int bad = 0;
    if (!cue) return 1;
    for (i = 0; i < sizeof data; ++i) {
        snprintf(name, sizeof name, "cue-track-%u.bin", i);
        if (!write_file(name, data, i + 1) || fprintf(cue, "FILE \"%s\" BINARY\n", name) < 0) {
            fclose(cue);
            return 1;
        }
    }
    if (fclose(cue)) return 1;
    bad |= check("many.cue", "eight tracks of different sizes", wrong_size, "many.cue");
    if (remove(name)) return 1;
    bad |= check("many.cue", "eighth track missing", missing, name);
    cue = fopen("many.cue", "ab");
    if (!cue) return 1;
    if (fputs("FILE \"ninth.bin\" BINARY\n", cue) == EOF) {
        fclose(cue);
        return 1;
    }
    if (fclose(cue)) return 1;
    return bad | check("many.cue", "ninth FILE entry", malformed, "many.cue");
}

int main(int argc, char **argv) {
    const char *root = argc > 1 ? argv[1] : "disc_cue_path_probe.tmp";
    char disc[1024], other[1024], cue[1100], bin[1100], back[1100];
    static const char sheet[] = "FILE \"disc.bin\" BINARY\r\n  TRACK 01 MODE2/2352\r\n    INDEX 01 00:00:00\r\n";
    char sector[2352] = {0};
    int bad = 0;
    size_t i;
    snprintf(disc, sizeof disc, "%s/disc", root);
    snprintf(other, sizeof other, "%s/elsewhere", root);
    mkdir_one(root); mkdir_one(disc); mkdir_one(other);
    snprintf(cue, sizeof cue, "%s/disc.cue", disc);
    snprintf(bin, sizeof bin, "%s/disc.bin", disc);
    if (!write_file(cue, sheet, sizeof sheet - 1) || !write_file(bin, sector, sizeof sector)) {
        printf("FAIL setup\n");
        return 1;
    }
    if (chdir(other) != 0) { printf("FAIL chdir\n"); return 1; }
    /* Absolute path of the cue from the new cwd: root may be relative. */
    if (root[0] != '/' && !(root[0] && root[1] == ':')) {
        snprintf(cue, sizeof cue, "../disc/disc.cue");
    }
    bad |= check(cue, "2352-byte track, forward-slash path, cwd elsewhere", wrong_size, cue);
#ifdef _WIN32
    snprintf(back, sizeof back, "%s", cue);
    for (i = 0; back[i]; ++i) if (back[i] == '/') back[i] = '\\';
    bad |= check(back, "2352-byte track, backslash path, cwd elsewhere", wrong_size, back);
#else
    (void)back; (void)i;
#endif
    {
        static const char broken[] = "FILE \"unterminated\n";
        if (!write_file("malformed.cue", broken, sizeof broken - 1)) return 1;
        bad |= check("malformed.cue", "unterminated FILE entry", malformed, "malformed.cue");
    }
    return bad | check_track_count();
}
