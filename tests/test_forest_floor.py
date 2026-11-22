"""Link the forest floor routines as separate translation units."""
from pathlib import Path
import subprocess

ROOT = Path(__file__).resolve().parents[1]


def test_forest_floor(tmp_path):
    sources = ROOT / "src/overlays/sc01_0077"
    binary = tmp_path / "floor"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests/forest_floor_probe.c", sources / "80134510.c", sources / "801345f8.c",
        sources / "801347a0.c", sources / "80134a28.c", sources / "80134a74.c",
        sources / "80134c20.c", sources / "80134fb8.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)


def test_floor_sources_stay_interpreted(tmp_path, monkeypatch):
    import sys
    sys.path.insert(0, str(ROOT / "tools"))
    import native_lane_gen as lane

    for pc, variants in lane.FOREST_FLOOR_EXTENTS.items():
        alternative = tmp_path / f"independent_{pc:08x}.c"
        row = dict(vram=pc, size=0, region="sc99_0001")
        for member, (size, digest) in variants.items():
            source = ROOT / "src/overlays" / member / f"{pc:08x}.c"
            alternative.write_bytes(source.read_bytes())
            row["size"] = size
            assert lane.forest_floor_source_only(row, source, None)
            assert lane.forest_floor_source_only(row, alternative, None)
            alternative.write_text("int independent(void) { return 1; }\n")
            assert lane.forest_floor_source_only(dict(row, region=member), alternative, None)
            info = dict(member=member)
            assert lane.forest_floor_source_only(row, alternative, info)
            info["member"] = "sc99_0001"
            with monkeypatch.context() as mp:
                mp.setattr(lane, "member_digest", lambda *args: digest)
                assert lane.forest_floor_source_only(row, alternative, info)
                assert not lane.forest_floor_source_only(dict(row, size=size+4), alternative, info)
                mp.setattr(lane, "member_digest", lambda *args: None)
                assert not lane.forest_floor_source_only(row, alternative, info)
            assert not lane.forest_floor_source_only(row, alternative, None)


def test_mine_floor_sources_require_their_member_even_without_a_disc(tmp_path):
    import sys
    sys.path.insert(0, str(ROOT / "tools"))
    import native_lane_gen as lane

    copy = tmp_path / "independent.c"
    copy.write_text("int independent(int value) { return value + 1; }\n")
    for pc, size in ((0x801347A0, 648), (0x80134A28, 76), (0x80135480, 1032)):
        row = dict(region="sc02_0039", vram=pc, size=size)
        assert lane.forest_floor_source_only(row, copy, None)
        assert not lane.forest_floor_source_only(dict(row, region="sc99_0001"), copy, None)


def test_forest_contact_transform(tmp_path):
    binary = tmp_path / "contact-transform"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests/forest_contact_transform_probe.c",
        ROOT / "src/overlays/sc01_0077/80136a94.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)


def test_moving_segment_cell_retry(tmp_path):
    binary = tmp_path / "moving-segment"
    source = ROOT / "src/overlays/sc01_0077/80135004.c"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests/forest_moving_segment_probe.c", source, "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)


def test_actor_slope_coordinates(tmp_path):
    binary = tmp_path / "actor-slope"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests/forest_actor_slope_probe.c", ROOT / "src/overlays/main_0012/801638a0.c",
        ROOT / "src/overlays/main_0012/8016380c.c",
        ROOT / "src/main/80015978.c", ROOT / "src/main/80015954.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)


def test_model_contact_link_and_grid_retries(tmp_path):
    binary = tmp_path / "model-contact"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests/forest_model_contact_probe.c", ROOT / "src/overlays/sc01_0077/80135a4c.c",
        "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)


def test_model_grid_face_order_and_material(tmp_path):
    binary = tmp_path / "model-grid"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests/forest_model_grid_probe.c", ROOT / "src/overlays/sc01_0077/80133ab0.c",
        "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)


def test_grid_controllers_retry_and_slope(tmp_path):
    binary = tmp_path / "grid-controllers"
    sources = ROOT / "src/overlays/sc01_0077"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests/forest_grid_controllers_probe.c", sources / "80133784.c",
        sources / "801343c4.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)


def test_grid_origin_overlaps_contact_fields(tmp_path):
    binary = tmp_path / "grid-contact-alias"
    subprocess.run([
        "cc", "-std=c11", "-O2", "-Wall", "-Wextra", "-Werror", "-I", ROOT / "include",
        ROOT / "tests/forest_grid_scalar_alias_probe.c", "-o", binary,
    ], check=True)
    subprocess.run([binary], check=True)
