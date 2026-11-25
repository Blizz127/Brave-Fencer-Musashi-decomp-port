#include "room_descriptor.h"
#include <assert.h>
#include <stddef.h>
#include <string.h>

_Static_assert(sizeof(RoomTransferDescriptor) == 36, "room descriptor size");
_Static_assert(offsetof(RoomTransferDescriptor, room) == 0, "room ID");
_Static_assert(offsetof(RoomTransferDescriptor, parameter) == 2, "parameter");
_Static_assert(offsetof(RoomTransferDescriptor, world) == 3, "world");
_Static_assert(offsetof(RoomTransferDescriptor, resource) == 4, "resource");
_Static_assert(offsetof(RoomTransferDescriptor, room_value) == 8, "room value");
_Static_assert(offsetof(RoomTransferDescriptor, scene_data) == 12, "scene data");
_Static_assert(offsetof(RoomTransferDescriptor, x) == 16, "x coordinate");
_Static_assert(offsetof(RoomTransferDescriptor, y) == 18, "y coordinate");
_Static_assert(offsetof(RoomTransferDescriptor, z) == 20, "z coordinate");
_Static_assert(offsetof(RoomTransferDescriptor, facing) == 22, "orientation");
_Static_assert(offsetof(RoomTransferDescriptor, activate) == 24, "activate address");
_Static_assert(offsetof(RoomTransferDescriptor, reserved1c) == 28, "reserved word");
_Static_assert(offsetof(RoomTransferDescriptor, finish) == 32, "finish address");

RoomTransferDescriptor D_80078E50, D_80079204;
s32 D_800A5E5C;
static unsigned save_calls, restore_calls;

void func_80029774(s32 enabled)
{
    assert(enabled == 1);
    ++save_calls;
}

void func_8002992C(s32 enabled)
{
    assert(enabled == 1);
    ++restore_calls;
}

int main(void)
{
    RoomTransferDescriptor input;
    unsigned char bytes[sizeof(input)];
    size_t i;
    for (i = 0; i < sizeof(bytes); ++i)
        bytes[i] = (unsigned char)(0xA7u + i * 13u);
    memcpy(&input, bytes, sizeof(input));
    func_800296F8(&input);
    assert(save_calls == 1 && D_800A5E5C == 1);
    assert(memcmp(&D_80079204, bytes, sizeof(bytes)) == 0);
    func_800298BC(&D_80078E50);
    assert(restore_calls == 1);
    assert(memcmp(&D_80078E50, bytes, sizeof(bytes)) == 0);
    return 0;
}
