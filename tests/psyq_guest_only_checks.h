/* Unavailable host entries leave the supplied game state ready for guest execution. */
#ifndef PSYQ_GUEST_ONLY_CHECKS_H
#define PSYQ_GUEST_ONLY_CHECKS_H

static void check_guest_only_services(const uint8_t *guest_ram, size_t size)
{
    static const struct {
        uint32_t pc;
        const char *name;
    } services[] = {
        {0x80043A18u, "CdIntToPos"},
        {0x8004867Cu, "PushMatrix"},
        {0x8004871Cu, "PopMatrix"},
        {0x80048D9Cu, "MulMatrix"},
        {0x80048EACu, "MulMatrix2"},
        {0x8004901Cu, "ScaleMatrix"},
        {0x800491ECu, "ReadGeomScreen"},
        {0x800491FCu, "SetBackColor"},
        {0x8004921Cu, "SetGeomOffset"},
        {0x8004923Cu, "SetGeomScreen"},
        {0x8004978Cu, "RotMatrix"},
        {0x80049A1Cu, "RotMatrixYXZ"},
        {0x80049F3Cu, "RotMatrixX"},
        {0x8004A0DCu, "RotMatrixY"},
        {0x8004A27Cu, "RotMatrixZ"},
        {0x8004CFECu, "ratan2"},
        {0x80051DC8u, "GsSortLine"},
        {0x80051F34u, "GsSortSprite"},
        {0x80052460u, "GsSortFastSprite"},
        {0x80052D90u, "GsInitCoordinate2"},
        {0x80052E38u, "GsSetLsMatrix"},
        {0x80053290u, "GsMapModelingData"},
        {0x80053308u, "GsSetProjection"},
        {0x800538ECu, "GsSetLightMode"},
        {0x80053AF8u, "GsSetAmbient"},
        {0x80054514u, "GsGetLw"},
        {0x800547D8u, "GsGetLs"},
        {0x80054AACu, "GsGetLws"},
        {0x80058EE4u, "SetLineG3"},
        {0x80058F24u, "SetLineG4"},
        {0x80059BFCu, "ClearOTagR"},
        {0x8005C2C8u, "bzero"},
        {0x8005C324u, "memcpy"},
        {0x8005C358u, "memset"},
        {0x8005C49Cu, "rand"},
        {0x8005C4CCu, "srand"},
        {0x8005C540u, "strcpy"},
    };
    uint32_t registers[32], saved[32];
    uint8_t *original = malloc(size);
    size_t i;

    CHECK(original != NULL);
    memcpy(original, guest_ram, size);
    for (i = 0; i < 32; ++i)
        registers[i] = 0xC1000000u + (uint32_t)i;
    registers[0] = 0;
    registers[4] = 0x80002000u;
    registers[5] = 0x80003000u;
    registers[6] = 0x80004000u;
    registers[29] = 0x801FE000u;
    memcpy(saved, registers, sizeof(saved));

    for (i = 0; i < sizeof(services) / sizeof(services[0]); ++i) {
        const BfmPsyqEntry *entry = bfm_psyq_find(services[i].pc);
        CHECK(entry && strcmp(entry->name, services[i].name) == 0);
        CHECK(entry->status == BFM_PSYQ_STUB && !entry->available);
        CHECK(!bfm_psyq_call(services[i].pc, registers));
        CHECK(memcmp(saved, registers, sizeof(saved)) == 0);
        CHECK(memcmp(original, guest_ram, size) == 0);
    }
    /* The former standalone strcmp helper never had a public table row. */
    CHECK(bfm_psyq_find(0x8005C4DCu) == NULL);
    CHECK(!bfm_psyq_call(0x8005C4DCu, registers));
    CHECK(memcmp(saved, registers, sizeof(saved)) == 0);
    CHECK(memcmp(original, guest_ram, size) == 0);
    free(original);
}

#endif
