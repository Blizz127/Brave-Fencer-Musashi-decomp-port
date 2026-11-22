"""Exercise the two known mine collision actor state callbacks on the host."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]

class MineCollisionActorTest(unittest.TestCase):
    def test_checked_state_callbacks(self):
        room = ROOT / 'src/overlays/sc02_0039'
        with tempfile.TemporaryDirectory() as temp:
            program = Path(temp) / 'collision-actor'
            subprocess.run([
                *shlex.split(os.environ.get('CC') or 'cc'), '-std=c11', '-O2', '-Wall', '-Wextra',
                '-Werror', '-I', ROOT / 'include', '-I', room, room / '8017dfe8.c',
                ROOT / 'tests/mine_collision_actor_probe.c', '-o', program,
            ], check=True)
            subprocess.run([program], check=True)

if __name__ == '__main__':
    unittest.main()
