"""Run the installed opening movie callback on a generated frame and native DMA."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class MovieDmaIrqTests(unittest.TestCase):
    def test_mounted_driver(self):
        executable = Path(os.environ.get(
            "MUSASHI_CODE_IMAGE", ROOT / "extracted/disc/files/SLUS_007.26"
        )).resolve()
        overlay = Path(os.environ.get(
            "MUSASHI_MOVIE_OVERLAY", ROOT / "extracted/overlays/main/0004.bin"
        )).resolve()
        if not executable.is_file() or not overlay.is_file():
            self.skipTest("requires a user-supplied USA executable and unpacked MAIN member 4")
        port = ROOT / "pc_port"
        sources = (
            "mips_formatter", "boot_memory", "dma_controller", "irq_controller",
            "mdec_controller", "gpu_controller", "gpu_dma2", "scanline_timer", "cd_dma3"
        )
        with tempfile.TemporaryDirectory() as directory:
            probe = Path(directory) / "movie-irq"
            subprocess.run([
                "cc", "-std=c11", "-O1", "-ffunction-sections", "-fdata-sections",
                "-Wl,--gc-sections", "-I", ROOT / "include", "-I", port / "include",
                ROOT / "tests/movie_dma1_irq_probe.c",
                *(port / (name + ".c") for name in sources), "-lcrypto", "-lm", "-o", probe
            ], check=True, timeout=180)
            env = os.environ.copy()
            env.pop("MUSASHI_CODE_IMAGE", None)
            env.pop("MUSASHI_CODE_IMAGE_MUTANT", None)
            for mode in ("pass", "foreign", "badcode", "changedowner", "active",
                         "selector", "geometry", "overlap", "member"):
                with self.subTest(mode=mode):
                    subprocess.run([probe, executable, overlay, mode], check=True,
                                   timeout=20, env=env)


if __name__ == "__main__":
    unittest.main()
