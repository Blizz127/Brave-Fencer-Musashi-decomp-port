import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SHOP = ROOT / "src" / "overlays" / "sc03_0001"


class ShopMenuMessagesTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which(os.environ.get("CC", "cc")), "requires a C compiler")
    def test_lookup_and_latch(self):
        with tempfile.TemporaryDirectory() as directory:
            exe = Path(directory) / "shop-menu-messages"
            # Bind the scalar to an array's middle word so both neighbors are observable.
            subprocess.run(
                [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                 "-Werror", "-O2", "-I", ROOT / "include",
                 "-DD_801EF1E0=(*shop_test_latch)", SHOP / "8018c89c.c",
                 SHOP / "8018c8d4.c", ROOT / "tests/shop_menu_message_probe.c",
                 "-o", exe],
                check=True,
            )
            subprocess.run([exe], check=True)
