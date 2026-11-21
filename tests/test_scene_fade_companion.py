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
class SceneFadeCompanionTests(unittest.TestCase):
    def test_setup_and_fade_poll(self):
        names = ('8017cd9c.c', '8017cdd8.c', '8017ce1c.c', '8017cfcc.c', '8016ee40.c')
        with tempfile.TemporaryDirectory() as temporary:
            output = Path(temporary) / 'scene-fade-companion'
            subprocess.run([*CC, '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror', '-UNDEBUG',
                            '-I', ROOT / 'include', '-I', ROOM, *(ROOM / name for name in names),
                            ROOT / 'tests/scene_fade_companion_probe.c', '-o', output], check=True)
            subprocess.run([output], check=True)


if __name__ == '__main__':
    unittest.main()
