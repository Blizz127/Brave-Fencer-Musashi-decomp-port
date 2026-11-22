/* Find and clear the first unused 56-byte Tower effect record. */
#include "psx_types.h"

extern u8 D_8011DB30[];
#if defined(__mips__) && !defined(__LP64__)
extern void func_80016714(s8 *record, s32 size);
#else
extern void func_80016714_native(u8 *record, s32 size);
#endif

u8 *func_801291C0(void)
{
    u8 *record = D_8011DB30;
    s32 index;

    for (index = 0; index < 128; ++index, record += 56) {
#if defined(__mips__) && !defined(__LP64__)
        u16 kind = *(const u16 *)record;
#else
        u16 kind = (u16)record[0] | (u16)((u16)record[1] << 8);
#endif
        if (kind == 0) {
#if defined(__mips__) && !defined(__LP64__)
            func_80016714((s8 *)record, 56);
#else
            func_80016714_native(record, 56);
#endif
            return record;
        }
    }
    return 0;
}
