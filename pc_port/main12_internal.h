#ifndef MUSASHI_MAIN12_INTERNAL_H
#define MUSASHI_MAIN12_INTERNAL_H

#include "musashi_main12.h"
#include "card/save_payload.h"

enum { MUSASHI_MAIN12_ROW_CELL = 0x80185af0u, MUSASHI_MAIN12_ROW_DATA = 0x80075ad0u };
enum { MAIN12_ROW_COUNT = MUSASHI_SAVE_FILES, MAIN12_SUMMARY_BYTES = 128, MAIN12_COLUMNS = 7, MAIN12_TEXT_BYTES = 36,
       MAIN12_FILE_ICON_POSITION = 0x80181dc4u,
       MAIN12_FILE_RESULT = 0x801857e8u, MAIN12_RESTORE_BUFFER = 0x801857f8u, MAIN12_RESTORE_SIZE = 732u,
       MAIN12_CONFIRM_LABEL = 0x801826d8u, MAIN12_LOAD_REQUEST = 0x80185ad4u, MAIN12_LOAD_ROW = 0x80185ad8u,
       MAIN12_ROW_DESCRIPTORS = 0x801825a4u, MAIN12_ROW_STRINGS = 0x801853f8u };

/* Allocated only for a successfully verified disc mount, outside guest RAM. */
struct MusashiMain12Owner {
    MusashiGuestIrqImage *image;
    MusashiBootMemory *memory;
    const uint8_t *transfer; /* First 2048-byte payload in the immutable raw disc. */
    int (*matches)(const MusashiMain12Owner *owner, const uint8_t *live,
                   int installed, const uint8_t mailbox[16]);
    const uint8_t *exe_text; /* First sector payload of the mounted executable text. */
    int (*resident_matches)(const MusashiMain12Owner *owner, uint32_t start, uint32_t size);
    uint8_t mailbox[16];
    uint8_t directory_row[40];
    uint8_t directory_valid;
    uint64_t directory_fd_token;
    uint8_t menu_word_changed;
    uint8_t row_pointer_written; /* Sequential pointer cells retired for the four slots. */
    uint8_t row_pointer_mask;
    struct {
        uint8_t descriptors[MAIN12_ROW_COUNT][MAIN12_COLUMNS * 4];
        uint8_t strings[MAIN12_ROW_COUNT][MAIN12_COLUMNS * MAIN12_TEXT_BYTES];
        uint32_t icon_position;
        uint8_t started, complete, menu_code, icon_written;
        uint8_t confirm_phase, selected_file, load_row_written;
    } display;
    struct {
        const MusashiCdDevice *files;
        const MusashiBiosEvents *events;
        uint64_t open_id, allocation[8];
        int32_t fd;
        uint8_t bytes[768];
        uint16_t copied;
        uint8_t closed, result, label, request_cleared, consumed, dismissed, exit_written;
        uint8_t foreground_result_written;
    } restore;
    int installed;
    struct {
        const MusashiCdDevice *files;
        const MusashiBiosEvents *events;
        uint64_t descriptor, allocation[8];
        unsigned phase;
        int32_t payload_fd;
        uint8_t payload;
    } metadata_read;
    struct {
        const MusashiCdDevice *files;
        const MusashiBiosEvents *events;
        const void *file_userdata;
        uint64_t open_id, read_id, allocation[8];
        unsigned stage;
        uint8_t status_written;
        uint8_t bytes[640];
    } menu_metadata;
};

#endif
