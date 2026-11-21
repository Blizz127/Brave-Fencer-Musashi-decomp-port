"""Data-free regressions for the Linux player package's provenance."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from legion_metadata import build_config, sha256, source_identity


class LegionMetadataTests(unittest.TestCase):
    def test_cache_tracks_selected_configuration_and_native_lane(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            cache = root / "CMakeCache.txt"
            home = f"CMAKE_HOME_DIRECTORY:INTERNAL={root}\n"
            self.assertEqual(build_config(root / "copied-player"), (None, None))
            cache.write_text(home + "CMAKE_BUILD_TYPE:STRING=\n")
            self.assertEqual(build_config(root / "player"), (None, None))
            cache.write_text(home + "CMAKE_BUILD_TYPE:STRING=Debug\nMUSASHI_NATIVE_LANE:BOOL=OFF\n")
            self.assertEqual(build_config(root / "player"), ("Debug", False))
            cache.write_text(home + "CMAKE_CONFIGURATION_TYPES:STRING=Debug;Release\nMUSASHI_NATIVE_LANE:BOOL=ON\n")
            self.assertEqual(build_config(root / "Release/player"), ("Release", True))
            self.assertEqual(build_config(root / "Unknown/player"), (None, None))

    def test_only_a_matching_test_receipt_identifies_the_player_source(self):
        with tempfile.TemporaryDirectory() as temp:
            root = Path(temp)
            git = ["git", "-C", root, "-c", "user.name=Fixture", "-c", "user.email=fixture@example.invalid"]
            subprocess.run(git + ["init", "-q", "-b", "fixture"], check=True)
            subprocess.run(git + ["commit", "-qm", "build", "--allow-empty"], check=True)
            commit = subprocess.check_output(git + ["rev-parse", "HEAD"], text=True).strip()
            player, shim = root / "player", root / "shim"
            player.write_bytes(b"synthetic player")
            shim.write_bytes(b"synthetic shim")
            receipt = {"source_commit": commit, "source_branch": "fixture", "player_sha256": sha256(player),
                       "helper_sha256": sha256(shim), "build_type": "Release", "private_notes": "omitted"}
            subprocess.run(git + ["commit", "-qm", "new checkout", "--allow-empty"], check=True)
            unknown = source_identity(root, player, shim, "Release", None)
            self.assertIsNone(unknown["source_commit"])
            self.assertNotEqual(unknown["packaging_checkout_commit"], commit)
            actual = source_identity(root, player, shim, "Release", receipt)
            self.assertEqual(actual["source_commit"], commit)
            self.assertEqual(actual["source_branch"], "fixture")
            self.assertNotIn("private_notes", actual)
            with self.assertRaisesRegex(ValueError, "build type"):
                source_identity(root, player, shim, "Debug", receipt)
            shim.write_bytes(b"other shim")
            with self.assertRaisesRegex(ValueError, "differs from candidate receipt"):
                source_identity(root, player, shim, "Release", receipt)
