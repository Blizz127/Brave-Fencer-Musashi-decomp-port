"""Data-free regressions for the Linux player package's provenance."""
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "tools"))
from legion_metadata import build_config, host_requirements, sha256, source_identity


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

    def test_host_requirements_follow_imports_and_exclude_bundled_sonames(self):
        output = {
            ("-d", "player"): "(NEEDED) Shared library: [libfixture.so]\n(NEEDED) Shared library: [libc.so.6]",
            ("-d", "lib/libfixture.so"): "(NEEDED) Shared library: [libstdc++.so.6]",
            ("--version-info", "player"): "Version needs section '.gnu.version_r'\n Name: GLIBC_2.9",
            ("--version-info", "lib/libfixture.so"): (
                "Version definition section '.gnu.version_d'\n Name: GLIBC_999.9\n"
                "Version needs section '.gnu.version_r'\n Name: GLIBCXX_3.4.32\n Name: CXXABI_1.3.15\n"
                " Name: GLIBC_2.38\n Name: GCC_4.3.0"),
        }
        root = Path("/synthetic-stage")
        def readelf(argv, *, text, env):
            self.assertEqual(argv[:2], ["readelf", "-W"])
            self.assertEqual(env["LC_ALL"], "C")
            return output[argv[2], argv[3].relative_to(root).as_posix()]
        with patch("legion_metadata.subprocess.check_output", side_effect=readelf):
            result = host_requirements(root, ["player", "lib/libfixture.so"])
        self.assertEqual(result["direct_host_sonames"], {
            "player": ["libc.so.6"], "lib/libfixture.so": ["libstdc++.so.6"]})
        self.assertEqual(result["symbol_versions"], {
            "GLIBC": "2.38", "GLIBCXX": "3.4.32", "CXXABI": "1.3.15", "GCC": "4.3.0"})

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
