import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ShopWalletHudTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which(os.environ.get("CC", "cc")), "requires a C compiler")
    def test_packet_construction_and_amount_changes(self):
        scene = ROOT / "src/overlays/sc03_0001"
        with tempfile.TemporaryDirectory() as directory:
            exe = Path(directory) / "shop-wallet-hud"
            subprocess.run(
                [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                 "-Werror", "-O2", "-I", ROOT / "include",
                 scene / "80177ea4.c", scene / "80177f84.c",
                 scene / "801783d0.c", scene / "8018c914.c",
                 ROOT / "tests/shop_wallet_hud_probe.c", "-o", exe],
                check=True,
            )
            subprocess.run([exe], check=True)
