"""Run the standalone probe on Linux, Windows, or a Wine-mapped test directory."""
import argparse
import os
from pathlib import Path, PureWindowsPath
import subprocess
import tempfile


def exercise(probe, runner, root, guest, windows, unc=None):
    count = 0
    slash = "\\" if windows else "/"
    fallback = str(guest / "unselected")
    defaults = {"xdg": fallback, "home": fallback, "appdata": fallback}

    def run(result, selected, explicit="", xdg="", home="", appdata="", cap=512):
        nonlocal count
        cmd = [*runner, str(probe), str(cap), explicit, xdg, home, appdata]
        # Wine background services can inherit pipe handles after the probe exits.
        with tempfile.TemporaryFile(mode="w+") as output, tempfile.TemporaryFile(mode="w+") as errors:
            proc = subprocess.run(cmd, cwd=root, stdout=output, stderr=errors, timeout=100)
            output.seek(0)
            errors.seek(0)
            lines = output.read().splitlines()
            assert proc.returncode == 0 and lines == [str(result), selected], (
                cmd, lines, errors.read(), selected)
        count += 1

    def path(name):
        return str(guest / name)

    target = path("Musashi (%LITERAL%)") + slash + "one" + slash + "two"
    host_target = root / "Musashi (%LITERAL%)" / "one" / "two"
    run(1, target, explicit=target, **defaults)
    assert host_target.is_dir() and not list(host_target.iterdir())
    sentinel = host_target / "existing data"
    sentinel.write_bytes(b"existing user fixture\n")
    initial = sentinel.stat()
    run(1, target + slash, explicit=target + slash, **defaults)
    assert list(host_target.iterdir()) == [sentinel]
    assert sentinel.stat().st_mtime_ns == initial.st_mtime_ns
    assert sentinel.read_bytes() == b"existing user fixture\n"
    assert not (root / "unselected").exists()

    # One existing file blocks nested creation and selection remains unchanged.
    blocked = target + slash + sentinel.name
    run(0, blocked, explicit=blocked, **defaults)
    run(0, blocked + slash + "leaf", explicit=blocked + slash + "leaf", **defaults)
    assert not (root / "unselected").exists()

    run(1, "relative" + slash + "from cwd", explicit="relative" + slash + "from cwd")
    assert (root / "relative" / "from cwd").is_dir()
    short = path("capacity boundary")
    run(0, "", explicit=short, cap=len(short))
    run(0, "", explicit=short, cap=0)
    assert not (root / "capacity boundary").exists()
    run(1, short, explicit=short, cap=len(short) + 1)
    # The full 511-byte destination succeeds, including its final NUL in storage.
    pieces = ["capacity limit"]
    exact = path(pieces[0])
    while len(exact) < 511:
        part = "a" * min(80, 510 - len(exact))
        if not part:
            pieces[-1] += "a"
            exact += "a"
            break
        pieces.append(part)
        exact += slash + part
    run(1, exact, explicit=exact, cap=512)
    assert root.joinpath(*pieces).is_dir()
    run(0, "", explicit=exact + "x", cap=600)
    run(0, "")

    app = path("launcher (%LITERAL%)")
    expected = app + slash + "bfm-port"
    if windows:
        run(1, expected, appdata=app, home=fallback, xdg=fallback)
        run(1, expected, appdata=app + slash)
        run(0, "", home=fallback, xdg=fallback)
        run(0, blocked + slash + "bfm-port", appdata=blocked, home=fallback)
        drive_root = PureWindowsPath(guest.anchor)
        run(1, str(drive_root), explicit=str(drive_root))
        forward = path("forward") + "/nested/folder"
        run(1, forward, explicit=forward)
        assert (root / "forward" / "nested" / "folder").is_dir()
        if unc:
            unc = unc.rstrip("\\")
            # This share must point at the disposable test directory.
            run(1, unc, explicit=unc)
            run(1, unc + "\\", explicit=unc + "\\")
            dest = unc + "\\UNC (%LITERAL%)\\nested"
            run(1, dest, explicit=dest)
            assert (root / "UNC (%LITERAL%)" / "nested").is_dir()
            missing = unc.rsplit("\\", 1)[0] + "\\absent-share\\leaf"
            run(0, missing, explicit=missing, **defaults)
    else:
        run(1, expected, xdg=app, home=fallback)
        run(1, expected, xdg=app + slash)
        default = path("user") + "/.local/state/bfm-port"
        run(1, default, home=path("user"), appdata=fallback)
        assert (root / "user" / ".local" / "state" / "bfm-port").is_dir()
        run(0, "", appdata=fallback)
        run(0, blocked + slash + "bfm-port", xdg=blocked, home=fallback)
        run(1, "/", explicit="/")
        literal = path("one\\two")
        run(1, literal, explicit=literal)
        assert (root / "one\\two").is_dir()
    # Creating symlinks on Windows can require the local Developer Mode setting.
    link = root / "directory link"
    try:
        link.symlink_to(host_target, target_is_directory=True)
    except OSError:
        pass
    else:
        run(1, path(link.name), explicit=path(link.name))
        missing_link = root / "missing directory link"
        missing_link.symlink_to(root / "absent-target", target_is_directory=True)
        run(0, path(missing_link.name), explicit=path(missing_link.name), **defaults)
        assert not (root / "absent-target").exists()
    assert (root / "launcher (%LITERAL%)" / "bfm-port").is_dir()
    assert not (root / "unselected").exists()
    return count


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("probe", type=Path)
    parser.add_argument("--wine", help="Wine executable for a Windows probe")
    parser.add_argument("--root", type=Path, help="existing empty scratch directory")
    parser.add_argument("--unc", help="optional UNC alias of the scratch directory")
    args = parser.parse_args()
    probe = args.probe.resolve()

    def run(root):
        if any(root.iterdir()):
            parser.error("the test directory must be empty")
        if args.wine:
            guest = PureWindowsPath("Z:" + str(root).replace("/", "\\"))
        else:
            guest = root
        result = exercise(probe, [args.wine] if args.wine else [], root, guest,
                          bool(args.wine) or os.name == "nt", args.unc)
        print(f"{result} directory selections and filesystem observations passed")

    if args.root:
        run(args.root.resolve())
    else:
        with tempfile.TemporaryDirectory() as temporary:
            run(Path(temporary).resolve())


if __name__ == "__main__":
    main()
