"""Test the bounded register APIs with locally supplied resident code."""
import ctypes
import hashlib
import os
from pathlib import Path
import struct
import subprocess
import unittest


def compile_registration_api(root, directory, symbols):
    directory = Path(directory)
    version = directory / 'public-symbols.map'
    version.write_text('{ global: ' + '; '.join(symbols) + '; local: *; };\n')
    library = directory / 'registration.so'
    subprocess.run([
        'cc', '-std=c11', '-O1', '-shared', '-fPIC',
        '-ffunction-sections', '-fdata-sections', '-Wl,--gc-sections', '-Wl,-z,defs',
        f'-Wl,--version-script={version}', '-I', str(root / 'include'),
        '-I', str(root / 'pc_port/include'), str(root / 'pc_port/mips_formatter.c'),
        str(root / 'pc_port/boot_memory.c'), '-lcrypto', '-o', str(library),
    ], check=True, stdin=subprocess.DEVNULL, timeout=90)
    return ctypes.CDLL(str(library))


def map_registration_image(memory, root):
    source = os.environ.get('MUSASHI_CODE_IMAGE')
    path = Path(source) if source else root / 'extracted/disc/files/SLUS_007.26'
    if not path.is_file():
        raise unittest.SkipTest('The registration test needs the locally supplied US disc executable')
    image = path.read_bytes()
    if hashlib.sha256(image).hexdigest() != '66371c3a7517e9eabd7cb6cf0c5abffe7296bd4bac29c85b8bf7bb9db349714a':
        raise ValueError('Unexpected guest executable identity')
    address, size = struct.unpack_from('<II', image, 0x18)
    # The tests set up their own data cells and checked device owners. Copy
    # only the four resident function spans exercised by the standalone APIs.
    for begin, end in ((0x800425B0, 0x80042610), (0x800429DC, 0x80042B30),
                       (0x80042F8C, 0x80043034)):
        if not (address <= begin < end <= address + size):
            raise ValueError('Registration functions outside the mapped image')
        offset = begin - address + 2048
        ctypes.memmove(ctypes.addressof(memory) + begin - 0x80000000,
                       image[offset:offset + end - begin], end - begin)
