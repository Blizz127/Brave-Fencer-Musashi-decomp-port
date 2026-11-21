"""Run the Windows card backend under a new temporary folder on each selected root.

On Windows, CTest uses the local temporary drive. Supply --drive-root R:\\ for
another mapped drive, or set BFM_TEST_CARD_DRIVE for CTest. An optional --unc-root or BFM_TEST_CARD_UNC names an
existing writable share. The probe uses the same SDL2 and OpenAL DLL search
path as the player. A cross-compiled probe can be run with --wine wine64.
"""
import argparse
import os
from pathlib import Path, PureWindowsPath
import subprocess
import tempfile


def host_name(root, wine):
    if wine:
        return Path(subprocess.check_output(["winepath", "-u", str(root)], text=True).strip())
    name = str(root)
    if name.startswith("\\\\"):
        return Path("\\\\?\\UNC\\" + name[2:])
    return Path("\\\\?\\" + name)


def test_root(root, probe, wine):
    host = host_name(root, wine)
    with tempfile.TemporaryDirectory(prefix="bfm-card-", dir=host) as directory:
        for length in (320, 511):
            folder = Path(directory) / str(length)
            path = root / Path(directory).name / str(length)
            while len(str(path)) < length:
                remaining = length - len(str(path))
                if remaining == 1:
                    folder = folder.with_name(folder.name + "x")
                    path = path.with_name(path.name + "x")
                    break
                name = "x" * min(64, remaining - 1)
                folder, path = folder / name, path / name
            if len(str(path)) != length:
                raise ValueError("Choose a shorter temporary root")
            folder.mkdir(parents=True)

            def run(commands, expected):
                with tempfile.TemporaryFile(mode="w+") as log:
                    args = ([wine] if wine else []) + [str(probe), str(path)]
                    result = subprocess.run(args, input=commands + "end\n", text=True,
                                            stdout=log, stderr=log, timeout=45)
                    log.seek(0)
                    output = log.read().splitlines()
                    if result.returncode or output != expected:
                        raise RuntimeError(f"{length}-character path exited {result.returncode}: {output!r}")

            run("attach 0 0\nput 0 0 -1 17\n", ["0", "1"])
            run("attach 0 0\nsnap 0 0 17\nput 0 0 17 31\n", ["0", "0 1", "1"])
            run("attach 0 0\nsnap 0 0 31\n", ["0", "0 1"])
            expected = bytes((i * 23 + i // 113 + 31) % 256 for i in range(131072))
            if (folder / "bfm_card0.mcd").read_bytes() != expected:
                raise RuntimeError("Committed image differs after reopen")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("probe", type=Path)
    parser.add_argument("--drive-root", default=os.environ.get("BFM_TEST_CARD_DRIVE"),
                        help="existing directory on a Windows drive")
    parser.add_argument("--unc-root", default=os.environ.get("BFM_TEST_CARD_UNC"))
    parser.add_argument("--wine", help="run a cross-compiled Windows executable through Wine")
    args = parser.parse_args()
    default = "C:\\windows\\temp" if args.wine else tempfile.gettempdir()
    drive = PureWindowsPath(args.drive_root or default)
    if not drive.is_absolute() or len(drive.drive) != 2:
        parser.error("a drive-letter path is required for the local test")
    test_root(drive, args.probe.resolve(), args.wine)
    if args.unc_root:
        share = PureWindowsPath(args.unc_root)
        if not share.is_absolute() or not share.drive.startswith("\\\\"):
            parser.error("the UNC test requires a share path")
        test_root(share, args.probe.resolve(), args.wine)


if __name__ == "__main__":
    main()
