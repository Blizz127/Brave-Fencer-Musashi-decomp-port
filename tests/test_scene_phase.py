import os
from pathlib import Path
import shlex
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
ROOM = ROOT / 'src/overlays/sc02_0039'
CC = shlex.split(os.environ.get('CC') or 'cc')


@unittest.skipUnless(shutil.which(CC[0]), 'a host C compiler is required')
class ScenePhaseTests(unittest.TestCase):
    def test_countdown_and_terminal_fade(self):
        sources = [ROOM / name for name in (
            '8016ee40.c', '8017ce40.c', '8017cefc.c', '8017cf1c.c', '8017cf58.c', '8017cff4.c',
        )]
        sources.extend((ROOT / 'src/overlays/main_0010/800d1ebc.c', ROOT / 'tests/scene_phase_probe.c'))
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / 'scene-phase'
            subprocess.run([*CC, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                            '-I', ROOT / 'include', '-I', ROOM, *sources, '-o', output], check=True)
            subprocess.run([output], check=True)

    def test_fade_setup_callback_results(self):
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / 'scene-fade-setup'
            subprocess.run([*CC, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                            '-I', ROOT / 'include', '-I', ROOM, ROOM / '8016ee40.c',
                            ROOT / 'tests/scene_fade_setup_probe.c', '-o', output], check=True)
            subprocess.run([output], check=True)


if __name__ == '__main__':
    unittest.main()
