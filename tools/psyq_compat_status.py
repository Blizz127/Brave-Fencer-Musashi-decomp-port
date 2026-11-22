"""Hand-maintained status of each Psy-Q entry point (read by tools/psyq_callsites.py).

status  "psycross"  PsyCross implements it: pc_port/platform/psyq/psyq_psycross.c
        "hle"       our own C: pc_port/platform/psyq/bfm_psyq_compat.c
        "stub-TBD"  no host version yet: the native lane refuses the PC and the
                    interpreter keeps running the retail code from the user's disc
                    against the device HLE (so a stub is slow, not broken).

INFERRED names addresses the vendor symbol files leave unnamed; each carries its
evidence. Nothing here is Psy-Q code: only names, argument counts and notes.
"""
from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class Status:
    status: str
    argc: int = 0
    returns: int = 0
    wrapper: str | None = None
    note: str = ""


# address -> (name, evidence)
INFERRED: dict[int, tuple[str, str]] = {
    0x8005C324: ("memcpy", "historical API label"),
    0x8005C2C8: ("bzero", "historical API label"),
    0x8005C358: ("memset", "historical API label"),
    0x8005C4CC: ("srand", "historical API label"),
    # The historical strcmp label at 0x8005C4DC has no game-call entry.
    0x8005C388: ("putchar", "Psy-Q object PUTCHAR.o"),
    0x8005C604: ("printf", "Psy-Q object PRNT.o"),
    0x80047CB4: ("InitGeom", "historical API label"),
    0x800599B8: ("LoadImage", "historical API label"),
    0x80059CF4: ("DrawOTag", "historical API label"),
    0x80059D68: ("PutDrawEnv", "historical API label"),
    0x80059FC0: ("PutDispEnv", "historical API label"),
    0x80059BFC: ("ClearOTagR", "historical API label"),
    0x800596F4: ("DrawSync", "historical API label"),
    0x80059658: ("SetDispMask", "called with 1 after the first frame is set up (resident md_MAIN_003)"),
    0x800495EC: ("NormalClip", "Three packed screen points select the signed projected triangle area and update MAC0 and FLAG."),
    0x8004978C: ("RotMatrix", "historical API label"),
    0x8004901C: ("ScaleMatrix", "historical API label"),
    0x800484EC: ("ApplyMatrixLV", "historical API label"),
    0x80048D9C: ("MulMatrix", "historical API label"),
    0x80048EAC: ("MulMatrix2", "historical API label"),
    0x80052460: ("GsSortFastSprite", "historical API label"),
    0x80052D90: ("GsInitCoordinate2", "historical API label"),
    0x80052E38: ("GsSetLsMatrix", "historical API label"),
    0x80053308: ("GsSetProjection", "historical API label"),
    0x80053AF8: ("GsSetAmbient", "historical API label"),
    0x800538EC: ("GsSetLightMode", "historical API label"),
    0x80054514: ("GsGetLw", "historical API label"),
    0x800547D8: ("GsGetLs", "historical API label"),
    0x80054AAC: ("GsGetLws", "historical API label"),
}

H = "hle"
P = "psycross"
S = "stub-TBD"

GPU_PTR = "packet/struct lives in guest RAM"
TABLE: dict[str, Status] = {
    # --- libgpu: packets and environments in guest RAM, rendered via bfm_plat_renderer
    "GetTPage": Status(H, 4, 1, "bfm_psyq_GetTPage", "pure: (tp<<7)|(abr<<5)|(y>>8<<4)|(x>>6)"),
    "GetClut": Status(H, 2, 1, "bfm_psyq_GetClut", "pure: (y<<6)|(x>>4)"),
    "AddPrim": Status(H, 2, 0, "bfm_psyq_AddPrim", GPU_PTR + "; 24-bit guest links"),
    "CatPrim": Status(H, 2, 0, "bfm_psyq_CatPrim", GPU_PTR),
    "SetSemiTrans": Status(H, 2, 0, "bfm_psyq_SetSemiTrans", GPU_PTR),
    "SetPolyF3": Status(H, 1, 0, "bfm_psyq_SetPolyF3", GPU_PTR + "; header len/code"),
    "SetPolyFT3": Status(H, 1, 0, "bfm_psyq_SetPolyFT3", GPU_PTR),
    "SetPolyG3": Status(H, 1, 0, "bfm_psyq_SetPolyG3", GPU_PTR),
    "SetPolyGT3": Status(H, 1, 0, "bfm_psyq_SetPolyGT3", GPU_PTR),
    "SetPolyF4": Status(H, 1, 0, "bfm_psyq_SetPolyF4", GPU_PTR),
    "SetPolyFT4": Status(H, 1, 0, "bfm_psyq_SetPolyFT4", GPU_PTR),
    "SetPolyG4": Status(H, 1, 0, "bfm_psyq_SetPolyG4", GPU_PTR),
    "SetPolyGT4": Status(H, 1, 0, "bfm_psyq_SetPolyGT4", GPU_PTR),
    "SetLineF2": Status(H, 1, 0, "bfm_psyq_SetLineF2", GPU_PTR),
    "SetLineG2": Status(H, 1, 0, "bfm_psyq_SetLineG2", GPU_PTR),
    "SetLineG3": Status(S, note="executes from the supplied disc"),
    "SetLineG4": Status(S, note="executes from the supplied disc"),
    "SetDefDispEnv": Status(H, 5, 1, "bfm_psyq_SetDefDispEnv", "fills a guest DISPENV; returns the guest pointer; 5th arg from guest stack"),
    "ClearOTagR": Status(S, note="executes from the supplied disc"),
    "LoadImage": Status(H, 2, 1, "bfm_psyq_LoadImage", "guest RECT + pixels -> bfm_plat_renderer_upload_vram (texture replacement applies)"),
    "StoreImage": Status(H, 2, 1, "bfm_psyq_StoreImage", "bfm_plat_renderer_download_vram into guest RAM"),
    "MoveImage": Status(H, 3, 1, "bfm_psyq_MoveImage", "download + upload through the renderer"),
    "DrawSync": Status(H, 1, 1, "bfm_psyq_DrawSync", "host rendering is synchronous: returns 0"),
    "SetDispMask": Status(H, 1, 0, "bfm_psyq_SetDispMask", "no-op on the host (display always on)"),
    "PutDrawEnv": Status(H, 1, 1, "bfm_psyq_PutDrawEnv", "guest DRAWENV -> BfmPlatDrawEnv; returns the guest pointer"),
    "PutDispEnv": Status(H, 1, 1, "bfm_psyq_PutDispEnv", "guest DISPENV -> BfmPlatDispEnv; returns the guest pointer"),
    "DrawOTag": Status(H, 1, 0, "bfm_psyq_DrawOTag", "walks the guest OT (24-bit links) and decodes GP0 packets to BfmPlatPrim; unknown codes skipped and counted"),
    "SetDrawEnv": Status(S, 0, 0, None, "builds a DR_ENV packet in guest RAM (TBD)"),
    "DrawSyncCallback": Status(S, 0, 0, None, "callback into guest code: needs the lane's guest-call thunk"),
    # libgs
    "GsSortLine": Status(S, note="executes from the supplied disc"),
    "GsSortFastSprite": Status(S, note="executes from the supplied disc"),
    "GsInitCoordinate2": Status(S, note="executes from the supplied disc"),
    "GsMapModelingData": Status(S, note="executes from the supplied disc"),
    "GsSetLightMode": Status(S, note="executes from the supplied disc"),
    "GsSetLsMatrix": Status(S, note="executes from the supplied disc"),
    "GsSetProjection": Status(S, note="executes from the supplied disc"),
    "GsSetAmbient": Status(S, note="executes from the supplied disc"),
    "SetBackColor": Status(S, note="executes from the supplied disc"),
    "GsSortSprite": Status(S, note="executes from the supplied disc"),
    "GsSortBg": Status(S, 0, 0, None, "GsBG cell-map walker with scaling and rotation: large"),
    "GsSortFastBg": Status(S, 0, 0, None, "GsBG cell-map walker (unscaled): moderate; next"),
    "GsGetLw": Status(S, note="executes from the supplied disc"),
    "GsGetLs": Status(S, note="executes from the supplied disc"),
    "GsGetLws": Status(S, note="executes from the supplied disc"),
    "GsSetRefView2L": Status(S, 0, 0, None, "view matrix from GsRVIEW2 (ratan2, rsin/rcos, matrix products): large"),
    "GsLinkObject5": Status(S, 0, 0, None, "TMD object packet preset (GsDOBJ5): large; 2 call sites"),
    "GsPresetObject": Status(S, 0, 0, None, "TMD object packet preset: large; 2 call sites"),
    "GsInitGraph2": Status(S, 0, 0, None, "display/draw init over libgs globals and libgpu: low call count"),
    "GsSetFogParam": Status(S, 0, 0, None, "fog: 1 call site"),
    # --- libetc
    "VSync": Status(H, 1, 1, "bfm_psyq_VSync", "n>1 waits n fields via bfm_plat_timing_vsync; n=0 one field; n=1 returns 0 (no hcount); n<0 returns the field counter"),
    "ResetCallback": Status(S, 0, 0, None, "interrupt callbacks belong to the IRQ HLE"),
    "DMACallback": Status(S, 0, 0, None, "interrupt callbacks belong to the IRQ HLE"),
    "StopCallback": Status(S, 0, 0, None, "interrupt callbacks belong to the IRQ HLE"),
    # --- libcd
    "CdIntToPos": Status(S, note="executes from the supplied disc"),
    "CdPosToInt": Status(H, 1, 1, "bfm_psyq_CdPosToInt", "pure: BCD CdlLOC -> LBA"),
    "CdSearchFile": Status(H, 2, 1, "bfm_psyq_CdSearchFile", "bfm_plat_disc_find_file -> guest CdlFILE {pos, size, name}; returns the pointer or 0"),
    # --- libgte: only stateless math (PsyCross's GTE has its own register file)
    "ratan2": Status(S, note="executes from the supplied disc"),
    "csqrt": Status(S, 0, 0, None, "stateless but not in PsyCross; fixed-point semantics to pin before an HLE"),
    "catan": Status(S, 0, 0, None, "stateless but not in PsyCross"),
    "SquareRoot12": Status(S, 0, 0, None, "stateless but not in PsyCross"),
    # GTE state through the bridge: PsyCross works on the gteRegs bank gte_owner leases
    "InitGeom": Status(P, 0, 0, "bfm_psyq_InitGeom", "GTE bridge (gte_owner lease)"),
    "SetGeomOffset": Status(S, note="executes from the supplied disc"),
    "SetGeomScreen": Status(S, note="executes from the supplied disc"),
    "ReadGeomOffset": Status(P, 2, 0, "bfm_psyq_ReadGeomOffset", "GTE bridge (gte_owner lease); reads C2_OFX/OFY"),
    "SetRotMatrix": Status(P, 1, 0, "bfm_psyq_SetRotMatrix", "GTE bridge (gte_owner lease)"),
    "SetTransMatrix": Status(P, 1, 0, "bfm_psyq_SetTransMatrix", "GTE bridge (gte_owner lease)"),
    "SetLightMatrix": Status(P, 1, 0, "bfm_psyq_SetLightMatrix", "GTE bridge (gte_owner lease)"),
    "ReadRotMatrix": Status(P, 1, 1, "bfm_psyq_ReadRotMatrix", "GTE bridge (gte_owner lease)"),
    "MulMatrix0": Status(P, 3, 1, "bfm_psyq_MulMatrix0", "GTE bridge (gte_owner lease); clobbers the rotation matrix like Psy-Q"),
    "MulRotMatrix": Status(P, 1, 1, "bfm_psyq_MulRotMatrix", "GTE bridge (gte_owner lease)"),
    "CompMatrix": Status(P, 3, 1, "bfm_psyq_CompMatrix", "GTE bridge (gte_owner lease)"),
    "CompMatrixLV": Status(P, 3, 1, "bfm_psyq_CompMatrixLV", "GTE bridge (gte_owner lease)"),
    "ApplyRotMatrix": Status(P, 2, 1, "bfm_psyq_ApplyRotMatrix", "GTE bridge (gte_owner lease)"),
    "ApplyRotMatrixLV": Status(P, 2, 1, "bfm_psyq_ApplyRotMatrixLV", "GTE bridge (gte_owner lease)"),
    "ApplyMatrixSV": Status(P, 3, 1, "bfm_psyq_ApplyMatrixSV", "GTE bridge (gte_owner lease); sets the rotation matrix like Psy-Q"),
    "RotTransSV": Status(P, 3, 0, "bfm_psyq_RotTransSV", "GTE bridge (gte_owner lease); long *flag via a host temporary"),
    "RotTransPers": Status(P, 4, 1, "bfm_psyq_RotTransPers", "GTE bridge (gte_owner lease); long *p/*flag via host temporaries"),
    "NormalClip": Status(P, 3, 1, "bfm_psyq_NormalClip", "GTE bridge computes the signed area of three packed screen points"),
    "RotTransPers3": Status(P, 8, 1, "bfm_psyq_RotTransPers3", "GTE bridge (gte_owner lease); args 5-8 from the guest stack; long * via temporaries"),
    "RotTransPers4": Status(P, 10, 1, "bfm_psyq_RotTransPers4", "GTE bridge (gte_owner lease); args 5-10 from the guest stack; long * via temporaries"),
    "RotMatrixYXZ": Status(S, note="executes from the supplied disc"),
    "RotMatrixX": Status(S, note="executes from the supplied disc"),
    "RotMatrixY": Status(S, note="executes from the supplied disc"),
    "RotMatrixZ": Status(S, note="executes from the supplied disc"),
    "PushMatrix": Status(S, note="executes from the supplied disc"),
    "PopMatrix": Status(S, note="executes from the supplied disc"),
    "ApplyMatrixLV": Status(P, 3, 1, "bfm_psyq_ApplyMatrixLV", "GTE bridge; PsyCross's Psy-Q hi/lo split"),
    "MulMatrix": Status(S, note="executes from the supplied disc"),
    "MulMatrix2": Status(S, note="executes from the supplied disc"),
    "ReadGeomScreen": Status(S, note="executes from the supplied disc"),
    "RotMatrix": Status(S, note="executes from the supplied disc"),
    "ScaleMatrix": Status(S, note="executes from the supplied disc"),
    # --- libc2
    "rand": Status(S, note="executes from the supplied disc"),
    "strcpy": Status(S, note="executes from the supplied disc"),
    "srand": Status(S, note="executes from the supplied disc"),
    "memcpy": Status(S, note="executes from the supplied disc"),
    "memset": Status(S, note="executes from the supplied disc"),
    "bzero": Status(S, note="executes from the supplied disc"),
    "strcmp": Status(S, note="executes from the supplied disc"),
    "putchar": Status(H, 1, 1, "bfm_psyq_putchar", "to the platform log (line-buffered)"),
    "printf": Status(S, 0, 0, None, "guest varargs + format in guest RAM (TBD)"),
}

SEGMENT_NOTES = {
    "snd": "sound driver: runs as guest code over the SPU HLE",
    "libgs": "libgs internal (TMD object sort/preset, BG and clip setup): large or rarely called; guest code for now",
    "libgte": "not in PsyCross; an HLE on the gteRegs bank via the bridge is possible",
    "libgpu": "libgpu internal",
    "libcd": "CD: guest code over the CD controller HLE",
    "libetc": "libetc",
    "libapi": "BIOS calls: the BIOS HLE handles them",
    "libpad": "pad: guest code over the SIO HLE",
    "libmcrd": "memory card: guest code over the card HLE",
    "apicard": "memory card BIOS: card HLE",
    "libc2": "libc",
}


def lookup(name: str, segment: str | None) -> Status:
    if name in TABLE:
        return TABLE[name]
    for prefix, note in SEGMENT_NOTES.items():
        if segment and segment.startswith(prefix):
            return Status(S, note=note)
    return Status(S, note="not reviewed")
