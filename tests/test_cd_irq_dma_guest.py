from pathlib import Path
import os
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[1]


def test_dma_registration_carrier():
    with tempfile.TemporaryDirectory(prefix='musashi-cd-irq-dma-') as scratch:
        probe = Path(scratch) / 'probe'
        subprocess.run([
            'cc', '-std=c11', '-O1', '-ffunction-sections', '-fdata-sections',
            '-Wl,--gc-sections', '-I', str(ROOT / 'include'), '-I', str(ROOT / 'pc_port/include'),
            str(ROOT / 'tests/cd_irq_dma_guest_probe.c'), str(ROOT / 'pc_port/boot_memory.c'),
            '-lcrypto', '-o', str(probe),
        ], check=True, timeout=180)
        subprocess.run([str(probe)], check=True, timeout=30)
        image = Path(os.environ.get('MUSASHI_CODE_IMAGE', ROOT / 'extracted/disc/files/SLUS_007.26'))
        if image.is_file():
            subprocess.run([str(probe), str(image)], check=True, timeout=30)
