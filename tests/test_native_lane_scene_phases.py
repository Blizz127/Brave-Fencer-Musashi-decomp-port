"""The countdown and fade callbacks still need the guest bus and event timing."""
import hashlib
from pathlib import Path
import shutil
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools"))
import native_lane_gen as lane


PHASES = (
    ("sc02_0005", 0x80184358, 172),
    ("sc02_0005", 0x80185A04, 132),
    ("sc02_0005", 0x80185A88, 176),
    ("sc02_0005", 0x801869E0, 36),
    ("sc02_0005", 0x80189CCC, 60),
    ("sc02_0005", 0x8018BA38, 172),
    ("sc02_0005", 0x8018BAE4, 116),
    ("sc02_0005", 0x80187664, 76),
    ("sc02_0005", 0x8018B16C, 368),
    ("sc02_0005", 0x80185060, 384),
    ("sc02_0005", 0x80186B08, 176),
    ("sc02_0005", 0x80187280, 200),
    ("sc02_0005", 0x8016AA50, 224),
    ("sc02_0005", 0x80146A6C, 72),
    ("sc02_0005", 0x80146AFC, 160),
    ("sc02_0005", 0x80186A04, 216),
    ("sc02_0005", 0x8014C4AC, 188),
    ("sc01_0077", 0x8012F14C, 88),
    ("sc02_0005", 0x801853EC, 160),
    ("sc02_0039", 0x80165CA0, 396),
    ("sc02_0039", 0x8013BC7C, 96),
    ("sc02_0039", 0x8013BCDC, 88),
    ("sc02_0039", 0x80129010, 204),
    ("sc02_0039", 0x8013A448, 124),
    ("sc02_0039", 0x8013A4C4, 108),
    ("sc02_0039", 0x801379FC, 388),
    ("sc02_0039", 0x8013A380, 200),
    ("sc02_0039", 0x80165E90, 276),
    ("sc02_0039", 0x8015094C, 292),
    ("sc02_0039", 0x8013A530, 816),
    ("sc02_0039", 0x8012AAAC, 500),
    ("sc02_0039", 0x8012A988, 292),
    ("sc02_0039", 0x8012ACE0, 100),
    ("sc02_0039", 0x80167AE0, 732),
    ("sc02_0039", 0x801299C8, 632),
    ("sc02_0039", 0x801287B8, 248),
    ("sc02_0039", 0x80135D20, 400),
    ("sc01_0077", 0x80135D20, 400),
    ("sc01_0077", 0x8015AD3C, 60),
    ("sc02_0035", 0x80146CA0, 20),
    ("sc02_0035", 0x8014900C, 20),
    ("main_0012", 0x8014900C, 20),
    ("sc01_0077", 0x80150150, 32),
    ("main_0012", 0x801287B8, 248),
    ("main_0012", 0x8014A2E4, 156),
    ("main_0012", 0x8014C278, 56),
    ("main_0012", 0x8014FDF4, 108),
    ("main_0012", 0x80150150, 32),
    ("main_0012", 0x80150460, 32),
    ("main_0012", 0x801505FC, 168),
    ("sc02_0039", 0x801506A4, 380),
    ("main_0012", 0x801508B4, 68),
    ("sc02_0039", 0x8016EE40, 84),
    ("sc02_0039", 0x8017CDD8, 68),
    ("sc02_0039", 0x8017CE1C, 36),
    ("sc02_0039", 0x8017CE40, 64),
    ("sc02_0039", 0x8017CEFC, 32),
    ("sc02_0039", 0x8017CF58, 116),
    ("sc02_0039", 0x8017CFCC, 40),
    ("sc02_0039", 0x8017CFF4, 40),
    ("main_0010", 0x800D1EBC, 156),
)


@pytest.mark.parametrize("member,pc,size", PHASES)
def test_phase_source_stays_interpreted(member, pc, size, tmp_path, monkeypatch):
    # The forest floor wrapper uses the identical registered MAIN source.
    source_member = "main_0012" if (member, pc) == ("sc01_0077", 0x80150150) else member
    source = ROOT / "src/overlays" / source_member / f"{pc:08x}.c"
    row = dict(name=f"func_{pc:08X}", vram=pc, size=size, source=str(source), region=member)
    (tmp_path / "ir").mkdir()

    def check(entry, info=None):
        return lane.build_one(entry, tmp_path, {}, {}, "clang", info)

    assert check(row).get("rule") == "timing"
    copy = tmp_path / f"{pc:08x}.c"
    copy.write_bytes(source.read_bytes())
    alternate = dict(row, region="sc99_0001", source=str(copy))
    assert check(alternate).get("rule") == "timing"
    copy.write_text(f"int {row['name']}(int x) {{ return x + 1; }}\n")
    assert check(dict(alternate, region=member)).get("rule") == "timing"
    assert check(alternate, dict(member=member)).get("rule") == "timing"

    # Model another member with exactly the same complete guest body at this PC.
    data = bytes(size)
    info = dict(member="sc99_0001", vram=pc, start=0, code=[(pc, pc + size)], data=data)
    monkeypatch.setitem(lane.SCENE_PHASE_EXTENTS[pc], member, (size, lane.fnv1a64(data)))
    assert check(alternate, info).get("rule") == "timing"
    if not all(shutil.which(tool) for tool in ("clang", "nm")):
        pytest.skip("clang and nm required for the independent-source control")
    assert check(alternate, dict(info, data=b"\x01" + data[1:]))["admitted"]
    assert check(alternate)["admitted"]


@pytest.mark.skipif(not shutil.which("clang"), reason="clang required to inspect the dispatcher")
@pytest.mark.parametrize("pc", (0x8017CD9C, 0x8017CF1C))
def test_phase_dispatch_rejects_host_function_table(pc, tmp_path):
    source = ROOT / "src/overlays/sc02_0039" / f"{pc:08x}.c"
    row = dict(name=f"func_{pc:08X}", vram=pc, size=60, source=str(source),
               region="sc02_0039")
    (tmp_path / "ir").mkdir()
    result = lane.build_one(row, tmp_path, {}, {}, "clang")
    assert result.get("rule") == "layout"
    assert result.get("detail") == "array of pointers"

@pytest.mark.parametrize("pc, size", [
    (0x8012A908, 128), (0x80162B1C, 424), (0x8017849C, 364),
    (0x8017869C, 168), (0x801787D4, 108),
])
def test_mine_slot_dispatch_preserves_the_main_member(tmp_path, pc, size):
    mine = ROOT / f"src/overlays/sc02_0039/{pc:08x}.c"
    main = ROOT / f"src/overlays/main_0012/{pc:08x}.c"
    row = dict(region="sc02_0039", vram=pc, size=size)
    copy = tmp_path / "copy.c"
    copy.write_bytes(mine.read_bytes())
    assert lane.mine_slot_update_source_only(row, main, None)
    assert lane.mine_slot_update_source_only(dict(row, region="sc99_0001"), copy, None)
    assert lane.mine_slot_update_source_only(dict(row, region="sc99_0001"), main,
                                             dict(member="sc02_0039"))
    assert not lane.mine_slot_update_source_only(dict(row, region="main_0012"), main,
                                                 dict(member="main_0012"))
    assert not lane.mine_slot_update_source_only(dict(row, region="sc99_0001"), main, None)



@pytest.mark.parametrize("pc,size", [
    (0x8012C890, 596), (0x801336E8, 84), (0x80136BC4, 88), (0x801457A4, 316),
    (0x8017DDB0, 80), (0x8017DE00, 132), (0x8017E25C, 152),
    (0x801627E8, 76), (0x8016E95C, 144), (0x801754A8, 148),
    (0x8012956C, 904),
    (0x8016E8F0, 40), (0x8016E9EC, 212), (0x8016EC0C, 352),
    (0x8016E918, 68), (0x8016EAC0, 124), (0x8016EB3C, 108), (0x8016EBA8, 100),
    (0x8017E2F4, 308), (0x8017E428, 216), (0x8017E500, 320),
])
def test_mine_model_actions_stay_with_their_member(tmp_path, pc, size):
    source = ROOT / f"src/overlays/sc02_0039/{pc:08x}.c"
    (tmp_path / "ir").mkdir()
    row = dict(name=f"func_{pc:08X}", region="sc02_0039", vram=pc, size=size, source=str(source))
    assert lane.build_one(row, tmp_path, {}, {}, "clang").get("rule") == "timing"
    copy = tmp_path / "update.c"
    copy.write_bytes(source.read_bytes())
    assert lane.build_one(dict(row, region="other", source=str(copy)),
                          tmp_path, {}, {}, "clang").get("rule") == "timing"
    copy.write_text(f"int func_{pc:08X}(int value) {{ return value; }}\n")
    assert lane.build_one(dict(row, region="other", source=str(copy)), tmp_path, {}, {}, "clang",
                          dict(member="sc02_0039")).get("rule") == "timing"
    assert not lane.mine_member_source_only(dict(row, region="other"), copy, dict(member="other"))


def test_mine_slot_guard_preserves_other_regional_initializers():
    for region in ("main_0012", "sc01_0000", "sc02_0031", "sc02_0035"):
        source = ROOT / f"src/overlays/{region}/8012c890.c"
        row = dict(vram=0x8012C890, region=region)
        assert not lane.mine_member_source_only(row, source, dict(member=region))


def test_mine_grid_guard_preserves_other_relocation_sources():
    for region in ("main_0012", "sc02_0035"):
        source = ROOT / f"src/overlays/{region}/80136bc4.c"
        row = dict(vram=0x80136BC4, region=region)
        assert not lane.mine_member_source_only(row, source, dict(member=region))


def test_mine_frame_guard_preserves_main_state_sources():
    for pc in (0x801627E8, 0x801754A8):
        source = ROOT / f"src/overlays/main_0012/{pc:08x}.c"
        row = dict(vram=pc, region="main_0012")
        assert not lane.mine_member_source_only(row, source, dict(member="main_0012"))


def test_mine_backdrop_guard_preserves_main_source():
    for pc in (0x8012956C,):
        source = ROOT / f"src/overlays/main_0012/{pc:08x}.c"
        row = dict(vram=pc, region="main_0012")
        assert not lane.mine_member_source_only(row, source, dict(member="main_0012"))


def test_mine_hud_motion_guard_preserves_other_regions():
    for pc, region in ((0x8016E918, "main_0012"), (0x8016EAC0, "sc02_0031"),
                       (0x8016EB3C, "sc02_0031"), (0x8016EBA8, "sc02_0031")):
        source = ROOT / f"src/overlays/{region}/{pc:08x}.c"
        row = dict(vram=pc, region=region)
        assert not lane.mine_member_source_only(row, source, dict(member=region))


def test_mine_descriptor_guard_preserves_main_lookup():
    source = ROOT / "src/overlays/main_0012/801457a4.c"
    row = dict(vram=0x801457A4, region="main_0012")
    assert not lane.mine_member_source_only(row, source, dict(member="main_0012"))


def test_mine_rotor_update_stays_with_its_member(tmp_path):
    pc, size = 0x8017DC7C, 308
    source = ROOT / f"src/overlays/sc02_0039/{pc:08x}.c"
    (tmp_path / "ir").mkdir()
    row = dict(name=f"func_{pc:08X}", region="sc02_0039", vram=pc, size=size, source=str(source))
    assert lane.build_one(row, tmp_path, {}, {}, "clang").get("rule") == "timing"
    copy = tmp_path / "update.c"
    copy.write_bytes(source.read_bytes())
    assert lane.build_one(dict(row, region="other", source=str(copy)),
                          tmp_path, {}, {}, "clang").get("rule") == "timing"
    copy.write_text(f"int func_{pc:08X}(int value) {{ return value; }}\n")
    assert lane.build_one(dict(row, region="other", source=str(copy)), tmp_path, {}, {}, "clang",
                          dict(member="sc02_0039")).get("rule") == "timing"
    assert not lane.mine_member_source_only(dict(row, region="other"), copy, dict(member="other"))


def test_mine_constructor_preserves_forest_dispatch(tmp_path):
    pc = 0x8017DE84
    source = ROOT / "src/overlays/sc02_0039/8017de84.c"
    forest = ROOT / "src/overlays/sc01_0077/8017de84.c"
    (tmp_path / "ir").mkdir()
    row = dict(name="func_8017DE84", region="sc02_0039", vram=pc, size=356, source=str(source))
    assert lane.build_one(row, tmp_path, {}, {}, "clang").get("rule") == "timing"
    copy = tmp_path / "constructor.c"
    copy.write_bytes(source.read_bytes())
    assert lane.build_one(dict(row, region="other", source=str(copy)),
                          tmp_path, {}, {}, "clang").get("rule") == "timing"
    copy.write_text("int func_8017DE84(int value) { return value; }\n")
    assert lane.build_one(dict(row, region="other", source=str(copy)), tmp_path, {}, {}, "clang",
                          dict(member="sc02_0039")).get("rule") == "timing"
    forest_row = dict(row, region="sc01_0077", size=60, source=str(forest))
    assert not lane.mine_member_source_only(forest_row, forest, dict(member="sc01_0077"))


def test_mine_collision_actor_selection_stays_with_its_member(tmp_path):
    pc = 0x8017DFE8
    (tmp_path / "ir").mkdir()
    source = tmp_path / "actor.c"
    row = dict(name="func_8017DFE8", vram=pc, size=60, region="sc02_0039", source=str(source))
    source.write_bytes((ROOT / "src/overlays/sc02_0039/8017dfe8.c").read_bytes())
    assert lane.build_one(dict(row, region="main_0012"), tmp_path, {}, {}, "clang").get("rule") == "timing"
    source.write_text("int func_8017DFE8(int x) { return x + 1; }\n")
    assert lane.build_one(row, tmp_path, {}, {}, "clang").get("rule") == "timing"
    assert lane.build_one(dict(row, region="other"), tmp_path, {}, {}, "clang",
                          dict(member="sc02_0039")).get("rule") == "timing"
    assert not lane.mine_member_source_only(dict(row, region="other"), source, dict(member="other"))


def test_mine_camera_preserves_the_independent_main_provider(tmp_path):
    pc = 0x80129CF8
    (tmp_path / "ir").mkdir()
    src = tmp_path / "camera.c"
    row = dict(name="func_80129CF8", vram=pc, size=764, region="sc02_0039", source=str(src))
    mine = ROOT / "src/overlays/sc02_0039/80129cf8.c"
    main = ROOT / "src/overlays/main_0012/80129cf8.c"
    src.write_bytes(mine.read_bytes())
    assert lane.build_one(dict(row, region="main_0012"), tmp_path, {}, {}, "clang").get("rule") == "timing"
    assert not lane.mine_member_source_only(dict(row, region="main_0012"), main, dict(member="main_0012"))
    src.write_text("int func_80129CF8(int x) { return x + 1; }\n")
    assert lane.build_one(row, tmp_path, {}, {}, "clang").get("rule") == "timing"
    assert lane.build_one(dict(row, region="sc99_0001"), tmp_path, {}, {}, "clang",
                          dict(member="sc02_0039")).get("rule") == "timing"
    if shutil.which("clang") and shutil.which("nm"):
        assert lane.build_one(dict(row, region="main_0012"), tmp_path, {}, {}, "clang")["admitted"]


@pytest.mark.parametrize("pc", (0x8015AD78, 0x8015ADB0, 0x80161104,
                                0x80161208, 0x801612B8, 0x8016130C))
def test_forest_action_uses_its_member_and_source(pc, tmp_path, monkeypatch):
    import hashlib

    selected = ROOT / f"src/overlays/sc01_0077/{pc:08x}.c"
    copy = tmp_path / "action.c"
    copy.write_bytes(selected.read_bytes())
    other = dict(region="sc99_0001", vram=pc)
    assert lane.forest_action_source_only(other, copy, None)

    copy.write_text("int independent(int x) { return x + 1; }\n")
    assert lane.forest_action_source_only(dict(other, region="sc01_0077"), copy, None)
    assert lane.forest_action_source_only(other, copy, dict(member="sc01_0077"))
    assert not lane.forest_action_source_only(other, copy, None)
    assert not lane.forest_action_source_only(other, copy, dict(member="main_0012"))

    # Model an older copy whose source identity is known, without needing the game.
    monkeypatch.setitem(lane.FOREST_ACTION_EARLIER_SOURCES, pc,
                        hashlib.sha256(copy.read_bytes()).hexdigest())
    assert lane.forest_action_source_only(other, copy, None)
    is_older_forest = lane.forest_action_source_only(dict(other, region="main_0012"), copy, None)
    assert is_older_forest == (pc != 0x801612B8)
    named_main = dict(other, region="main_0012")
    assert lane.forest_action_source_only(named_main, copy, dict(member="sc99_0001"))
    verified_main = lane.forest_action_source_only(other, copy, dict(member="main_0012"))
    assert verified_main == (pc != 0x801612B8)


@pytest.mark.parametrize("pc", (0x8015444C, 0x801549F8))
def test_animation_commands_hold_relocated_code_and_source_copies(pc, tmp_path, monkeypatch):
    size, high, low, _ = lane.FOREST_ANIMATION_EXTENTS[pc]
    row = dict(vram=pc, size=8, region="another_member")
    source = tmp_path / "scalar.c"
    source.write_text(f"int func_{pc:08X}(int value) {{ return value + 1; }}\n")
    assert not lane.forest_animation_source_only(row, source, None)
    assert lane.forest_animation_source_only(dict(row, region="sc01_0077"), source, None)
    assert lane.forest_animation_source_only(dict(row, region="main_0012"), source, None)

    original = bytearray((n * 11) & 255 for n in range(size))
    original[high:high + 2] = original[low:low + 2] = b"\0\0"
    digest = hashlib.sha256(original).hexdigest()
    monkeypatch.setitem(lane.FOREST_ANIMATION_EXTENTS, pc, (size, high, low, digest))
    relocated = bytearray(original)
    relocated[high:high + 2] = b"\x01\x02"
    relocated[low:low + 2] = b"\x03\x04"
    info = dict(member="another_member", vram=pc - 8, start=0, data=bytes(8) + relocated)
    assert lane.forest_animation_source_only(row, source, info)
    relocated[high + 2] ^= 1
    assert not lane.forest_animation_source_only(row, source, dict(info, data=bytes(8) + relocated))

    if pc == 0x8015444C:
        blob = source.read_bytes()
        digest = hashlib.sha1(f"blob {len(blob)}\0".encode("ascii") + blob).hexdigest()
        monkeypatch.setattr(lane, "FOREST_ANIMATION_PRIOR_BLOBS", {digest})
        assert lane.forest_animation_source_only(row, source, None)
        source.write_text(source.read_text() + "\n")
        assert not lane.forest_animation_source_only(row, source, None)
    source.write_bytes((ROOT / f"src/overlays/sc01_0077/{pc:08x}.c").read_bytes())
    assert lane.forest_animation_source_only(row, source, None)

@pytest.mark.parametrize("pc,size,source_member", [
    (0x8012C724, 44, "sc02_0035"),
    (0x8013E298, 44, "main_0012"),
    (0x8013E370, 160, "main_0012"),
    (0x8013E410, 56, "main_0012"),
    (0x8013E448, 108, "main_0012"),
])
def test_forest_placement_aliases_stay_interpreted(tmp_path, pc, size, source_member):
    source = ROOT / "src" / "overlays" / source_member / f"{pc:08x}.c"
    row = dict(name=f"func_{pc:08X}", vram=pc, size=size, source=str(source))
    (tmp_path / "ir").mkdir()
    for region, info in (("sc01_0077", None), (source_member, dict(member="sc01_0077"))):
        result = lane.build_one(dict(row, region=region), tmp_path, {}, {}, "clang", info)
        assert result.get("rule") == "timing"


@pytest.mark.skipif(not shutil.which("clang"), reason="clang required for native selection")
def test_forest_image_moves_preserve_existing_member_selection(tmp_path):
    pc = 0x80146014
    forest = ROOT / "src/overlays/sc01_0077/80146014.c"
    published = ROOT / "src/overlays/sc02_0035/80146014.c"
    assert forest.read_bytes() == published.read_bytes()
    (tmp_path / "ir").mkdir()

    def decision(source, region, member):
        row = dict(name="func_80146014", region=region, vram=pc, size=96, source=str(source))
        info = dict(member=member, vram=pc, start=0, code=[(pc, pc + 96)], data=bytes(96))
        return lane.build_one(row, tmp_path, {}, {}, "clang", info)

    assert decision(published, "sc01_0077", "sc02_0035").get("rule") == "timing"
    assert decision(published, "sc02_0035", "sc01_0077").get("rule") == "timing"
    assert decision(forest, "other", "other").get("rule") == "timing"
    for member in ("sc02_0035", "main_0012"):
        assert decision(published, member, member)["admitted"]


@pytest.mark.parametrize("pc,size", [(0x8017EA98, 40), (0x8017EAC0, 108), (0x8017EC40, 60)])
def test_forest_settle_effects_stay_interpreted(tmp_path, pc, size):
    source = ROOT / "src" / "overlays" / "sc01_0077" / f"{pc:08x}.c"
    row = dict(name=f"func_{pc:08X}", vram=pc, size=size, source=str(source))
    (tmp_path / "ir").mkdir()
    for region, info in (("sc01_0077", None), ("other", dict(member="sc01_0077"))):
        result = lane.build_one(dict(row, region=region), tmp_path, {}, {}, "clang", info)
        assert result.get("rule") == "timing"


@pytest.mark.parametrize("pc,size", [(0x8017E1D4, 60), (0x8017E460, 84), (0x8017F240, 80)])
def test_forest_tutorial_soldier_stay_interpreted(tmp_path, pc, size):
    source = ROOT / "src" / "overlays" / "sc01_0077" / f"{pc:08x}.c"
    row = dict(name=f"func_{pc:08X}", vram=pc, size=size, source=str(source))
    (tmp_path / "ir").mkdir()
    for region, info in (("sc01_0077", None), ("other", dict(member="sc01_0077"))):
        result = lane.build_one(dict(row, region=region), tmp_path, {}, {}, "clang", info)
        assert result.get("rule") == "timing"


def test_forest_ability_callback_stays_guest_timed(tmp_path):
    source = ROOT / "src/overlays/sc01_0077/80165ac8.c"
    row = dict(name="func_80165AC8", vram=0x80165AC8, size=64, source=str(source))
    (tmp_path / "ir").mkdir()
    for region, info in (("sc01_0077", None), ("sc02_0031", {"member": "sc01_0077"})):
        result = lane.build_one(dict(row, region=region), tmp_path, {}, {}, "clang", info)
        assert result.get("rule") == "timing"


@pytest.mark.parametrize("vram,size", [
    (0x8017E868, 60), (0x8017E8A4, 128), (0x8017E924, 80),
    (0x8017E974, 76), (0x8017E9C0, 108), (0x8017EA2C, 48),
])
def test_forest_bridge_player_callback_timing(tmp_path, vram, size):
    source = ROOT / f"src/overlays/sc01_0077/{vram:08x}.c"
    row = dict(name=f"func_{vram:08X}", vram=vram, size=size, source=str(source))
    (tmp_path / "ir").mkdir()
    for region, info in (("sc01_0077", None), ("sc02_0031", {"member": "sc01_0077"})):
        result = lane.build_one(dict(row, region=region), tmp_path, {}, {}, "clang", info)
        assert result.get("rule") == "timing"


@pytest.mark.skipif(not shutil.which("clang"), reason="clang required for native selection")
@pytest.mark.parametrize("pc,size,region,member,rule", [
    (0x80163408, 84, "sc01_0077", "sc01_0077", "escape"),
    (0x8016345C, 124, "sc01_0077", "main_0012", "timing"),
    (0x8016345C, 124, "main_0012", "sc01_0077", "timing"),
    (0x8016345C, 124, "main_0012", "main_0012", None),
])
def test_forest_projectile_contact_selection(tmp_path, pc, size, region, member, rule):
    # Keep the source constant across the power lookup cases to test member selection.
    source = ROOT / f"src/overlays/sc01_0077/{pc:08x}.c"
    row = dict(name=f"func_{pc:08X}", vram=pc, size=size, region=region, source=str(source))
    info = dict(member=member, vram=pc, start=0, code=[(pc, pc + size)], data=bytes(size))
    (tmp_path / "ir").mkdir()
    result = lane.build_one(row, tmp_path, {}, {}, "clang", info)
    assert result.get("rule") == rule
    assert result["admitted"] == (rule is None)
