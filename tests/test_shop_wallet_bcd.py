import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]


class ShopWalletBcdTest(unittest.TestCase):
    @unittest.skipUnless(shutil.which(os.environ.get("CC", "cc")), "requires a C compiler")
    def test_decimal_digits_and_word_shifts(self):
        with tempfile.TemporaryDirectory() as directory:
            exe = Path(directory) / "shop-wallet-bcd"
            subprocess.run(
                [os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra",
                 "-Werror", "-O2", "-I", ROOT / "include",
                 ROOT / "src/overlays/sc03_0001/801783d0.c",
                 ROOT / "tests/shop_wallet_bcd_probe.c", "-o", exe],
                check=True,
            )
            subprocess.run([exe], check=True)
