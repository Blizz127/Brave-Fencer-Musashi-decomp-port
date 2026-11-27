/* SPDX-License-Identifier: MIT */
/* Room-specific defeat actions take precedence over the checkpoint choices. */
typedef struct {
    short room;
    short action;
} RecoveryRule;

extern RecoveryRule D_8018E9B8[];
extern short D_800B9A08;
extern int func_80029A94(int slot);

int func_80159464(void)
{
    RecoveryRule *rule = D_8018E9B8;
    while (rule->room) {
        if (D_800B9A08 == rule->room)
            return rule->action;
        ++rule;
    }
    if (!func_80029A94(1) && !func_80029A94(0))
        return -1;
    return 0;
}
