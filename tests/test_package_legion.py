"""Check how the packaged Linux launcher selects a player's disc image."""

import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


LAUNCHER = Path(__file__).resolve().parents[1] / "tools" / "launch_legion.sh"


class LegionDiscSelectionTests(unittest.TestCase):
    def setUp(self):
        scratch = tempfile.TemporaryDirectory()
        self.addCleanup(scratch.cleanup)
        self.work = Path(scratch.name)
        home = self.work / "test home"
        games = home / "Games"
        self.library_disc = games / "Example Library" / "games" / "brave-fencer-musashi" / "disc"
        self.home_disc = games / "brave-fencer-musashi" / "disc"
        self.package = self.work / "test package"
        self.package.mkdir()
        self.launcher = self.package / "launch.sh"
        shutil.copy2(LAUNCHER, self.launcher)
        self.record = self.work / "player arguments"
        player = self.package / "musashi_native_boot"
        player.write_text('#!/bin/sh\nprintf "%s\\n" "$@" > "$TEST_PLAYER_ARGS"\n'
                          'printf "%s\\n" "$BFM_LAUNCH_LOG"\n')
        player.chmod(0o755)

        # A failed lookup should fail the test without opening a desktop dialog.
        commands = self.work / "commands"
        commands.mkdir()
        for name in ("zenity", "kdialog"):
            picker = commands / name
            picker.write_text("#!/bin/sh\nexit 1\n")
            picker.chmod(0o755)
        config = self.work / "config"
        self.remembered = config / "bfm-port" / "disc"
        self.env = {
            "HOME": str(home), "XDG_CONFIG_HOME": str(config),
            "XDG_STATE_HOME": str(self.work / "state"),
            "PATH": str(commands) + os.pathsep + os.defpath,
            "TEST_PLAYER_ARGS": str(self.record),
        }

    def image(self, directory, name="Disc One.cue"):
        directory.mkdir(parents=True, exist_ok=True)
        image = directory / name
        image.touch()
        return image

    def remember(self, image):
        self.remembered.parent.mkdir(parents=True, exist_ok=True)
        self.remembered.write_text(str(image) + "\n")

    def expect_launch(self, expected, **environment):
        result = subprocess.run([str(self.launcher), "--extra", "two words"],
                                cwd=self.work, env={**self.env, **environment},
                                capture_output=True, text=True, timeout=5)
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertEqual(self.record.read_text().splitlines(),
                         ["--disc", str(expected), "--extra", "two words"])
        self.assertEqual(self.remembered.read_text().splitlines(), [str(expected)])
        log = Path(self.env["XDG_STATE_HOME"]) / "bfm-port" / "last-run.log"
        self.assertEqual(log.read_text().splitlines(), [str(log)])

    def test_single_level_library_fallback_prefers_cue(self):
        self.image(self.library_disc, "A track.bin")
        self.expect_launch(self.image(self.library_disc, "Disc One.CUE"))

    def test_home_game_directory_precedes_library(self):
        self.image(self.library_disc)
        self.expect_launch(self.image(self.home_disc))

    def test_package_directory_precedes_home_game_directory(self):
        self.image(self.home_disc)
        self.expect_launch(self.image(self.package / "disc"))

    def test_configured_directory_with_spaces_precedes_package_directory(self):
        self.image(self.package / "disc")
        configured = self.work / "configured disc images"
        self.expect_launch(self.image(configured), BFM_DISC_DIR=str(configured))

    def test_remembered_image_precedes_directory_search(self):
        configured = self.work / "configured disc images"
        self.image(configured)
        self.image(self.package / "disc")
        remembered = self.image(self.work / "previous selection", "Saved image.bin")
        self.remember(remembered)
        self.expect_launch(remembered, BFM_DISC_DIR=str(configured))

    def test_explicit_image_precedes_remembered_image(self):
        self.remember(self.image(self.work / "previous selection"))
        explicit = self.image(self.work / "new selection", "New image.BIN")
        self.expect_launch(explicit, BFM_DISC=str(explicit))


if __name__ == "__main__":
    unittest.main()
