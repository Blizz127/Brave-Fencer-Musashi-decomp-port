#include "psx_types.h"

typedef struct {
    u16 class_id;
    u16 state;
} ArenaActor;

typedef struct {
    u16 class_id;
    u16 state;
    u8 unused04[0x0e];
    s16 travel_step;
    u8 unused14[8];
    s32 departure_timer;
    u8 unused20[0x44];
    ArenaActor *boss;
    u8 unused68[0x64];
    ArenaActor *positive_crew[2];
    ArenaActor *negative_crew[2];
    u8 unusedDC[0x22];
    s16 remaining_distance;
} SupplyBucket;

/* Wait for the crew on this side before crossing the cable again. */
void func_801884AC(SupplyBucket *bucket)
{
    ArenaActor *first;
    ArenaActor *second;

    if (bucket->travel_step > 0) {
        first = bucket->positive_crew[0];
        second = bucket->positive_crew[1];
    } else {
        first = bucket->negative_crew[0];
        second = bucket->negative_crew[1];
    }
    if (first->state != 5 && second->state != 5 && bucket->boss->state != 29) {
        bucket->departure_timer--;
    }
    if (bucket->departure_timer == 0) {
        bucket->remaining_distance = 784;
        bucket->state = 2;
        bucket->travel_step *= -1;
    }
}
